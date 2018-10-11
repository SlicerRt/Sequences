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
#include <vtkDoubleArray.h>
#include <vtkMRMLCameraNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLSliceCompositeNode.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLSegmentationNode.h>
#include <vtkMRMLTransformNode.h>
#include <vtkMRMLVectorVolumeDisplayNode.h>
#include <vtkMRMLViewNode.h>
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLDoubleArrayNode.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>

// Sequence MRML includes
#include <vtkMRMLSequenceNode.h>

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
  target->CopyWithSingleModifiedEvent(source);
}

vtkMRMLNode* vtkMRMLNodeSequencer::NodeSequencer::DeepCopyNodeToScene(vtkMRMLNode* source, vtkMRMLScene* scene)
{
  if (source == NULL)
  {
    vtkGenericWarningMacro("NodeSequencer::CopyNode failed, invalid node");
    return NULL;
  }
  std::string baseName = "Data";
  if (source->GetAttribute("Sequences.BaseName") != 0)
  {
    baseName = source->GetAttribute("Sequences.BaseName");
  }
  else if (source->GetName() != 0)
  {
    baseName = source->GetName();
  }
  std::string newNodeName = baseName;

  vtkSmartPointer<vtkMRMLNode> target = vtkSmartPointer<vtkMRMLNode>::Take(source->CreateNodeInstance());
  this->CopyNode(source, target, false);

  // Generating unique node names is slow, and makes adding many nodes to a sequence too slow
  // We will instead ensure that all file names for storable nodes are unique when saving
  target->SetName(newNodeName.c_str());
  target->SetAttribute("Sequences.BaseName", baseName.c_str());

  vtkMRMLNode* addedTargetNode = scene->AddNode(target);
  return addedTargetNode;
}

void vtkMRMLNodeSequencer::NodeSequencer::AddDefaultDisplayNodes(vtkMRMLNode* node)
{
  vtkMRMLDisplayableNode* displayableNode = vtkMRMLDisplayableNode::SafeDownCast(node);
  if (displayableNode == NULL)
  {
    // not a displayable node, there is nothing to do
    return;
  }
  displayableNode->CreateDefaultDisplayNodes();
}

void vtkMRMLNodeSequencer::NodeSequencer::AddDefaultSequenceStorageNode(vtkMRMLSequenceNode* node)
{
  if (node == NULL)
  {
    // no node, there is nothing to do
    return;
  }
  node->AddDefaultStorageNode();
}

//----------------------------------------------------------------------------

class ScalarVolumeNodeSequencer : public vtkMRMLNodeSequencer::NodeSequencer
{
public:
  ScalarVolumeNodeSequencer()
  {
    this->RecordingEvents->InsertNextValue(vtkMRMLVolumeNode::ImageDataModifiedEvent);
    this->SupportedNodeClassName = "vtkMRMLScalarVolumeNode";
    this->SupportedNodeParentClassNames.push_back("vtkMRMLVolumeNode");
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
    if (!shallowCopy && targetImageData.GetPointer()!=NULL)
    {
      targetImageData = vtkSmartPointer<vtkImageData>::Take(sourceVolumeNode->GetImageData()->NewInstance());
      targetImageData->DeepCopy(sourceVolumeNode->GetImageData());
    }
    targetVolumeNode->SetAndObserveImageData(targetImageData); // invokes vtkMRMLVolumeNode::ImageDataModifiedEvent, which is not masked by StartModify
    target->EndModify(oldModified);
  }

  virtual void AddDefaultDisplayNodes(vtkMRMLNode* node)
  {
    vtkMRMLVolumeNode* displayableNode = vtkMRMLVolumeNode::SafeDownCast(node);
    if (displayableNode == NULL)
    {
      // not a displayable node, there is nothing to do
      return;
    }
    if (displayableNode->GetDisplayNode())
    {
      // there is a display node already
      return;
    }
    displayableNode->CreateDefaultDisplayNodes();

    // Turn off auto window/level for scalar volumes (it is costly to compute recommended ww/wl and image would appear to be flickering)
    vtkMRMLScalarVolumeDisplayNode* scalarVolumeDisplayNode = vtkMRMLScalarVolumeDisplayNode::SafeDownCast(displayableNode->GetDisplayNode());
    if (scalarVolumeDisplayNode)
    {
      scalarVolumeDisplayNode->AutoWindowLevelOff();
    }
  }

};

//----------------------------------------------------------------------------

class VectorVolumeNodeSequencer : public vtkMRMLNodeSequencer::NodeSequencer
{
public:
  VectorVolumeNodeSequencer()
  {
    this->RecordingEvents->InsertNextValue(vtkMRMLVolumeNode::ImageDataModifiedEvent);
    this->SupportedNodeClassName = "vtkMRMLVectorVolumeNode";
    this->SupportedNodeParentClassNames.push_back("vtkMRMLDisplayableNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLVolumeNode");
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
    if (!shallowCopy && targetImageData.GetPointer() != NULL)
    {
      targetImageData = vtkSmartPointer<vtkImageData>::Take(sourceVolumeNode->GetImageData()->NewInstance());
      targetImageData->DeepCopy(sourceVolumeNode->GetImageData());
    }
    targetVolumeNode->SetAndObserveImageData(targetImageData); // invokes vtkMRMLVolumeNode::ImageDataModifiedEvent, which is not masked by StartModify
    target->EndModify(oldModified);
  }

  virtual void AddDefaultDisplayNodes(vtkMRMLNode* node)
  {
    vtkMRMLVolumeNode* displayableNode = vtkMRMLVolumeNode::SafeDownCast(node);
    if (displayableNode == NULL)
    {
      // not a displayable node, there is nothing to do
      return;
    }
    if (displayableNode->GetDisplayNode())
    {
      // there is a display node already
      return;
    }
    displayableNode->CreateDefaultDisplayNodes();

    // Turn off auto window/level for scalar volumes (it is costly to compute recommended ww/wl and image would appear to be flickering)
    vtkMRMLVectorVolumeDisplayNode* vectorVolumeDisplayNode = vtkMRMLVectorVolumeDisplayNode::SafeDownCast(displayableNode->GetDisplayNode());
    if (vectorVolumeDisplayNode)
    {
      vectorVolumeDisplayNode->AutoWindowLevelOff();
    }
  }

};

//----------------------------------------------------------------------------

class SegmentationNodeSequencer : public vtkMRMLNodeSequencer::NodeSequencer
{
public:
  SegmentationNodeSequencer()
  {
    this->RecordingEvents->InsertNextValue(vtkSegmentation::MasterRepresentationModified);
    this->SupportedNodeClassName = "vtkMRMLSegmentationNode";
    this->SupportedNodeParentClassNames.push_back("vtkMRMLDisplayableNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLTransformableNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLStorableNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLNode");
  }

  virtual void CopyNode(vtkMRMLNode* source, vtkMRMLNode* target, bool shallowCopy /* =false */)
  {
    int oldModified = target->StartModify();
    vtkMRMLSegmentationNode* targetSegmentationNode = vtkMRMLSegmentationNode::SafeDownCast(target);
    vtkMRMLSegmentationNode* sourceSegmentationNode = vtkMRMLSegmentationNode::SafeDownCast(source);
    // targetScalarVolumeNode->SetAndObserveTransformNodeID is not called, as we want to keep the currently applied transform
    vtkSmartPointer<vtkSegmentation> targetSegmentation = sourceSegmentationNode->GetSegmentation();
    if (!shallowCopy && targetSegmentation.GetPointer() != NULL)
    {
      targetSegmentation = vtkSmartPointer<vtkSegmentation>::Take(sourceSegmentationNode->GetSegmentation()->NewInstance());
      targetSegmentation->DeepCopy(sourceSegmentationNode->GetSegmentation());
    }
    targetSegmentationNode->SetAndObserveSegmentation(targetSegmentation);
    target->EndModify(oldModified);
  }

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
    vtkAbstractTransform* sourceTransform;
    bool setAsTransformToParent = vtkMRMLTransformNode::IsAbstractTransformComputedFromInverse(sourceTransformNode->GetTransformFromParent());
    if (setAsTransformToParent)
    {
      sourceTransform = sourceTransformNode->GetTransformToParent();
    }
    else
    {
      sourceTransform = sourceTransformNode->GetTransformFromParent();
    }
    vtkSmartPointer<vtkAbstractTransform> targetTransform;
    if (shallowCopy || sourceTransform == NULL)
    {
      targetTransform = sourceTransform;
    }
    else
    {
      targetTransform = vtkSmartPointer<vtkAbstractTransform>::Take(sourceTransform->NewInstance());
      // vtkAbstractTransform's DeepCopy does not do a full deep copy, therefore
      // we need to use vtkMRMLTransformNode's utility method instead.
      vtkMRMLTransformNode::DeepCopyTransform(targetTransform, sourceTransform);
    }
    if (setAsTransformToParent)
    {
      targetTransformNode->SetAndObserveTransformToParent(targetTransform);
    }
    else
    {
      targetTransformNode->SetAndObserveTransformFromParent(targetTransform);
    }
    target->EndModify(oldModified);
  }

  virtual void AddDefaultDisplayNodes(vtkMRMLNode* vtkNotUsed(node))
  {
    // don't create display nodes for transforms by default
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
    if (!shallowCopy && targetPolyData.GetPointer()!=NULL)
    {
      targetPolyData = vtkSmartPointer<vtkPolyData>::Take(sourceModelNode->GetPolyData()->NewInstance());
      targetPolyData->DeepCopy(sourceModelNode->GetPolyData());
    }
    targetModelNode->SetAndObservePolyData(targetPolyData);
    target->EndModify(oldModified);
  }

};

//----------------------------------------------------------------------------

class SliceCompositeNodeSequencer : public vtkMRMLNodeSequencer::NodeSequencer
{
public:
  SliceCompositeNodeSequencer()
  {
    this->SupportedNodeClassName = "vtkMRMLSliceCompositeNode";
    this->SupportedNodeParentClassNames.push_back("vtkMRMLNode");
  }
  virtual void CopyNode(vtkMRMLNode* source, vtkMRMLNode* target, bool vtkNotUsed(shallowCopy) /* =false */)
  {
    // Singleton node, therefore we cannot use the default Copy() method
    vtkMRMLSliceCompositeNode* targetNode = vtkMRMLSliceCompositeNode::SafeDownCast(target);
    vtkMRMLSliceCompositeNode* sourceNode = vtkMRMLSliceCompositeNode::SafeDownCast(source);
    int disabledModify = targetNode->StartModify();
    targetNode->SetBackgroundVolumeID(sourceNode->GetBackgroundVolumeID());
    targetNode->SetForegroundVolumeID(sourceNode->GetForegroundVolumeID());
    targetNode->SetLabelVolumeID(sourceNode->GetLabelVolumeID());
    targetNode->SetCompositing(sourceNode->GetCompositing());
    targetNode->SetForegroundOpacity(sourceNode->GetForegroundOpacity());
    targetNode->SetLabelOpacity(sourceNode->GetLabelOpacity());
    targetNode->SetLinkedControl(sourceNode->GetLinkedControl());
    targetNode->SetHotLinkedControl(sourceNode->GetHotLinkedControl());
    targetNode->SetDoPropagateVolumeSelection(sourceNode->GetDoPropagateVolumeSelection());
    targetNode->EndModify(disabledModify);
  }
};

//----------------------------------------------------------------------------

class DisplayNodeSequencer : public vtkMRMLNodeSequencer::NodeSequencer
{
public:
  DisplayNodeSequencer()
  {
    this->SupportedNodeClassName = "vtkMRMLDisplayNode";
    this->SupportedNodeParentClassNames.push_back("vtkMRMLNode");
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

  virtual void CopyNode(vtkMRMLNode* source, vtkMRMLNode* target, bool vtkNotUsed(shallowCopy) /* =false */)
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

  virtual void CopyNode(vtkMRMLNode* source, vtkMRMLNode* target, bool vtkNotUsed(shallowCopy) /* =false */)
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

class SliceNodeSequencer : public vtkMRMLNodeSequencer::NodeSequencer
{
public:
  SliceNodeSequencer()
  {
    this->SupportedNodeClassName = "vtkMRMLSliceNode";
    this->SupportedNodeParentClassNames.push_back("vtkMRMLAbstractViewNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLNode");
  }

  virtual void CopyNode(vtkMRMLNode* source, vtkMRMLNode* target, bool vtkNotUsed(shallowCopy) /* =false */)
  {
    // Singleton node, therefore we cannot use the default Copy() method
    vtkMRMLSliceNode* targetSliceNode = vtkMRMLSliceNode::SafeDownCast(target);
    vtkMRMLSliceNode* sourceSliceNode = vtkMRMLSliceNode::SafeDownCast(source);
    int disabledModify = targetSliceNode->StartModify();
    targetSliceNode->SetSliceVisible(sourceSliceNode->GetSliceVisible());
    targetSliceNode->GetSliceToRAS()->DeepCopy(sourceSliceNode->GetSliceToRAS());
    double* doubleVector = sourceSliceNode->GetFieldOfView();
    targetSliceNode->SetFieldOfView(doubleVector[0], doubleVector[1], doubleVector[2]);
    int* intVector = sourceSliceNode->GetDimensions();
    targetSliceNode->SetDimensions(intVector[0], intVector[1], intVector[2]);
    doubleVector = sourceSliceNode->GetXYZOrigin();
    targetSliceNode->SetXYZOrigin(doubleVector[0], doubleVector[1], doubleVector[2]);
    intVector = sourceSliceNode->GetUVWDimensions();
    targetSliceNode->SetUVWDimensions(intVector[0], intVector[1], intVector[2]);
    intVector = sourceSliceNode->GetUVWMaximumDimensions();
    targetSliceNode->SetUVWMaximumDimensions(intVector[0], intVector[1], intVector[2]);
    doubleVector = sourceSliceNode->GetUVWExtents();
    targetSliceNode->SetUVWExtents(doubleVector[0], doubleVector[1], doubleVector[2]);
    doubleVector = sourceSliceNode->GetUVWOrigin();
    targetSliceNode->SetUVWOrigin(doubleVector[0], doubleVector[1], doubleVector[2]);
    doubleVector = sourceSliceNode->GetPrescribedSliceSpacing();
    targetSliceNode->SetPrescribedSliceSpacing(doubleVector[0], doubleVector[1], doubleVector[2]);
    targetSliceNode->UpdateMatrices();
    targetSliceNode->EndModify(disabledModify);
  }
};

//----------------------------------------------------------------------------

class ViewNodeSequencer : public vtkMRMLNodeSequencer::NodeSequencer
{
public:
  ViewNodeSequencer()
  {
    this->SupportedNodeClassName = "vtkMRMLViewNode";
    this->SupportedNodeParentClassNames.push_back("vtkMRMLAbstractViewNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLNode");
  }

  virtual void CopyNode(vtkMRMLNode* source, vtkMRMLNode* target, bool vtkNotUsed(shallowCopy) /* =false */)
  {
    vtkMRMLViewNode* targetNode = vtkMRMLViewNode::SafeDownCast(target);
    vtkMRMLViewNode* sourceNode = vtkMRMLViewNode::SafeDownCast(source);
    int disabledModify = targetNode->StartModify();
    targetNode->SetBoxVisible(sourceNode->GetBoxVisible());
    targetNode->SetAxisLabelsVisible(sourceNode->GetAxisLabelsVisible());
    targetNode->SetAxisLabelsCameraDependent(sourceNode->GetAxisLabelsCameraDependent());
    targetNode->SetLetterSize(sourceNode->GetLetterSize());
    targetNode->SetStereoType(sourceNode->GetStereoType());
    targetNode->SetRenderMode(sourceNode->GetRenderMode());
    targetNode->SetUseDepthPeeling(sourceNode->GetUseDepthPeeling());
    targetNode->SetFPSVisible(sourceNode->GetFPSVisible());
    targetNode->EndModify(disabledModify);
  }
};

//----------------------------------------------------------------------------

class DoubleArrayNodeSequencer : public vtkMRMLNodeSequencer::NodeSequencer
{
public:
  DoubleArrayNodeSequencer()
  {
    this->SupportedNodeClassName = "vtkMRMLDoubleArrayNode";
    this->SupportedNodeParentClassNames.push_back("vtkMRMLStorableNode");
    this->SupportedNodeParentClassNames.push_back("vtkMRMLNode");
  }

  virtual void CopyNode(vtkMRMLNode* source, vtkMRMLNode* target, bool shallowCopy /* =false */)
  {
    int oldModified = target->StartModify();
    vtkMRMLDoubleArrayNode* targetDoubleArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast(target);
    vtkMRMLDoubleArrayNode* sourceDoubleArrayNode = vtkMRMLDoubleArrayNode::SafeDownCast(source);
    targetDoubleArrayNode->CopyWithoutModifiedEvent(sourceDoubleArrayNode); // Copies the attributes, etc.
    vtkDoubleArray* targetDoubleArray = sourceDoubleArrayNode->GetArray();
    if (!shallowCopy && targetDoubleArray!=NULL)
    {
      targetDoubleArray = sourceDoubleArrayNode->GetArray()->NewInstance();
      targetDoubleArray->DeepCopy(sourceDoubleArrayNode->GetArray());
    }
    targetDoubleArrayNode->SetArray(targetDoubleArray);
    target->EndModify(oldModified);
  }

};

//----------------------------------------------------------------------------
#if VTK_MAJOR_VERSION <= 7 || (VTK_MAJOR_VERSION <= 8 && VTK_MINOR_VERSION <= 1)
  // Needed when we don't use the vtkStandardNewMacro.
  vtkInstantiatorNewMacro(vtkMRMLNodeSequencer);
#endif

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
  this->RegisterNodeSequencer(new DisplayNodeSequencer());
  this->RegisterNodeSequencer(new ScalarVolumeNodeSequencer());
  this->RegisterNodeSequencer(new TransformNodeSequencer());
  this->RegisterNodeSequencer(new ModelNodeSequencer());
  this->RegisterNodeSequencer(new CameraNodeSequencer());
  this->RegisterNodeSequencer(new SegmentationNodeSequencer());
  this->RegisterNodeSequencer(new SliceNodeSequencer());
  this->RegisterNodeSequencer(new SliceCompositeNodeSequencer());
  this->RegisterNodeSequencer(new ViewNodeSequencer());
  this->RegisterNodeSequencer(new MarkupsFiducialNodeSequencer());
  this->RegisterNodeSequencer(new DoubleArrayNodeSequencer());
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
  bool inserted = false;
  for (std::list< NodeSequencer* >::iterator sequencerIt = this->NodeSequencers.begin();
    sequencerIt != this->NodeSequencers.end(); ++sequencerIt)
  {
    if (sequencer->IsNodeSupported((*sequencerIt)->GetSupportedNodeClassName()))
    {
      this->NodeSequencers.insert(sequencerIt, sequencer);
      inserted = true;
      break;
    }
  }
  if (!inserted)
  {
    // it's the most generic sequencer, insert at the end
    this->NodeSequencers.insert(this->NodeSequencers.end(), sequencer);
  }

  // Update cached SupportedNodeClassNames list
  this->SupportedNodeClassNames.clear();
  for (std::list< NodeSequencer* >::iterator sequencerIt = this->NodeSequencers.begin();
    sequencerIt != this->NodeSequencers.end(); ++sequencerIt)
  {
    if ((*sequencerIt)->GetSupportedNodeClassName() == "vtkMRMLNode")
    {
      // We want to make the list usable for filtering supported node types
      // therefore we don't include the generic vtkMRMLNode type.
      continue;
    }
    this->SupportedNodeClassNames.push_back((*sequencerIt)->GetSupportedNodeClassName());
  }
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

//-----------------------------------------------------------
const std::vector< std::string > & vtkMRMLNodeSequencer::GetSupportedNodeClassNames()
{
  return this->SupportedNodeClassNames;
}

VTK_SINGLETON_CXX(vtkMRMLNodeSequencer);
