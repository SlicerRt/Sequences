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
::AddDataNodeAtValue( vtkMRMLNode* rNode, vtkMRMLNode* dataNode, const char* value )
{
  vtkMRMLHierarchyNode* rootNode = vtkMRMLHierarchyNode::SafeDownCast( rNode );
  if ( ! rootNode || ! dataNode )
  {
    vtkErrorMacro("AddDataNodeAtValue failed: rootNode or dataNode is invalid");
    return;
  }

  // Need to create connector nodes if the timePoint already exists
  vtkMRMLHierarchyNode* sequenceNode = vtkMRMLHierarchyNode::SafeDownCast( GetSequenceNodeAtValue( rootNode, value ) );  
  if ( sequenceNode == NULL )
  {
    sequenceNode = vtkMRMLHierarchyNode::New();
    sequenceNode->AllowMultipleChildrenOn();
    sequenceNode->SetAttribute( "HierarchyType", "MultiDimension" );
    sequenceNode->SetAttribute( "MultiDimension.Value", value );
    sequenceNode->SetScene( this->GetMRMLScene() );
    this->GetMRMLScene()->AddNode( sequenceNode );
    sequenceNode->SetParentNodeID( rootNode->GetID() );    
    std::stringstream sequenceNodeName;
    sequenceNodeName << "Seq_" << rootNode->GetName() << "_" << value;
    sequenceNode->SetName(sequenceNodeName.str().c_str());
  }

  /*
  if ( sequenceNode->GetNumberOfChildrenNodes() == 0 && sequenceNode->GetAssociatedNode() == NULL )
  {
    sequenceNode->SetAssociatedNodeID( dataNode->GetID() );
    return;
  }
  */
  
/*  if ( sequenceNode->GetAssociatedNode() != NULL )
  {
    vtkMRMLHierarchyNode* dataConnectorNode = vtkMRMLHierarchyNode::New();
    dataConnectorNode->AllowMultipleChildrenOff();
    dataConnectorNode->SetAttribute( "HierarchyType", "MultiDimension" );
    dataConnectorNode->SetScene( this->GetMRMLScene() );
    this->GetMRMLScene()->AddNode( dataConnectorNode );
    dataConnectorNode->SetParentNodeID( sequenceNode->GetID() );
    dataConnectorNode->SetAssociatedNodeID( sequenceNode->GetAssociatedNodeID() );
    sequenceNode->SetAssociatedNodeID( "" );
    std::stringstream dataConnectorNodeName;
    dataConnectorNodeName << "Conn_" << rootNode->GetName() << "_" << value << "_" << dataNode->GetName();
    dataConnectorNode->SetName(dataConnectorNodeName.str().c_str());
  }
*/

  // Create a data connector node for the new node
  vtkMRMLHierarchyNode* dataConnectorNode = vtkMRMLHierarchyNode::New();
  dataConnectorNode->AllowMultipleChildrenOff();
  dataConnectorNode->SetAttribute( "HierarchyType", "MultiDimension" );
  dataConnectorNode->SetScene( this->GetMRMLScene() );
  this->GetMRMLScene()->AddNode( dataConnectorNode );
  dataConnectorNode->SetParentNodeID( sequenceNode->GetID() );
  dataConnectorNode->SetAssociatedNodeID( dataNode->GetID() );
  std::stringstream dataConnectorNodeName;
  dataConnectorNodeName << "Conn_" << rootNode->GetName() << "_" << value << "_" << dataNode->GetName();
  dataConnectorNode->SetName(dataConnectorNodeName.str().c_str());
}


//---------------------------------------------------------------------------
void vtkSlicerMultiDimensionLogic
::RemoveSequenceNodeAtValue( vtkMRMLNode* rNode, const char* value )
{
  vtkMRMLHierarchyNode* rootNode = vtkMRMLHierarchyNode::SafeDownCast(rNode);
  if ( ! rootNode )
  {
    return;
  }

  vtkMRMLHierarchyNode* sequenceNode = vtkMRMLHierarchyNode::SafeDownCast( this->GetSequenceNodeAtValue( rootNode, value ) );

  // Delete all the child data connector nodes
  for ( int i = 0; i < sequenceNode->GetNumberOfChildrenNodes(); i++ )
  {
    vtkMRMLNode* dataConnectorNode = sequenceNode->GetNthChildNode( i );
    if ( this->IsDataConnectorNode( dataConnectorNode ) )
    {
      dataConnectorNode->Delete();
    }
  }

  this->GetMRMLScene()->RemoveNode( sequenceNode );
  sequenceNode->SetAssociatedNodeID( "" );
  sequenceNode->SetParentNodeID( "" );
  sequenceNode->Delete();
}


//---------------------------------------------------------------------------
vtkCollection* vtkSlicerMultiDimensionLogic
::GetDataNodesAtValue( vtkMRMLNode* rNode, const char* value )
{
  vtkMRMLHierarchyNode* rootNode = vtkMRMLHierarchyNode::SafeDownCast(rNode);
  if ( ! rootNode )
  {
    return NULL;
  }

  // Return all of the child nodes that are not connector nodes
  vtkMRMLHierarchyNode* sequenceNode = vtkMRMLHierarchyNode::SafeDownCast( this->GetSequenceNodeAtValue( rootNode, value ) );
  vtkCollection* dataNodeCollection = vtkCollection::New();

  if ( sequenceNode == NULL )
  {
    return dataNodeCollection;
  }

  int numChildNodes = sequenceNode->GetNumberOfChildrenNodes();

  if ( numChildNodes == 0 && sequenceNode->GetAssociatedNode() != NULL )
  {
    dataNodeCollection->AddItem( sequenceNode->GetAssociatedNode() );
  }

  for ( int i = 0; i < numChildNodes; i++ )
  {
    vtkMRMLHierarchyNode* dataConnectorNode = vtkMRMLHierarchyNode::SafeDownCast( sequenceNode->GetNthChildNode( i ) ); 
    if ( dataConnectorNode != NULL && this->IsDataConnectorNode( dataConnectorNode ) )
    {
      dataNodeCollection->AddItem( dataConnectorNode->GetAssociatedNode() );
    }
  }

  return dataNodeCollection;
}

//---------------------------------------------------------------------------

// Note: The map should be of the form: < FromValue, ToValue >
void vtkSlicerMultiDimensionLogic
::UpdateValues( vtkMRMLNode* rNode, std::map< std::string, std::string > valueMap )
{
  vtkMRMLHierarchyNode* rootNode = vtkMRMLHierarchyNode::SafeDownCast(rNode);
  if ( ! rootNode )
  {
    return;
  }

  // Iterate through all the children nodes
  for ( int i = 0; i < rootNode->GetNumberOfChildrenNodes(); i++ )
  {
    vtkMRMLHierarchyNode* sequenceNode = vtkMRMLHierarchyNode::SafeDownCast( rootNode->GetNthChildNode( i ) );
    std::string currentValue = sequenceNode->GetAttribute( "MultiDimension.Value" );
    // If the multi-dimension value is valid and appears in the map then map to the new values
    if ( valueMap.find( currentValue ) != valueMap.end() )
    {
      sequenceNode->SetAttribute( "MultiDimension.Value", valueMap[ currentValue ].c_str() );
    }
  }
}


//---------------------------------------------------------------------------
bool vtkSlicerMultiDimensionLogic
::IsDataConnectorNode( vtkMRMLNode* node )
{
  vtkMRMLHierarchyNode* hierarchyNode = vtkMRMLHierarchyNode::SafeDownCast(node);
  if ( ! hierarchyNode )
  {
    return false;
  }

  if ( hierarchyNode->GetNumberOfChildrenNodes() > 0 )
  {
    return false;
  }

  const char* hierarchyType = hierarchyNode->GetAttribute( "HierarchyType" );
  if ( hierarchyType == NULL || strcmp( hierarchyType, "MultiDimension" ) != 0 )
  {
    return false;
  }

  return true;
}


//---------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkSlicerMultiDimensionLogic
::GetSequenceNodeAtValue( vtkMRMLHierarchyNode* rootNode, const char* value )
{  
  if ( rootNode==NULL )
  {
    vtkErrorMacro("GetSequenceNodeAtValue failed: rootNode is invalid");
    return NULL;
  }

  int numberOfChildrenNodes = rootNode->GetNumberOfChildrenNodes();
  for ( int i = 0; i < numberOfChildrenNodes; i++ )
  {
    vtkMRMLHierarchyNode* sequenceNode = rootNode->GetNthChildNode(i);
    // note that this is the sequence node, which does not contain any data, only the value along the dimension
    if ( strcmp( sequenceNode->GetAttribute( "MultiDimension.Value" ), value ) == 0 )
    {
      // found it!
      return sequenceNode;
    }
  }

  return NULL;
}