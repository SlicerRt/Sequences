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

// MultiDimension Logic includes
#include "vtkSlicerMultiDimensionLogic.h"

// MRML includes
#include "vtkMRMLHierarchyNode.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkNew.h>

// STD includes
#include <cassert>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerMultiDimensionLogic);

//----------------------------------------------------------------------------
vtkSlicerMultiDimensionLogic::vtkSlicerMultiDimensionLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerMultiDimensionLogic::~vtkSlicerMultiDimensionLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerMultiDimensionLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerMultiDimensionLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerMultiDimensionLogic::RegisterNodes()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerMultiDimensionLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerMultiDimensionLogic
::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
void vtkSlicerMultiDimensionLogic
::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
vtkMRMLNode* vtkSlicerMultiDimensionLogic
::CreateMultiDimensionRootNode()
{
  vtkMRMLHierarchyNode* rootNode = vtkMRMLHierarchyNode::New();
  rootNode->AllowMultipleChildrenOn();
  rootNode->SetAttribute("HierarchyType", "MultiDimension");
  rootNode->SetAttribute("MultiDimension.Name", "Time");
  rootNode->SetAttribute("MultiDimension.Unit", "Sec");

  this->GetMRMLScene()->AddNode(rootNode);

  return rootNode;
}

//---------------------------------------------------------------------------
vtkMRMLNode* vtkSlicerMultiDimensionLogic
::SetMultiDimensionRootNode(vtkMRMLNode* node)
{
  vtkMRMLHierarchyNode* rootNode = vtkMRMLHierarchyNode::SafeDownCast(node);
  if (!rootNode)
  {
    return NULL;
  }

  rootNode->AllowMultipleChildrenOn();
  rootNode->SetAttribute("HierarchyType", "MultiDimension");
  rootNode->SetAttribute("MultiDimension.Name", "Time");
  rootNode->SetAttribute("MultiDimension.Unit", "Sec");

  if (!this->GetMRMLScene()->GetNodeByID(rootNode->GetID()))
  {
    this->GetMRMLScene()->AddNode(rootNode);
  }

  return rootNode;
}

//---------------------------------------------------------------------------
void vtkSlicerMultiDimensionLogic
::DeleteMultiDimensionRootNode(vtkMRMLNode* node)
{
  vtkMRMLHierarchyNode* rootNode = vtkMRMLHierarchyNode::SafeDownCast(node);
  if (!rootNode)
  {
    return;
  }
  const char* hierarchyType = rootNode->GetAttribute("HierarchyType");
  if (hierarchyType == NULL || strcmp(hierarchyType, "MultiDimension") != 0)
  {
    return;
  }
  this->GetMRMLScene()->RemoveNode(rootNode);
  rootNode->Delete();
}

//---------------------------------------------------------------------------
void vtkSlicerMultiDimensionLogic
::AddChildVolumeNodeAtTimePoint(vtkMRMLNode* rNode, vtkMRMLNode* cNode, char* timePoint)
{
  vtkMRMLHierarchyNode* rootNode = vtkMRMLHierarchyNode::SafeDownCast(rNode);
  vtkMRMLScalarVolumeNode* childNode = vtkMRMLScalarVolumeNode::SafeDownCast(cNode);
  if (!rootNode || !childNode)
  {
    return;
  }
  vtkMRMLHierarchyNode* chidlHierarchyNode = vtkMRMLHierarchyNode::New();
  chidlHierarchyNode->AllowMultipleChildrenOff();
  chidlHierarchyNode->SetAttribute("HierarchyType", "MultiDimension");
  chidlHierarchyNode->SetAttribute("MultiDimension.Name", "Time");
  chidlHierarchyNode->SetAttribute("MultiDimension.Unit", "Sec");
  chidlHierarchyNode->SetAttribute("MultiDimension.Value", timePoint);

  chidlHierarchyNode->SetParentNodeID(rootNode->GetID());
  chidlHierarchyNode->SetAssociatedNodeID(childNode->GetID());

  this->GetMRMLScene()->AddNode(chidlHierarchyNode);
}

//---------------------------------------------------------------------------
void vtkSlicerMultiDimensionLogic
::RemoveChildVolumeNodeAtTimePoint(vtkMRMLNode* rNode, char* timePoint)
{
  vtkMRMLHierarchyNode* rootNode = vtkMRMLHierarchyNode::SafeDownCast(rNode);
  if (!rootNode)
  {
    return;
  }
  vtkMRMLHierarchyNode* childHierarchyNode = NULL;
  vtkMRMLScalarVolumeNode* volumeNode = NULL;
  int numberOfChildrenNodes = rootNode->GetNumberOfChildrenNodes();
  for (int i=0; i<numberOfChildrenNodes; i++)
  {
    childHierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(rootNode->GetNthChildNode(i));
    volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(childHierarchyNode->GetAssociatedNode());
    if (strcmp(volumeNode->GetAttribute("MultiDimension.Value"), timePoint) == 0)
    {
      break;
    }
  }
  childHierarchyNode->SetAssociatedNodeID("");
  childHierarchyNode->SetParentNodeID("");
  childHierarchyNode->Delete();
}

//---------------------------------------------------------------------------
vtkMRMLNode* vtkSlicerMultiDimensionLogic
::GetChildVolumeNodeAtTimePoint(vtkMRMLNode* rNode, char* timePoint)
{
  vtkMRMLHierarchyNode* rootNode = vtkMRMLHierarchyNode::SafeDownCast(rNode);
  if (!rootNode)
  {
    return NULL;
  }
  vtkMRMLHierarchyNode* childHierarchyNode = NULL;
  vtkMRMLScalarVolumeNode* volumeNode = NULL;
  int numberOfChildrenNodes = rootNode->GetNumberOfChildrenNodes();
  for (int i=0; i<numberOfChildrenNodes; i++)
  {
    childHierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(rootNode->GetNthChildNode(i));
    volumeNode = vtkMRMLScalarVolumeNode::SafeDownCast(childHierarchyNode->GetAssociatedNode());
    if (strcmp(volumeNode->GetAttribute("MultiDimension.Value"), timePoint) == 0)
    {
      break;
    }
  }
  return volumeNode;
}
