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
  //rootNode->SetName( "MultiDimensionHierarchy" );

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
  //rootNode->SetName( "MultiDimensionHierarchy" );

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
::AddChildNodeAtTimePoint(vtkMRMLNode* rNode, vtkMRMLNode* cNode, char* timePoint)
{
  vtkMRMLHierarchyNode* rootNode = vtkMRMLHierarchyNode::SafeDownCast(rNode);
  vtkMRMLNode* childNode = cNode;

  if (!rootNode || !childNode)
  {
    return;
  }

  // Need to create connector nodes if the timePoint already exists
  vtkMRMLHierarchyNode* childHierarchyNode = vtkMRMLHierarchyNode::SafeDownCast( this->FindChildNodeAtTimePoint( rootNode, timePoint ) );

  if ( childHierarchyNode == NULL )
  {
    childHierarchyNode = vtkMRMLHierarchyNode::New();
    childHierarchyNode->AllowMultipleChildrenOn();
    childHierarchyNode->SetAttribute("HierarchyType", "MultiDimension");
    childHierarchyNode->SetAttribute("MultiDimension.Value", timePoint);
    childHierarchyNode->SetParentNodeID( rootNode->GetID() );
    childHierarchyNode->SetScene( this->GetMRMLScene() );
    this->GetMRMLScene()->AddNode( childHierarchyNode );
  }

  if ( childHierarchyNode->GetNumberOfChildrenNodes() == 0 && childHierarchyNode->GetAssociatedNode() == NULL )
  {
    childHierarchyNode->SetAssociatedNodeID( childNode->GetID() );
    return;
  }
  
  if ( childHierarchyNode->GetAssociatedNode() != NULL )
  {
    vtkMRMLHierarchyNode* dataConnectorNode = vtkMRMLHierarchyNode::New();
    dataConnectorNode->AllowMultipleChildrenOff();
    dataConnectorNode->SetAttribute("HierarchyType", "MultiDimension");
    dataConnectorNode->SetParentNodeID( childHierarchyNode->GetID() );
    dataConnectorNode->SetAssociatedNodeID( childHierarchyNode->GetAssociatedNodeID() );
    childHierarchyNode->SetAssociatedNodeID( "" );
    // this->GetMRMLScene()->AddNode( dataConnectorNode ); // Do not add the data connector nodes to scene
  }

  // Create a data connector node for the new node
  vtkMRMLHierarchyNode* dataConnectorNode = vtkMRMLHierarchyNode::New();
  dataConnectorNode->AllowMultipleChildrenOff();
  dataConnectorNode->SetAttribute("HierarchyType", "MultiDimension");
  dataConnectorNode->SetParentNodeID( childHierarchyNode->GetID() );
  dataConnectorNode->SetAssociatedNodeID( childNode->GetID() );
  // this->GetMRMLScene()->AddNode( dataConnectorNode ); // Do not add the data connector nodes to scene

}


//---------------------------------------------------------------------------
void vtkSlicerMultiDimensionLogic
::RemoveChildNodeAtTimePoint(vtkMRMLNode* rNode, char* timePoint)
{
  vtkMRMLHierarchyNode* rootNode = vtkMRMLHierarchyNode::SafeDownCast(rNode);
  if (!rootNode)
  {
    return;
  }

  vtkMRMLHierarchyNode* childHierarchyNode = vtkMRMLHierarchyNode::SafeDownCast( this->FindChildNodeAtTimePoint( rootNode, timePoint ) );

  // Delete all the child data connector nodes
  for ( int i = 0; i < childHierarchyNode->GetNumberOfChildrenNodes(); i++ )
  {
    vtkMRMLNode* currentChildNode = childHierarchyNode->GetNthChildNode( i );
    if ( this->IsDataConnectorNode( currentChildNode ) )
    {
      currentChildNode->Delete();
    }
  }

  this->GetMRMLScene()->RemoveNode( childHierarchyNode );
  childHierarchyNode->SetAssociatedNodeID( "" );
  childHierarchyNode->SetParentNodeID( "" );
  childHierarchyNode->Delete();
}


//---------------------------------------------------------------------------
vtkCollection* vtkSlicerMultiDimensionLogic
::GetChildNodesAtTimePoint(vtkMRMLNode* rNode, char* timePoint)
{
  vtkMRMLHierarchyNode* rootNode = vtkMRMLHierarchyNode::SafeDownCast(rNode);
  if (!rootNode)
  {
    return NULL;
  }

  // Return all of the child nodes that are not connector nodes
  vtkMRMLHierarchyNode* childHierarchyNode = vtkMRMLHierarchyNode::SafeDownCast( this->FindChildNodeAtTimePoint( rootNode, timePoint ) );
  vtkCollection* childCollection = vtkCollection::New();

  for ( int i = 0; i < childHierarchyNode->GetNumberOfChildrenNodes(); i++ )
  {
    vtkMRMLNode* currentChildNode = childHierarchyNode->GetNthChildNode( i );
    if ( this->IsDataConnectorNode( currentChildNode ) )
    {
      vtkMRMLHierarchyNode* dataConnectorNode = vtkMRMLHierarchyNode::SafeDownCast( currentChildNode );
      childCollection->AddItem( dataConnectorNode->GetAssociatedNode() );
    }
    else
    {
      childCollection->AddItem( currentChildNode );
    }
  }

  return childCollection;
}

//---------------------------------------------------------------------------
int vtkSlicerMultiDimensionLogic
::GetNumberOfChildrenNodes(vtkMRMLNode* rNode)
{
  vtkMRMLHierarchyNode* rootNode = vtkMRMLHierarchyNode::SafeDownCast(rNode);
  if (!rootNode)
  {
    return NULL;
  }
  
  return rootNode->GetNumberOfChildrenNodes();
}

//---------------------------------------------------------------------------

// Note: The map should be of the form: < FromValue, ToValue >
void vtkSlicerMultiDimensionLogic
::UpdateValues( vtkMRMLNode* rNode, std::map< std::string, std::string > valueMap )
{
  vtkMRMLHierarchyNode* rootNode = vtkMRMLHierarchyNode::SafeDownCast(rNode);
  if (!rootNode)
  {
    return;
  }

  // Iterate through all the children nodes
  for ( int i = 0; i < rootNode->GetNumberOfChildrenNodes(); i++ )
  {
    vtkMRMLHierarchyNode* childHierarchyNode = vtkMRMLHierarchyNode::SafeDownCast( rootNode->GetNthChildNode( i ) );
    std::string value = childHierarchyNode->GetAttribute( "MultiDimension.Value" );
    // If the multi-dimension value is valid and appears in the map then map to the new values
    if ( valueMap.find( value ) != valueMap.end() )
    {
      childHierarchyNode->SetAttribute( "MultiDimension.Value", valueMap[ value ].c_str() );
    }
  }

}


//---------------------------------------------------------------------------
bool vtkSlicerMultiDimensionLogic
::IsDataConnectorNode(vtkMRMLNode* node)
{
  vtkMRMLHierarchyNode* hierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(node);
  if (!hierarchyNode)
  {
    return false;
  }

  if ( hierarchyNode->GetNumberOfChildrenNodes() > 0 )
  {
    return false;
  }

  const char* hierarchyType = hierarchyNode->GetAttribute("HierarchyType");
  if (hierarchyType == NULL || strcmp(hierarchyType, "MultiDimension") != 0)
  {
    return false;
  }

  return true;
}


//---------------------------------------------------------------------------
vtkMRMLNode* vtkSlicerMultiDimensionLogic
::FindChildNodeAtTimePoint( vtkMRMLNode* rNode, char* timePoint )
{
  vtkMRMLHierarchyNode* rootNode = vtkMRMLHierarchyNode::SafeDownCast(rNode);
  if (!rootNode)
  {
    return NULL;
  }

  int numberOfChildrenNodes = rootNode->GetNumberOfChildrenNodes();
  for (int i=0; i<numberOfChildrenNodes; i++)
  {
    vtkMRMLHierarchyNode* childHierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(rootNode->GetNthChildNode(i));
    // Observe that this is the "TimeStamp" hierarchy node, and does not contain any data
    if (strcmp(childHierarchyNode->GetAttribute("MultiDimension.Value"), timePoint) == 0)
    {
      return childHierarchyNode;
    }
  }

  return NULL;
}