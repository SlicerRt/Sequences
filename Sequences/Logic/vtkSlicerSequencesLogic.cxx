/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// Sequence Logic includes
#include "vtkSlicerSequencesLogic.h"

// MRMLSequence includes
#include "vtkMRMLLinearTransformSequenceStorageNode.h"
#include "vtkMRMLSequenceNode.h"
#include "vtkMRMLSequenceStorageNode.h"
#include "vtkMRMLVolumeSequenceStorageNode.h"
#include "vtkMRMLBitStreamSequenceStorageNode.h"

// MRML includes
#include "vtkCacheManager.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtksys/SystemTools.hxx> 

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerSequencesLogic);

//----------------------------------------------------------------------------
vtkSlicerSequencesLogic::vtkSlicerSequencesLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerSequencesLogic::~vtkSlicerSequencesLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerSequencesLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerSequencesLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerSequencesLogic::RegisterNodes()
{
  if (this->GetMRMLScene()==NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLSequenceNode>::New());
  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLSequenceStorageNode>::New());
  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLLinearTransformSequenceStorageNode>::New());
  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLVolumeSequenceStorageNode>::New());
  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLBitStreamSequenceStorageNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerSequencesLogic::UpdateFromMRMLScene()
{
  if (this->GetMRMLScene()==NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
}

//---------------------------------------------------------------------------
void vtkSlicerSequencesLogic
::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
void vtkSlicerSequencesLogic
::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
}

//----------------------------------------------------------------------------
vtkMRMLSequenceNode* vtkSlicerSequencesLogic::AddSequence(const char* filename)
{
  if (this->GetMRMLScene() == 0 || filename == 0)
  {
    return 0;
  }
  vtkNew<vtkMRMLSequenceNode> sequenceNode;
  //vtkNew<vtkMRMLModelDisplayNode> displayNode;
  vtkNew<vtkMRMLSequenceStorageNode> storageNode;

  // check for local or remote files
  int useURI = 0; // false;
  if (this->GetMRMLScene()->GetCacheManager() != NULL)
  {
    useURI = this->GetMRMLScene()->GetCacheManager()->IsRemoteReference(filename);
    vtkDebugMacro("AddModel: file name is remote: " << filename);
  }
  const char *localFile = 0;
  if (useURI)
  {
    storageNode->SetURI(filename);
    // reset filename to the local file name
    localFile = ((this->GetMRMLScene())->GetCacheManager())->GetFilenameFromURI(filename);
  }
  else
  {
    storageNode->SetFileName(filename);
    localFile = filename;
  }
  const std::string fname(localFile ? localFile : "");
  // the sequence name is based on the file name (vtksys call should work even if
  // file is not on disk yet)
  std::string name = vtksys::SystemTools::GetFilenameName(fname);
  vtkDebugMacro("AddModel: got Sequence name = " << name.c_str());

  // check to see which node can read this type of file
  if (storageNode->SupportedFileType(name.c_str()))
  {
    std::string baseName = vtksys::SystemTools::GetFilenameWithoutExtension(fname);
    std::string uname(this->GetMRMLScene()->GetUniqueNameByString(baseName.c_str()));
    sequenceNode->SetName(uname.c_str());

    this->GetMRMLScene()->SaveStateForUndo();

    this->GetMRMLScene()->AddNode(storageNode.GetPointer());

    // Set the scene so that SetAndObserveStorageNodeID can find the node in the scene.
    sequenceNode->SetScene(this->GetMRMLScene());
    sequenceNode->SetAndObserveStorageNodeID(storageNode->GetID());

    this->GetMRMLScene()->AddNode(sequenceNode.GetPointer());

    // now set up the reading
    vtkDebugMacro("AddSequence: calling read on the storage node");
    int retval = storageNode->ReadData(sequenceNode.GetPointer());
    if (retval != 1)
    {
      vtkErrorMacro("AddSequence: error reading " << filename);
      this->GetMRMLScene()->RemoveNode(sequenceNode.GetPointer());
      return 0;
    }
  }
  else
  {
    vtkErrorMacro("Couldn't read file: " << filename);
    return 0;
  }

  return sequenceNode.GetPointer();
}

