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

// Enable this define if the extension is used with a Slicer core
// that supports dynamic changing of MRML node HideFromEditors property
//#define DYNAMIC_HIDENODEFROMEDITORS_AVAILABLE

// MultiDimension Logic includes
#include "vtkSlicerMultiDimensionBrowserLogic.h"
#include "vtkMRMLMultiDimensionBrowserNode.h"

// MRML includes
#include "vtkMRMLHierarchyNode.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLScalarVolumeDisplayNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkNew.h>

// STL includes
#include <algorithm>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerMultiDimensionBrowserLogic);

//----------------------------------------------------------------------------
vtkSlicerMultiDimensionBrowserLogic::vtkSlicerMultiDimensionBrowserLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerMultiDimensionBrowserLogic::~vtkSlicerMultiDimensionBrowserLogic()
{  
}

//----------------------------------------------------------------------------
void vtkSlicerMultiDimensionBrowserLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}

//---------------------------------------------------------------------------
void vtkSlicerMultiDimensionBrowserLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerMultiDimensionBrowserLogic::RegisterNodes()
{
  if (this->GetMRMLScene()==NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
  vtkMRMLMultiDimensionBrowserNode* browserNode = vtkMRMLMultiDimensionBrowserNode::New();
  this->GetMRMLScene()->RegisterNodeClass(browserNode);
  browserNode->Delete();
}

//---------------------------------------------------------------------------
void vtkSlicerMultiDimensionBrowserLogic::UpdateFromMRMLScene()
{
  if (this->GetMRMLScene()==NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
  // TODO: probably we need to add observer to all vtkMRMLMultiDimensionBrowserNode-type nodes
}

//---------------------------------------------------------------------------
void vtkSlicerMultiDimensionBrowserLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (node==NULL)
  {
    vtkErrorMacro("An invalid node is attempted to be added");
    return;
  }
  if (node->IsA("vtkMRMLMultiDimensionBrowserNode"))
  {
    vtkDebugMacro("OnMRMLSceneNodeAdded: Have a vtkMRMLMultiDimensionBrowserNode node");
    vtkUnObserveMRMLNodeMacro(node); // remove any previous observation that might have been added
    vtkObserveMRMLNodeMacro(node);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerMultiDimensionBrowserLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (node==NULL)
  {
    vtkErrorMacro("An invalid node is attempted to be removed");
    return;
  }
  if (node->IsA("vtkMRMLMultiDimensionBrowserNode"))
  {
    vtkDebugMacro("OnMRMLSceneNodeRemoved: Have a vtkMRMLMultiDimensionBrowserNode node");
    vtkUnObserveMRMLNodeMacro(node);
  } 
}

//---------------------------------------------------------------------------
void vtkSlicerMultiDimensionBrowserLogic::UpdateVirtualOutputNode(vtkMRMLMultiDimensionBrowserNode* browserNode)
{
  vtkMRMLHierarchyNode* virtualOutputNode=browserNode->GetVirtualOutputNode();
  if (virtualOutputNode==NULL)
  {
    // there is no valid output node, so there is nothing to update
    return;
  }

  vtkMRMLHierarchyNode* selectedSequenceNode=browserNode->GetSelectedSequenceNode();
  if (selectedSequenceNode==NULL)
  {
    // no selected sequence node
    return;
  }

  vtkMRMLScene* scene=virtualOutputNode->GetScene();
  if (scene==NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }

  std::vector<vtkMRMLHierarchyNode*> outputConnectorNodes;  
  virtualOutputNode->GetAllChildrenNodes(outputConnectorNodes);
  std::vector<vtkMRMLHierarchyNode*> validOutputConnectorNodes;
  int numberOfSourceChildNodes=selectedSequenceNode->GetNumberOfChildrenNodes();
  for (int sourceChildNodeIndex=0; sourceChildNodeIndex<numberOfSourceChildNodes; ++sourceChildNodeIndex)
  {    
    vtkMRMLHierarchyNode* sourceConnectorNode=selectedSequenceNode->GetNthChildNode(sourceChildNodeIndex);
    if (sourceConnectorNode==NULL || sourceConnectorNode->GetAssociatedNode()==NULL)
    {
      vtkErrorMacro("Invalid sequence node");
      continue;
    }
    vtkMRMLNode* sourceNode=selectedSequenceNode->GetNthChildNode(sourceChildNodeIndex)->GetAssociatedNode();
    const char* dataRole=sourceConnectorNode->GetAttribute("MultiDimension.DataRole");
    if (dataRole==NULL)
    {
      vtkErrorMacro("MultiDimension.DataRole is missing");
      continue;
    }
    vtkMRMLNode* targetOutputNode=NULL;
    for (std::vector<vtkMRMLHierarchyNode*>::iterator outputConnectorNodeIt=outputConnectorNodes.begin(); outputConnectorNodeIt!=outputConnectorNodes.end(); ++outputConnectorNodeIt)
    {
      const char* outputDataRole=(*outputConnectorNodeIt)->GetAttribute("MultiDimension.DataRole");
      if (outputDataRole==NULL)
      {
        vtkErrorMacro("MultiDimension.DataRole is missing from a virtual output node");
        continue;
      }
      if (strcmp(dataRole,outputDataRole)==0)
      {
        // found a node with the same name in the hierarchy => reuse that
        vtkMRMLNode* outputNode=(*outputConnectorNodeIt)->GetAssociatedNode();
        if (outputNode==NULL)
        {
          vtkErrorMacro("A connector node found without an associated node");
          continue;
        }
        validOutputConnectorNodes.push_back(*outputConnectorNodeIt);
        targetOutputNode=outputNode;
        break;
      }      
    }

    if (targetOutputNode==NULL)
    {
      // haven't found a node in the virtual output node hierarchy that we could reuse,
      // so create a new targetOutputNode
      targetOutputNode=sourceNode->CreateNodeInstance();
      targetOutputNode->SetHideFromEditors(false);
      scene->AddNode(targetOutputNode);
      targetOutputNode->Delete(); // ownership transferred to the scene, so we can release the pointer
      // now connect this new node to the virtualOutput hierarchy with a new connector node
      vtkMRMLHierarchyNode* outputConnectorNode=vtkMRMLHierarchyNode::New();
      scene->AddNode(outputConnectorNode);
      outputConnectorNode->Delete(); // ownership transferred to the scene, so we can release the pointer
      outputConnectorNode->SetAttribute("MultiDimension.DataRole", dataRole);
      outputConnectorNode->SetParentNodeID(virtualOutputNode->GetID());
      outputConnectorNode->SetAssociatedNodeID(targetOutputNode->GetID());
      std::string outputConnectorNodeName=std::string(virtualOutputNode->GetName())+" "+dataRole+" connector";
      outputConnectorNode->SetName(outputConnectorNodeName.c_str());
      outputConnectorNode->SetHideFromEditors(true);
      outputConnectorNodes.push_back(outputConnectorNode);
      validOutputConnectorNodes.push_back(outputConnectorNode);
    }

    // Update the target node with the contents of the source node

    /*if (targetOutputNode->IsA("vtkMRMLScalarVolumeNode"))
    {
      vtkMRMLScalarVolumeNode* targetScalarVolumeNode=vtkMRMLScalarVolumeNode::SafeDownCast(targetOutputNode);
      char* targetOutputDisplayNodeId=NULL;
      if (targetScalarVolumeNode->GetDisplayNode()==NULL)
      {
        // there is no display node yet, so create one now
        vtkMRMLScalarVolumeDisplayNode* displayNode=vtkMRMLScalarVolumeDisplayNode::New();
        displayNode->SetDefaultColorMap();
        scene->AddNode(displayNode);
        displayNode->Delete(); // ownership transferred to the scene, so we can release the pointer
        targetOutputDisplayNodeId=displayNode->GetID();
        displayNode->SetHideFromEditors(false); // TODO: remove this line, just for testing        
      }
      else
      {
        targetOutputDisplayNodeId=targetScalarVolumeNode->GetDisplayNode()->GetID();
      }      
      targetOutputNode->CopyWithSingleModifiedEvent(sourceNode);    
      targetScalarVolumeNode->SetAndObserveDisplayNodeID(targetOutputDisplayNodeId);
    }
    else
    */

#ifdef DYNAMIC_HIDENODEFROMEDITORS_AVAILABLE
    int targetInitialModify=targetOutputNode->StartModify();
    if (sourceNode->GetHideFromEditors())
    {
      // We need to carefully copy the source node to the target
      // because we have to avoid the node having temporarily hidden
      // from editors (as it leads to loss of selection status,
      // e.g., deselected from the foreground/background slice view).
      // Save source
      int sourceInitialDisaleModified=sourceNode->GetDisableModifiedEvent();        
      // Modify source
      sourceNode->SetDisableModifiedEvent(1);
      sourceNode->SetHideFromEditors(0);
      // Copy source->target
      targetOutputNode->CopyWithSingleModifiedEvent(sourceNode);
      // Restore source
      sourceNode->SetHideFromEditors(1);
      sourceNode->SetDisableModifiedEvent(sourceInitialDisaleModified);
    }
    else
    {
      //targetOutputNode->SetName(sourceNode->GetName());
      targetOutputNode->CopyWithSingleModifiedEvent(sourceNode);    
    }
    targetOutputNode->EndModify(targetInitialModify);
#else
    // Slice browser is updated when there is a rename, but we want to avoid update, because
    // the source node may be hidden from editors and it would result in removing the target node
    // from the slicer browser. To avoid update of the slice browser, we set the name in advance.
    targetOutputNode->SetName(sourceNode->GetName());
    targetOutputNode->CopyWithSingleModifiedEvent(sourceNode);
    // Usually source nodes are hidden from editors, so make sure that they are visible
    targetOutputNode->SetHideFromEditors(false);
    // The following renaming a hack is for making sure the volume appears in the slice viewer GUI
    std::string name=targetOutputNode->GetName();
    targetOutputNode->SetName("");
    targetOutputNode->SetName(name.c_str());
    //targetOutputNode->SetName(dataRole);
#endif
  }

  // Remove orphaned connector nodes and associated data nodes 
  for (std::vector<vtkMRMLHierarchyNode*>::iterator outputConnectorNodeIt=outputConnectorNodes.begin(); outputConnectorNodeIt!=outputConnectorNodes.end(); ++outputConnectorNodeIt)
  {
    if (std::find(validOutputConnectorNodes.begin(), validOutputConnectorNodes.end(), (*outputConnectorNodeIt))==validOutputConnectorNodes.end())
    {
      // this output connector node is not in the list of valid connector nodes (no corresponding data for the current parameter value)
      // so, remove the connector node and data node from the scene
      scene->RemoveNode((*outputConnectorNodeIt)->GetAssociatedNode());
      scene->RemoveNode(*outputConnectorNodeIt);
    }
  }
}

//---------------------------------------------------------------------------
void vtkSlicerMultiDimensionBrowserLogic::ProcessMRMLNodesEvents(vtkObject *caller, unsigned long event, void *vtkNotUsed(callData))
{
  vtkMRMLMultiDimensionBrowserNode *browserNode = vtkMRMLMultiDimensionBrowserNode::SafeDownCast(caller);
  if (browserNode==NULL)
  {
    vtkErrorMacro("Expected a vtkMRMLMultiDimensionBrowserNode");
    return;
  }

  UpdateVirtualOutputNode(browserNode);
}
