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
vtkMRMLHierarchyNode* vtkSlicerMultiDimensionLogic
::CreateMultiDimensionRootNode()
{
  vtkSmartPointer<vtkMRMLHierarchyNode> rootNode = vtkSmartPointer<vtkMRMLHierarchyNode>::New();
  rootNode->AllowMultipleChildrenOn();
  rootNode->SetAttribute("HierarchyType", "MultiDimension");
  rootNode->SetAttribute("MultiDimension.Name", "<Undefined>");
  rootNode->SetAttribute("MultiDimension.Unit", "<Undefined>");
  this->GetMRMLScene()->AddNode(rootNode);
  rootNode->SetName("MultiDimensionHierarchy");

  return rootNode;
}

//---------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkSlicerMultiDimensionLogic
::SetMultiDimensionRootNode(vtkMRMLHierarchyNode* node)
{
  // TODO: remove this method
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
::DeleteMultiDimensionRootNode(vtkMRMLHierarchyNode* node)
{
  // TODO: remove this method
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
::AddDataNodeAtValue(vtkMRMLHierarchyNode* rootNode, vtkMRMLNode* dataNode, const char* parameterValue)
{
  if (rootNode==NULL || dataNode==NULL)
  {
    vtkErrorMacro("AddDataNodeAtValue failed: rootNode or dataNode is invalid");
    return;
  }
 
  // This name is appended to the end of the sequence and node names, for example " (time=23sec)"
  std::string sequenceNamePostfix=std::string(" (")+rootNode->GetAttribute("MultiDimension.Name")+"="+parameterValue+rootNode->GetAttribute("MultiDimension.Unit")+")";

  // Need to create connector nodes if the timePoint already exists
  vtkMRMLHierarchyNode* sequenceNode = vtkMRMLHierarchyNode::SafeDownCast( GetSequenceNodeAtValue( rootNode, parameterValue ) );  
  if ( sequenceNode == NULL )
  {
    sequenceNode = vtkMRMLHierarchyNode::New();
    sequenceNode->AllowMultipleChildrenOn();
    sequenceNode->SetAttribute( "HierarchyType", "MultiDimension" );
    sequenceNode->SetAttribute( "MultiDimension.Value", parameterValue );

    sequenceNode->SetHideFromEditors(true);
    this->GetMRMLScene()->AddNode( sequenceNode );
    sequenceNode->Delete(); // ownership passed to the scene
    sequenceNode->SetParentNodeID( rootNode->GetID() );    
    std::string sequenceNodeName=std::string(rootNode->GetName())+sequenceNamePostfix+" sequence";
    sequenceNode->SetName(sequenceNodeName.c_str());
  }  

  std::string dataNodeName=std::string(rootNode->GetName())+"/"+dataNode->GetName()+sequenceNamePostfix;

  // Create a data connector node for the new node
  vtkSmartPointer<vtkMRMLHierarchyNode> dataConnectorNode = vtkSmartPointer<vtkMRMLHierarchyNode>::New();
  dataConnectorNode->AllowMultipleChildrenOff();
  dataConnectorNode->SetAttribute( "HierarchyType", "MultiDimension" );
  dataConnectorNode->SetAttribute( "MultiDimension.SourceDataName", dataNode->GetName() );
  dataConnectorNode->SetHideFromEditors(true);
  this->GetMRMLScene()->AddNode( dataConnectorNode );
  dataConnectorNode->SetParentNodeID( sequenceNode->GetID() );
  std::string dataConnectorNodeName=dataNodeName+" connector";
  dataConnectorNode->SetName(dataConnectorNodeName.c_str());

  // Update data node name (from "Image" it generates "Sequence1/Image (time=23sec)") and add to the scene
  
  dataNode->SetName( dataNodeName.c_str() );
  dataNode->SetHideFromEditors(true);
  this->GetMRMLScene()->AddNode( dataNode );
  dataConnectorNode->SetAssociatedNodeID( dataNode->GetID() );
}


//---------------------------------------------------------------------------
void vtkSlicerMultiDimensionLogic
::RemoveSequenceNodeAtValue(vtkMRMLHierarchyNode* rNode, const char* value )
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
::GetDataNodesAtValue(vtkMRMLHierarchyNode* rootNode, const char* parameterValue )
{
  // TODO: replace this method by one that takes a vtkCollection as input, as it's dangerous to return a collection (the caller might forget to delete it)
  if ( ! rootNode )
  {
    return NULL;
  }

  // Return all of the child nodes that are not connector nodes
  vtkMRMLHierarchyNode* sequenceNode = vtkMRMLHierarchyNode::SafeDownCast( this->GetSequenceNodeAtValue( rootNode, parameterValue ) );
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