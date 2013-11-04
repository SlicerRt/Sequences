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

// MultidimData Logic includes
#include "vtkSlicerMultidimDataLogic.h"

// MRML includes
#include "vtkMRMLHierarchyNode.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkNew.h>

#define SAFE_CHAR_POINTER(unsafeString) ( unsafeString==NULL?"":unsafeString )

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerMultidimDataLogic);

//----------------------------------------------------------------------------
vtkSlicerMultidimDataLogic::vtkSlicerMultidimDataLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerMultidimDataLogic::~vtkSlicerMultidimDataLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerMultidimDataLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerMultidimDataLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerMultidimDataLogic::RegisterNodes()
{
  if (this->GetMRMLScene()==NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
}

//---------------------------------------------------------------------------
void vtkSlicerMultidimDataLogic::UpdateFromMRMLScene()
{
  if (this->GetMRMLScene()==NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
}

//---------------------------------------------------------------------------
void vtkSlicerMultidimDataLogic
::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
void vtkSlicerMultidimDataLogic
::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkSlicerMultidimDataLogic
::CreateMultidimDataRootNode()
{
  vtkSmartPointer<vtkMRMLHierarchyNode> rootNode = vtkSmartPointer<vtkMRMLHierarchyNode>::New();
  rootNode->AllowMultipleChildrenOn();
  rootNode->SetAttribute("HierarchyType", "MultidimData");
  rootNode->SetAttribute("MultidimData.NodeType", "Root");
  rootNode->SetAttribute("MultidimData.Name", "[Undefined]");
  rootNode->SetAttribute("MultidimData.Unit", "[Undefined]");
  this->GetMRMLScene()->AddNode(rootNode);
  rootNode->SetName("MultidimDataHierarchy");

  return rootNode;
}

//---------------------------------------------------------------------------
void vtkSlicerMultidimDataLogic
::AddDataNodeAtValue(vtkMRMLHierarchyNode* rootNode, vtkMRMLNode* dataNode, const char* parameterValue)
{
  if (rootNode==NULL || dataNode==NULL || parameterValue==NULL)
  {
    vtkErrorMacro("AddDataNodeAtValue failed: rootNode, dataNode, or parameterValue is invalid");
    return;
  }
 
  // This name is appended to the end of the sequence and node names, for example " (time=23sec)"
  std::string sequenceNamePostfix=std::string(" (")
    +SAFE_CHAR_POINTER(rootNode->GetAttribute("MultidimData.Name"))+"="+parameterValue
    +SAFE_CHAR_POINTER(rootNode->GetAttribute("MultidimData.Unit"))+")";

  // Need to create connector nodes if the timePoint already exists
  vtkMRMLHierarchyNode* sequenceNode = this->CreateSequenceNodeAtValue( rootNode, parameterValue );  

  std::string dataNodeName=std::string(rootNode->GetName())+"/"+dataNode->GetName()+sequenceNamePostfix;

  // Create a data connector node for the new node
  vtkSmartPointer<vtkMRMLHierarchyNode> dataConnectorNode = vtkSmartPointer<vtkMRMLHierarchyNode>::New();
  dataConnectorNode->AllowMultipleChildrenOff();
  dataConnectorNode->SetAttribute( "HierarchyType", "MultidimData" );
  dataConnectorNode->SetAttribute( "MultidimData.NodeType", "DataConnector");
  dataConnectorNode->SetAttribute( "MultidimData.DataRole", dataNode->GetName() );
  dataConnectorNode->SetHideFromEditors(true);
  this->GetMRMLScene()->AddNode( dataConnectorNode );
  dataConnectorNode->SetScene( this->GetMRMLScene() );
  dataConnectorNode->SetParentNodeID( sequenceNode->GetID() );
  std::string dataConnectorNodeName=dataNodeName+" connector";
  dataConnectorNode->SetName(dataConnectorNodeName.c_str());

  // Update data node name (from "Image" it generates "Sequence1/Image (time=23sec)") and add to the scene
  
  dataNode->SetName( dataNodeName.c_str() );
  dataNode->SetHideFromEditors(true);
  if ( this->GetMRMLScene()->IsNodePresent( dataNode ) == 0 ) // Note: return value is position - 1 // TODO: This may not be optimal
  {
    this->GetMRMLScene()->AddNode( dataNode );
    dataNode->SetScene( this->GetMRMLScene() );
  }
  dataConnectorNode->SetAssociatedNodeID( dataNode->GetID() );
}


//---------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkSlicerMultidimDataLogic
::CreateSequenceNodeAtValue(vtkMRMLHierarchyNode* rootNode, const char* parameterValue)
{
  if ( rootNode == NULL || parameterValue==NULL)
  {
    vtkErrorMacro("CreateSequenceNodeAtValue failed: rootNode or parameterValue is invalid");
    return NULL;
  }
 
  // This name is appended to the end of the sequence and node names, for example " (time=23sec)"
  std::string sequenceNamePostfix=std::string(" (")
    +SAFE_CHAR_POINTER(rootNode->GetAttribute("MultidimData.Name"))+"="+parameterValue
    +SAFE_CHAR_POINTER(rootNode->GetAttribute("MultidimData.Unit"))+")";

  // If one already exist, return it, otherwise create a new one
  vtkMRMLHierarchyNode* sequenceNode = vtkMRMLHierarchyNode::SafeDownCast( GetSequenceNodeAtValue( rootNode, parameterValue ) );  
  if ( sequenceNode == NULL )
  {
    sequenceNode = vtkMRMLHierarchyNode::New();
    sequenceNode->AllowMultipleChildrenOn();
    sequenceNode->SetAttribute( "HierarchyType", "MultidimData" );
    sequenceNode->SetAttribute( "MultidimData.NodeType", "Sequence");
    sequenceNode->SetAttribute( "MultidimData.Value", parameterValue );

    sequenceNode->SetHideFromEditors(true);
    this->GetMRMLScene()->AddNode( sequenceNode );
    sequenceNode->SetScene( this->GetMRMLScene() );
    sequenceNode->Delete(); // ownership passed to the scene
    sequenceNode->SetParentNodeID( rootNode->GetID() );    
    std::string sequenceNodeName=std::string(rootNode->GetName())+sequenceNamePostfix+" sequence";
    sequenceNode->SetName(sequenceNodeName.c_str());
  }

  return sequenceNode;
}


//---------------------------------------------------------------------------
void vtkSlicerMultidimDataLogic
::RemoveDataNodeAtValue( vtkMRMLHierarchyNode* rootNode, vtkMRMLNode* dataNode, const char* value )
{
  // Note that this function doesn't delete the data node, it just removes it from the hierarchy
  if ( rootNode==NULL || dataNode==NULL || value==NULL)
  {
    vtkErrorMacro("vtkSlicerMultidimDataLogic::RemoveDataNodeAtValue failed: rootNode, dataNode, or value is invalid");
    return;
  }

  vtkMRMLHierarchyNode* sequenceNode = vtkMRMLHierarchyNode::SafeDownCast( this->GetSequenceNodeAtValue( rootNode, value ) );

  // Delete the data connector node
  for ( int i = 0; i < sequenceNode->GetNumberOfChildrenNodes(); i++ )
  {
    vtkMRMLHierarchyNode* dataConnectorNode = vtkMRMLHierarchyNode::SafeDownCast( sequenceNode->GetNthChildNode( i ) );
    if ( this->IsDataConnectorNode( dataConnectorNode ) && strcmp( dataConnectorNode->GetAssociatedNodeID(), dataNode->GetID() ) == 0 )
    {
      dataConnectorNode->SetAssociatedNodeID( "" );
      dataConnectorNode->SetParentNodeID( "" );
      this->GetMRMLScene()->RemoveNode( dataConnectorNode );
    }
  }

}


//---------------------------------------------------------------------------
void vtkSlicerMultidimDataLogic
::RemoveSequenceNodeAtValue( vtkMRMLHierarchyNode* rootNode, const char* value )
{

  if ( rootNode==NULL || value==NULL )
  {
    vtkErrorMacro("vtkSlicerMultidimDataLogic::RemoveSequenceNodeAtValue failed: rootNode or value is invalid");
    return;
  }

  vtkMRMLHierarchyNode* sequenceNode = vtkMRMLHierarchyNode::SafeDownCast( this->GetSequenceNodeAtValue( rootNode, value ) );

  // Delete all the child data connector nodes
  for ( int i = 0; i < sequenceNode->GetNumberOfChildrenNodes(); i++ )
  {
    vtkMRMLHierarchyNode* dataConnectorNode = vtkMRMLHierarchyNode::SafeDownCast( sequenceNode->GetNthChildNode( i ) );
    if ( this->IsDataConnectorNode( dataConnectorNode ) )
    {
      dataConnectorNode->SetAssociatedNodeID( "" );
      dataConnectorNode->SetParentNodeID( "" );
      this->GetMRMLScene()->RemoveNode( dataConnectorNode );
    }
  }

  sequenceNode->SetAssociatedNodeID( "" );
  sequenceNode->SetParentNodeID( "" );
  this->GetMRMLScene()->RemoveNode( sequenceNode );
}


//---------------------------------------------------------------------------
void vtkSlicerMultidimDataLogic
::GetDataNodesAtValue(vtkCollection* foundNodes, vtkMRMLHierarchyNode* rootNode, const char* parameterValue)
{
  if (rootNode==NULL)
  {
    vtkWarningMacro("GetDataNodesAtValue failed, invalid root node"); 
    return;
  }
  if (foundNodes==NULL)
  {
    vtkErrorMacro("GetDataNodesAtValue failed, invalid output collection"); 
    return;
  }
  if (parameterValue==NULL)
  {
    vtkErrorMacro("GetDataNodesAtValue failed, invalid parameter value"); 
    return;
  }

  // Return all of the child nodes that are not connector nodes
  vtkMRMLHierarchyNode* sequenceNode = vtkMRMLHierarchyNode::SafeDownCast( this->GetSequenceNodeAtValue( rootNode, parameterValue ) );

  if ( sequenceNode == NULL )
  {
    vtkWarningMacro("GetDataNodesAtValue failed, no sequence node has been found with value "<<parameterValue);
    return;
  }

  int numChildNodes = sequenceNode->GetNumberOfChildrenNodes();

  if ( numChildNodes == 0 && sequenceNode->GetAssociatedNode() != NULL )
  {
    foundNodes->AddItem( sequenceNode->GetAssociatedNode() );
  }

  for ( int i = 0; i < numChildNodes; i++ )
  {
    vtkMRMLHierarchyNode* dataConnectorNode = vtkMRMLHierarchyNode::SafeDownCast( sequenceNode->GetNthChildNode( i ) ); 
    if ( dataConnectorNode != NULL && this->IsDataConnectorNode( dataConnectorNode ) )
    {
      if (dataConnectorNode->GetAssociatedNode()!=NULL)
      {
        foundNodes->AddItem( dataConnectorNode->GetAssociatedNode() );
      }
    }
  }  
}


//---------------------------------------------------------------------------
void vtkSlicerMultidimDataLogic
::GetNonDataNodesAtValue(vtkCollection* foundNodes, vtkMRMLHierarchyNode* rootNode, const char* parameterValue)
{
  if (rootNode==NULL)
  {
    vtkWarningMacro("GetNonDataNodesAtValue failed, invalid root node"); 
    return;
  }
  if (foundNodes==NULL)
  {
    vtkErrorMacro("GetNonDataNodesAtValue failed, invalid output collection"); 
    return;
  }
  if (parameterValue==NULL)
  {
    vtkErrorMacro("GetNonDataNodesAtValue failed, invalid parameter value"); 
    return;
  }

  // Return all of the nodes in the 
  vtkSmartPointer<vtkCollection> dataNodeCollection = vtkSmartPointer<vtkCollection>::New();
  this->GetDataNodesAtValue( dataNodeCollection, rootNode, parameterValue );    

  for ( int i = 0; i < this->GetMRMLScene()->GetNumberOfNodes(); i++ )
  {
    vtkMRMLNode* currentNode = vtkMRMLNode::SafeDownCast( this->GetMRMLScene()->GetNthNode( i ) );
    
    bool isSequenced = false;
    for ( int j = 0; j < dataNodeCollection->GetNumberOfItems(); j++ )
    {
      vtkMRMLNode* testDataNode = vtkMRMLNode::SafeDownCast( dataNodeCollection->GetItemAsObject( j ) );
      if ( strcmp( currentNode->GetID(), testDataNode->GetID() ) == 0 )
      {
        isSequenced = true;
      }
    }

    if ( ! isSequenced )
    {
      foundNodes->AddItem( currentNode );
    }
  }
}


//---------------------------------------------------------------------------
const char* vtkSlicerMultidimDataLogic
::GetDataNodeRoleAtValue( vtkMRMLHierarchyNode* rootNode, vtkMRMLNode* dataNode, const char* parameterValue)
{
  if (rootNode==NULL)
  {
    vtkWarningMacro("GetDataNodeRoleAtValue failed, invalid root node"); 
    return "";
  }
  if (dataNode==NULL)
  {
    vtkErrorMacro("GetDataNodeRoleAtValue failed, invalid input data node"); 
    return "";
  }
  if (parameterValue==NULL)
  {
    vtkErrorMacro("GetDataNodeRoleAtValue failed, invalid parameter value"); 
    return "";
  }

  // Return all of the child nodes that are not connector nodes
  vtkMRMLHierarchyNode* dataConnectorNode = this->GetDataConnectorNode( rootNode, dataNode, parameterValue );

  if ( dataConnectorNode == NULL )
  {
    vtkWarningMacro("GetDataNodeRoleAtValue failed, no data connector node has been found with value "<<parameterValue);
    return "";
  }

  return SAFE_CHAR_POINTER(dataConnectorNode->GetAttribute( "MultidimData.DataRole" ));
}


//---------------------------------------------------------------------------
void vtkSlicerMultidimDataLogic
::SetDataNodeRoleAtValue( vtkMRMLHierarchyNode* rootNode, vtkMRMLNode* dataNode, const char* parameterValue, const char* role )
{
  if (rootNode==NULL)
  {
    vtkWarningMacro("GetDataNodeRoleAtValue failed, invalid root node"); 
    return;
  }
  if (dataNode==NULL)
  {
    vtkErrorMacro("GetDataNodeRoleAtValue failed, invalid input data node"); 
    return;
  }
  if (parameterValue==NULL)
  {
    vtkErrorMacro("GetDataNodeRoleAtValue failed, invalid parameter value"); 
    return;
  }
  if (role==NULL)
  {
    vtkErrorMacro("GetDataNodeRoleAtValue failed, invalid role value"); 
    return;
  }

  // Return all of the child nodes that are not connector nodes
  vtkMRMLHierarchyNode* dataConnectorNode = this->GetDataConnectorNode( rootNode, dataNode, parameterValue );

  if ( dataConnectorNode == NULL )
  {
    vtkWarningMacro("GetDataNodeRoleAtValue failed, no data connector node has been found with value "<<parameterValue);
    return;
  }

  dataConnectorNode->SetAttribute( "MultidimData.DataRole", role );
}


//---------------------------------------------------------------------------
bool vtkSlicerMultidimDataLogic
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
  if ( hierarchyType == NULL || strcmp( hierarchyType, "MultidimData" ) != 0 )
  {
    return false;
  }

  const char* nodeType = hierarchyNode->GetAttribute( "MultidimData.NodeType" );
  if ( nodeType == NULL || strcmp( nodeType, "DataConnector" ) != 0 )
  {
    return false;
  }

  return true;
}


//---------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkSlicerMultidimDataLogic
::GetDataConnectorNode( vtkMRMLHierarchyNode* rootNode, vtkMRMLNode* dataNode, const char* parameterValue )
{
  // Return all of the child nodes that are not connector nodes
  vtkMRMLHierarchyNode* sequenceNode = vtkMRMLHierarchyNode::SafeDownCast( this->GetSequenceNodeAtValue( rootNode, parameterValue ) );

  if (sequenceNode==NULL)
  {
    // not found the sequence node
    return NULL;
  }

  int numChildNodes = sequenceNode->GetNumberOfChildrenNodes();

  for ( int i = 0; i < numChildNodes; i++ )
  {
    vtkMRMLHierarchyNode* dataConnectorNode = vtkMRMLHierarchyNode::SafeDownCast( sequenceNode->GetNthChildNode( i ) ); 
    if ( dataConnectorNode != NULL && this->IsDataConnectorNode( dataConnectorNode ) && strcmp( dataConnectorNode->GetAssociatedNodeID(), dataNode->GetID() ) == 0 )
    {
      return dataConnectorNode;
    }
  }

  // not found the data connector node
  return NULL;
}


//---------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkSlicerMultidimDataLogic
::GetSequenceNodeAtValue( vtkMRMLHierarchyNode* rootNode, const char* value )
{  
  if ( rootNode==NULL || value==NULL )
  {
    vtkErrorMacro("GetSequenceNodeAtValue failed: rootNode or value is invalid");
    return NULL;
  }

  int numberOfChildrenNodes = rootNode->GetNumberOfChildrenNodes();
  for ( int i = 0; i < numberOfChildrenNodes; i++ )
  {
    vtkMRMLHierarchyNode* sequenceNode = rootNode->GetNthChildNode(i);
    // note that this is the sequence node, which does not contain any data, only the value along the dimension
    const char* multiDimensionValue=sequenceNode->GetAttribute( "MultidimData.Value" );
    if ( multiDimensionValue!=NULL && strcmp( multiDimensionValue, value ) == 0 )
    {
      // found it!
      return sequenceNode;
    }
  }

  // not found
  return NULL;
}



//---------------------------------------------------------------------------
void vtkSlicerMultidimDataLogic
::SetDataNodesHiddenAtValue( vtkMRMLHierarchyNode* rootNode, bool hidden, const char* value )
{  
  if ( rootNode==NULL || value==NULL)
  {
    vtkErrorMacro("SetDataNodesHiddenAtValue failed: rootNode or value is invalid");
    return;
  }

  vtkSmartPointer<vtkCollection> dataNodes = vtkSmartPointer<vtkCollection>::New();
  this->GetDataNodesAtValue( dataNodes, rootNode, value );

  for ( int i = 0; i < dataNodes->GetNumberOfItems(); i++ )
  {
    vtkMRMLNode* currentDataNode = vtkMRMLNode::SafeDownCast( dataNodes->GetItemAsObject( i ) );
    currentDataNode->SetHideFromEditors( hidden );
  }

}


//---------------------------------------------------------------------------
bool vtkSlicerMultidimDataLogic
::GetDataNodesHiddenAtValue( vtkMRMLHierarchyNode* rootNode, const char* value )
{  
  if ( rootNode==NULL || value==NULL)
  {
    vtkErrorMacro("GetDataNodesHiddenAtValue failed: rootNode or value is invalid");
    return true;
  }

  vtkSmartPointer<vtkCollection> dataNodes = vtkSmartPointer<vtkCollection>::New();
  this->GetDataNodesAtValue( dataNodes, rootNode, value );

  for ( int i = 0; i < dataNodes->GetNumberOfItems(); i++ )
  {
    vtkMRMLNode* currentDataNode = vtkMRMLNode::SafeDownCast( dataNodes->GetItemAsObject( i ) );
    if ( ! currentDataNode->GetHideFromEditors() )
    {
      return false; // Return false if any node is not hidden for this value
    }
  }

  return true;
}
