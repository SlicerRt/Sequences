/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

#include "vtkMRMLNodeSequencer.h"

// MRML includes
#include "vtkMRMLNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkAbstractTransform.h>
#include <vtkCommand.h>
#include <vtkImageData.h>
#include <vtkIntArray.h>
#include <vtkMatrix4x4.h>
#include <vtkMRMLCameraNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLVolumeNode.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>

//----------------------------------------------------------------------------

vtkMRMLNodeSequencer::NodeSequencer::NodeSequencer()
{
  this->RecordingEvents = vtkSmartPointer< vtkIntArray >::New();
  this->RecordingEvents->InsertNextValue(vtkCommand::ModifiedEvent);
  this->SupportedNodeClassName = "vtkMRMLNode";
}

vtkMRMLNodeSequencer::NodeSequencer::~NodeSequencer()
{
}

vtkIntArray* vtkMRMLNodeSequencer::NodeSequencer::GetRecordingEvents()
{
  return this->RecordingEvents;
}

std::string vtkMRMLNodeSequencer::NodeSequencer::GetSupportedNodeClassName()
{
  return this->SupportedNodeClassName;
}

bool vtkMRMLNodeSequencer::NodeSequencer::IsNodeSupported(vtkMRMLNode* node)
{
  if (!node)
  {
    return false;
  }
  return node->IsA(this->SupportedNodeClassName.c_str());
}

bool vtkMRMLNodeSequencer::NodeSequencer::IsNodeSupported(const std::string& nodeClassName)
{
  if (nodeClassName == this->SupportedNodeClassName)
  {
    return true;
  }
  for (std::vector< std::string >::iterator supportedClassNameIt = this->SupportedNodeParentClassNames.begin();
    supportedClassNameIt != this->SupportedNodeParentClassNames.end(); ++supportedClassNameIt)
  {
    if (*supportedClassNameIt == nodeClassName)
    {
      return true;
    }
  }
  return false;
}

void vtkMRMLNodeSequencer::NodeSequencer::CopyNode(vtkMRMLNode* source, vtkMRMLNode* target, bool vtkNotUsed(shallowCopy) /* =false */)
{
  // TODO: maybe copy without node references?
  target->CopyWithSingleModifiedEvent(source);
}

vtkMRMLNode* vtkMRMLNodeSequencer::NodeSequencer::DeepCopyNodeToScene(vtkMRMLNode* source, vtkMRMLScene* scene)
{
  if (source == NULL)
  {
    vtkGenericWarningMacro("NodeSequencer::CopyNode failed, invalid node");
    return NULL;
  }
  std::string newNodeName = scene->GetUniqueNameByString(source->GetName() ? source->GetName() : "Sequence");

  vtkSmartPointer<vtkMRMLNode> target = vtkSmartPointer<vtkMRMLNode>::Take(source->CreateNodeInstance());
  this->CopyNode(source, target, false);

  // Make sure all the node names in the sequence's scene are unique for saving purposes
  // TODO: it would be better to make sure all names are unique when saving the data?
  target->SetName(newNodeName.c_str());

  vtkMRMLNode* addedTargetNode = scene->AddNode(target);
  return addedTargetNode;
}

//----------------------------------------------------------------------------

class ScalarVolumeNodeSequencer : public vtkMRMLNodeSequencer::NodeSequencer
{
public:
  ScalarVolumeNodeSequencer()
  {
    this->RecordingEvents->InsertNextValue(vtkMRMLVolumeNode::ImageDataModifiedEvent);
    this->SupportedNodeClassName = "vtkMRMLVolumeNode";
    this->SupportedNodeParentClassNames.push_back("vtkMRMLDisplayableNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLTransformableNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLStorableNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLNode");
  }

  virtual void CopyNode(vtkMRMLNode* source, vtkMRMLNode* target, bool shallowCopy /* =false */)
  {
    int oldModified = target->StartModify();
    vtkMRMLVolumeNode* targetVolumeNode = vtkMRMLVolumeNode::SafeDownCast(target);
    vtkMRMLVolumeNode* sourceVolumeNode = vtkMRMLVolumeNode::SafeDownCast(source);
    // targetScalarVolumeNode->SetAndObserveTransformNodeID is not called, as we want to keep the currently applied transform
    vtkSmartPointer<vtkMatrix4x4> ijkToRasmatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    sourceVolumeNode->GetIJKToRASMatrix(ijkToRasmatrix);
    targetVolumeNode->SetIJKToRASMatrix(ijkToRasmatrix);
    vtkSmartPointer<vtkImageData> targetImageData = sourceVolumeNode->GetImageData();
    if (!shallowCopy)
    {
      targetImageData = sourceVolumeNode->GetImageData()->NewInstance();
      targetImageData->DeepCopy(sourceVolumeNode->GetImageData());
    }
    targetVolumeNode->SetAndObserveImageData(targetImageData); // invokes vtkMRMLVolumeNode::ImageDataModifiedEvent, which is not masked by StartModify
    target->EndModify(oldModified);
  }

  /*
  virtual vtkMRMLNode* DeepCopyNodeToScene(vtkMRMLNode* source, vtkMRMLScene* scene)
  {
    vtkMRMLNode* newNode = vtkMRMLNodeSequencer::NodeSequencer::DeepCopyNodeToScene(source, scene);
    if (newNode == NULL)
    {
      return NULL;
    }

    // To DeepCopy volumes we need to deep copy the image data
    vtkMRMLVolumeNode* volumeNode = vtkMRMLVolumeNode::SafeDownCast(newNode);
    if (volumeNode != NULL)
    {
      vtkGenericWarningMacro("ScalarVolumeNodeSequencer::DeepCopyToScene failed: volume node is expected");
      return newNode;
    }

    // Need to get a observe a copy of the image data (this is OK even if it is NULL)
    int wasModified = newNode->StartModify();
    vtkNew<vtkImageData> imageDataCopy;
    imageDataCopy->DeepCopy(volumeNode->GetImageData());
    volumeNode->SetAndObserveImageData(imageDataCopy.GetPointer());
    newNode->EndModify(wasModified);

    return newNode;
  }
  */

};

//----------------------------------------------------------------------------

class TransformNodeSequencer : public vtkMRMLNodeSequencer::NodeSequencer
{
public:
  TransformNodeSequencer()
  {
    this->RecordingEvents->InsertNextValue(vtkMRMLTransformableNode::TransformModifiedEvent);
    this->SupportedNodeClassName = "vtkMRMLTransformNode";
    this->SupportedNodeParentClassNames.push_back("vtkMRMLDisplayableNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLTransformableNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLStorableNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLNode");
  }

  virtual void CopyNode(vtkMRMLNode* source, vtkMRMLNode* target, bool shallowCopy /* =false */)
  {
    int oldModified = target->StartModify();
    vtkMRMLTransformNode* targetTransformNode = vtkMRMLTransformNode::SafeDownCast(target);
    vtkMRMLTransformNode* sourceTransformNode = vtkMRMLTransformNode::SafeDownCast(source);
    vtkSmartPointer<vtkAbstractTransform> targetAbstractTransform = sourceTransformNode->GetTransformToParent();
    if (!shallowCopy)
    {
      targetAbstractTransform = sourceTransformNode->GetTransformToParent()->NewInstance();
      targetAbstractTransform->DeepCopy(sourceTransformNode->GetTransformToParent());
    }
    targetTransformNode->SetAndObserveTransformToParent(targetAbstractTransform);
    target->EndModify(oldModified);
  }

};

//----------------------------------------------------------------------------

class ModelNodeSequencer : public vtkMRMLNodeSequencer::NodeSequencer
{
public:
  ModelNodeSequencer()
  {
    this->RecordingEvents->InsertNextValue(vtkMRMLModelNode::PolyDataModifiedEvent);
    this->SupportedNodeClassName = "vtkMRMLModelNode";
    this->SupportedNodeParentClassNames.push_back("vtkMRMLDisplayableNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLTransformableNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLStorableNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLNode");
  }

  virtual void CopyNode(vtkMRMLNode* source, vtkMRMLNode* target, bool shallowCopy /* =false */)
  {
    int oldModified = target->StartModify();
    vtkMRMLModelNode* targetModelNode = vtkMRMLModelNode::SafeDownCast(target);
    vtkMRMLModelNode* sourceModelNode = vtkMRMLModelNode::SafeDownCast(source);
    vtkSmartPointer<vtkPolyData> targetPolyData = sourceModelNode->GetPolyData();
    if (!shallowCopy)
    {
      targetPolyData = sourceModelNode->GetPolyData()->NewInstance();
      targetPolyData->DeepCopy(sourceModelNode->GetPolyData());
    }
    targetModelNode->SetAndObservePolyData(targetPolyData);
    target->EndModify(oldModified);
  }

};

//----------------------------------------------------------------------------

class MarkupsFiducialNodeSequencer : public vtkMRMLNodeSequencer::NodeSequencer
{
public:
  MarkupsFiducialNodeSequencer()
  {
    // TODO: check if a special event is needed
    //this->RecordingEvents->InsertNextValue(vtkMRMLModelNode::PolyDataModifiedEvent);
    this->SupportedNodeClassName = "vtkMRMLMarkupsFiducialNode";
    this->SupportedNodeParentClassNames.push_back("vtkMRMLMarkupsNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLDisplayableNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLTransformableNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLStorableNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLNode");
  }

  virtual void CopyNode(vtkMRMLNode* source, vtkMRMLNode* target, bool shallowCopy /* =false */)
  {
    // Don't copy with single-modified-event
    // TODO: check if default behavior is really not correct
    int oldModified = target->StartModify();
    vtkMRMLMarkupsFiducialNode* targetMarkupsFiducialNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(target);
    vtkMRMLMarkupsFiducialNode* sourceMarkupsFiducialNode = vtkMRMLMarkupsFiducialNode::SafeDownCast(source);
    targetMarkupsFiducialNode->Copy(sourceMarkupsFiducialNode);
    target->EndModify(oldModified);
  }

};

//----------------------------------------------------------------------------

class CameraNodeSequencer : public vtkMRMLNodeSequencer::NodeSequencer
{
public:
  CameraNodeSequencer()
  {
    this->SupportedNodeClassName = "vtkMRMLCameraNode";
    this->SupportedNodeParentClassNames.push_back("vtkMRMLTransformableNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLStorableNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLNode");
  }

  virtual void CopyNode(vtkMRMLNode* source, vtkMRMLNode* target, bool shallowCopy /* =false */)
  {
    vtkMRMLCameraNode* targetCameraNode = vtkMRMLCameraNode::SafeDownCast(target);
    vtkMRMLCameraNode* sourceCameraNode = vtkMRMLCameraNode::SafeDownCast(source);
    int disabledModify = targetCameraNode->StartModify();
    targetCameraNode->SetPosition(sourceCameraNode->GetPosition());
    targetCameraNode->SetFocalPoint(sourceCameraNode->GetFocalPoint());
    targetCameraNode->SetViewUp(sourceCameraNode->GetViewUp());
    targetCameraNode->SetParallelProjection(sourceCameraNode->GetParallelProjection());
    targetCameraNode->SetParallelScale(sourceCameraNode->GetParallelScale());
    // Maybe the new position and focalpoint combo doesn't fit the existing
    // clipping range
    targetCameraNode->ResetClippingRange();
    targetCameraNode->Modified();
    targetCameraNode->EndModify(disabledModify);
  }

};

//----------------------------------------------------------------------------
// Needed when we don't use the vtkStandardNewMacro.
vtkInstantiatorNewMacro(vtkMRMLNodeSequencer);

//----------------------------------------------------------------------------
// vtkMRMLNodeSequencer methods

//----------------------------------------------------------------------------
// Up the reference count so it behaves like New
vtkMRMLNodeSequencer* vtkMRMLNodeSequencer::New()
{
  vtkMRMLNodeSequencer* instance = Self::GetInstance();
  instance->Register(0);
  return instance;
}

//----------------------------------------------------------------------------
vtkMRMLNodeSequencer* vtkMRMLNodeSequencer::GetInstance()
{
  if(!Self::Instance)
    {
    // Try the factory first
    Self::Instance = (vtkMRMLNodeSequencer*)
                     vtkObjectFactory::CreateInstance("vtkMRMLNodeSequencer");

    // if the factory did not provide one, then create it here
    if(!Self::Instance)
      {
      Self::Instance = new vtkMRMLNodeSequencer;
      }
    }
  // return the instance
  return Self::Instance;
}

//----------------------------------------------------------------------------
vtkMRMLNodeSequencer::vtkMRMLNodeSequencer():Superclass()
{
  this->NodeSequencers.push_back(new NodeSequencer());
  this->RegisterNodeSequencer(new ScalarVolumeNodeSequencer());
  this->RegisterNodeSequencer(new TransformNodeSequencer());
  this->RegisterNodeSequencer(new ModelNodeSequencer());
  this->RegisterNodeSequencer(new CameraNodeSequencer());
  // TODO: fix link error
  //this->RegisterNodeSequencer(new MarkupsFiducialNodeSequencer());
}

//----------------------------------------------------------------------------
vtkMRMLNodeSequencer::~vtkMRMLNodeSequencer()
{
  for (std::list< NodeSequencer* >::iterator sequencerIt = this->NodeSequencers.begin();
    sequencerIt != this->NodeSequencers.end(); ++sequencerIt)
  {
    delete (*sequencerIt);
    (*sequencerIt) = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkMRMLNodeSequencer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------
void vtkMRMLNodeSequencer::RegisterNodeSequencer(NodeSequencer* sequencer)
{
  if (!sequencer)
  {
    vtkErrorMacro("vtkMRMLNodeSequencer::RegisterNodeSequencer: sequencer is invalid");
    return;
  }

  // Insert sequencer just before a more generic sequencer
  for (std::list< NodeSequencer* >::iterator sequencerIt = this->NodeSequencers.begin();
    sequencerIt != this->NodeSequencers.end(); ++sequencerIt)
  {
    if (sequencer->IsNodeSupported((*sequencerIt)->GetSupportedNodeClassName()))
    {
      this->NodeSequencers.insert(sequencerIt, sequencer);
      return;
    }
  }

  // default sequencer
  this->NodeSequencers.insert(this->NodeSequencers.end(), sequencer);
}

//-----------------------------------------------------------
vtkMRMLNodeSequencer::NodeSequencer* vtkMRMLNodeSequencer::GetNodeSequencer(vtkMRMLNode* node)
{
  for (std::list< NodeSequencer* >::iterator sequencerIt = this->NodeSequencers.begin();
    sequencerIt != this->NodeSequencers.end(); ++sequencerIt)
  {
    if ((*sequencerIt)->IsNodeSupported(node))
    {
      // found suitable sequencer
      return (*sequencerIt);
    }
  }

  // default sequencer
  return this->NodeSequencers.back();
}


VTK_SINGLETON_CXX(vtkMRMLNodeSequencer);

