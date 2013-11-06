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

// MultidimData Logic includes
#include "vtkSlicerMultidimDataBrowserLogic.h"
#include "vtkMRMLMultidimDataBrowserNode.h"
#include "vtkMRMLMultidimDataNode.h"

// MRML includes
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLScalarVolumeDisplayNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkMatrix4x4.h>
#include <vtkNew.h>

// STL includes
#include <algorithm>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerMultidimDataBrowserLogic);

//----------------------------------------------------------------------------
vtkSlicerMultidimDataBrowserLogic::vtkSlicerMultidimDataBrowserLogic()
{
}

//----------------------------------------------------------------------------
vtkSlicerMultidimDataBrowserLogic::~vtkSlicerMultidimDataBrowserLogic()
{  
}

//----------------------------------------------------------------------------
void vtkSlicerMultidimDataBrowserLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}

//---------------------------------------------------------------------------
void vtkSlicerMultidimDataBrowserLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerMultidimDataBrowserLogic::RegisterNodes()
{
  if (this->GetMRMLScene()==NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
  vtkMRMLMultidimDataBrowserNode* browserNode = vtkMRMLMultidimDataBrowserNode::New();
  this->GetMRMLScene()->RegisterNodeClass(browserNode);
  browserNode->Delete();
}

//---------------------------------------------------------------------------
void vtkSlicerMultidimDataBrowserLogic::UpdateFromMRMLScene()
{
  if (this->GetMRMLScene()==NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
  // TODO: probably we need to add observer to all vtkMRMLMultidimDataBrowserNode-type nodes
}

//---------------------------------------------------------------------------
void vtkSlicerMultidimDataBrowserLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (node==NULL)
  {
    vtkErrorMacro("An invalid node is attempted to be added");
    return;
  }
  if (node->IsA("vtkMRMLMultidimDataBrowserNode"))
  {
    vtkDebugMacro("OnMRMLSceneNodeAdded: Have a vtkMRMLMultidimDataBrowserNode node");
    vtkUnObserveMRMLNodeMacro(node); // remove any previous observation that might have been added
    vtkObserveMRMLNodeMacro(node);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerMultidimDataBrowserLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (node==NULL)
  {
    vtkErrorMacro("An invalid node is attempted to be removed");
    return;
  }
  if (node->IsA("vtkMRMLMultidimDataBrowserNode"))
  {
    vtkDebugMacro("OnMRMLSceneNodeRemoved: Have a vtkMRMLMultidimDataBrowserNode node");
    vtkUnObserveMRMLNodeMacro(node);
  } 
}

//---------------------------------------------------------------------------
void vtkSlicerMultidimDataBrowserLogic::UpdateVirtualOutputNodes(vtkMRMLMultidimDataBrowserNode* browserNode)
{
  if (browserNode==NULL)
  {
    vtkWarningMacro("vtkSlicerMultidimDataBrowserLogic::UpdateVirtualOutputNodes failed: browserNode is invalid");
    return;
  }
  
  if (browserNode->GetRootNode()==NULL)
  {
    browserNode->RemoveAllVirtualOutputNodes();
    return;
  }

  vtkMRMLScene* scene=browserNode->GetScene();
  if (scene==NULL)
  {
    vtkErrorMacro("vtkSlicerMultidimDataBrowserLogic::UpdateVirtualOutputNodes failed: scene is invalid");
    return;
  }

  vtkSmartPointer<vtkCollection> nodesInSelectedBundle=vtkSmartPointer<vtkCollection>::New();
  int selectedBundleIndex=browserNode->GetSelectedBundleIndex();
  std::string parameterValue;
  if (selectedBundleIndex>=0)
  {
    parameterValue=browserNode->GetRootNode()->GetNthParameterValue(selectedBundleIndex);
    browserNode->GetRootNode()->GetDataNodesAtValue(nodesInSelectedBundle, parameterValue.c_str());
  }
  
#ifdef DYNAMIC_HIDENODEFROMEDITORS_AVAILABLE
  // Vectors for calling EndModify() events at once, at the end
  std::vector<vtkMRMLNode*> endModifyNodes;  
  std::vector<int> endModifyValues;  
#endif

  // Keep track of virtual nodes that we use and remove the ones that are not used in the selected bundle
  std::set<vtkMRMLNode*> virtualOutputNodesToKeep;

  vtkMRMLNode* sourceNode=NULL;
  vtkCollectionSimpleIterator it;
  for (nodesInSelectedBundle->InitTraversal(it); (sourceNode = (vtkMRMLNode*)nodesInSelectedBundle->GetNextItemAsObject(it)) ;)
  {
    std::string dataRole=browserNode->GetRootNode()->GetDataNodeRoleAtValue(sourceNode, parameterValue.c_str());
    if (dataRole.empty())
    {
      vtkErrorMacro("Role of node "<< (sourceNode->GetName()?sourceNode->GetName():"(undefined)") <<" is unknown");
      continue;
    }
    vtkMRMLNode* targetOutputNode=browserNode->GetVirtualOutputNode(dataRole.c_str());
    if (targetOutputNode!=NULL)
    {
      // a virtual output node with the requested role exists already
      if (strcmp(targetOutputNode->GetClassName(),sourceNode->GetClassName())!=0)
      {
        // this node is of a different class, cannot be reused
        targetOutputNode=NULL;
      }
    }
    if (targetOutputNode==NULL)
    {
      // haven't found virtual output node that we could reuse, so create a new targetOutputNode
      targetOutputNode=sourceNode->CreateNodeInstance();
      targetOutputNode->SetHideFromEditors(false);
      scene->AddNode(targetOutputNode);
      targetOutputNode->Delete(); // ownership transferred to the scene, so we can release the pointer
      // add this new node to the virtual outputs
      browserNode->AddVirtualOutputNode(targetOutputNode,dataRole.c_str());
    }
    virtualOutputNodesToKeep.insert(targetOutputNode);

    // Update the target node with the contents of the source node    

#ifdef DYNAMIC_HIDENODEFROMEDITORS_AVAILABLE
    int targetInitialModify=targetOutputNode->StartModify();
    endModifyValues.push_back(targetInitialModify);
    endModifyNodes.push_back(targetOutputNode);
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
      ShallowCopy(targetOutputNode,sourceNode);
      // Restore source
      sourceNode->SetHideFromEditors(1);
      sourceNode->SetDisableModifiedEvent(sourceInitialDisaleModified);
    }
    else
    {
      //targetOutputNode->SetName(sourceNode->GetName());
      targetOutputNode->CopyWithSingleModifiedEvent(sourceNode);    
    }
    //targetOutputNode->EndModify(targetInitialModify);

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

  // Remove orphaned virtual output nodes
  std::vector<vtkMRMLNode*> virtualNodes;
  browserNode->GetAllVirtualOutputNodes(virtualNodes);
  for (std::vector<vtkMRMLNode*>::iterator virtualNodeIt=virtualNodes.begin(); virtualNodeIt!=virtualNodes.end(); ++virtualNodeIt)
  {
    if (virtualOutputNodesToKeep.find(*virtualNodeIt)==virtualOutputNodesToKeep.end())
    {
      // this output connector node is not in the list of valid connector nodes (no corresponding data for the current parameter value)
      // so, remove the connector node and data node from the scene
      browserNode->RemoveVirtualOutputNode(*virtualNodeIt);
    }
  }

#ifdef DYNAMIC_HIDENODEFROMEDITORS_AVAILABLE
  for (int i=endModifyNodes.size()-1; i>=0; i--)
  {
    endModifyNodes[i]->EndModify(endModifyValues[i]);  
  }
#endif

}

//---------------------------------------------------------------------------
void vtkSlicerMultidimDataBrowserLogic::ProcessMRMLNodesEvents(vtkObject *caller, unsigned long event, void *vtkNotUsed(callData))
{
  vtkMRMLMultidimDataBrowserNode *browserNode = vtkMRMLMultidimDataBrowserNode::SafeDownCast(caller);
  if (browserNode==NULL)
  {
    vtkErrorMacro("Expected a vtkMRMLMultidimDataBrowserNode");
    return;
  }

  UpdateVirtualOutputNodes(browserNode);
}

//---------------------------------------------------------------------------
void vtkSlicerMultidimDataBrowserLogic::ShallowCopy(vtkMRMLNode* target, vtkMRMLNode* source)
{
  if (target->IsA("vtkMRMLScalarVolumeNode"))
  {
    vtkMRMLScalarVolumeNode* targetScalarVolumeNode=vtkMRMLScalarVolumeNode::SafeDownCast(target);
    vtkMRMLScalarVolumeNode* sourceScalarVolumeNode=vtkMRMLScalarVolumeNode::SafeDownCast(source);
    /*char* targetOutputDisplayNodeId=NULL;
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
    target->CopyWithSingleModifiedEvent(source);    
    targetScalarVolumeNode->SetAndObserveDisplayNodeID(targetOutputDisplayNodeId);*/
    targetScalarVolumeNode->SetAndObserveDisplayNodeID(sourceScalarVolumeNode->GetDisplayNodeID());
    targetScalarVolumeNode->SetAndObserveImageData(sourceScalarVolumeNode->GetImageData());
    //targetScalarVolumeNode->SetAndObserveTransformNodeID(sourceScalarVolumeNode->GetTransformNodeID());
    vtkSmartPointer<vtkMatrix4x4> ijkToRasmatrix=vtkSmartPointer<vtkMatrix4x4>::New();
    sourceScalarVolumeNode->GetIJKToRASMatrix(ijkToRasmatrix);
    targetScalarVolumeNode->SetIJKToRASMatrix(ijkToRasmatrix);
    targetScalarVolumeNode->SetLabelMap(sourceScalarVolumeNode->GetLabelMap());
    targetScalarVolumeNode->SetName(sourceScalarVolumeNode->GetName());
    targetScalarVolumeNode->SetDisplayVisibility(sourceScalarVolumeNode->GetDisplayVisibility());

      // TODO: copy attributes and node references, storage node?
  }
  else
  {
    target->CopyWithSingleModifiedEvent(source);
  }
}