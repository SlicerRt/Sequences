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
#include <vtkObjectFactory.h>

// STL includes
#include <algorithm>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerMultidimDataBrowserLogic);

//----------------------------------------------------------------------------
vtkSlicerMultidimDataBrowserLogic::vtkSlicerMultidimDataBrowserLogic()
: UpdateVirtualOutputNodesInProgress(false)
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
  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLMultidimDataBrowserNode>::New());
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
  if (this->UpdateVirtualOutputNodesInProgress)
  {
    // avoid infinitie loops (caused by triggering a node update during a node update)
    return;
  }
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

  this->UpdateVirtualOutputNodesInProgress=true;
  
  int selectedBundleIndex=browserNode->GetSelectedBundleIndex();
  std::string parameterValue;
  if (selectedBundleIndex>=0)
  {
    parameterValue=browserNode->GetRootNode()->GetNthParameterValue(selectedBundleIndex);
  }

  std::vector< vtkMRMLMultidimDataNode* > synchronizedRootNodes;
  browserNode->GetSynchronizedRootNodes(synchronizedRootNodes, true);
  
  // Keep track of virtual nodes that we use and remove the ones that are not used in the selected bundle
  std::set< vtkMRMLNode* > synchronizedRootNodesToKeep;

  for (std::vector< vtkMRMLMultidimDataNode* >::iterator sourceRootNodeIt=synchronizedRootNodes.begin(); sourceRootNodeIt!=synchronizedRootNodes.end(); ++sourceRootNodeIt)
  {
    vtkMRMLMultidimDataNode* synchronizedRootNode=(*sourceRootNodeIt);
    if (synchronizedRootNode==NULL)
    {
      vtkErrorMacro("Synchronized root node is invalid");
      continue;
    }
    vtkMRMLNode* sourceNode=synchronizedRootNode->GetDataNodeAtValue(parameterValue.c_str());
    if (sourceNode==NULL)
    {
      // no source node is available for the chosen time point
      continue;
    }
    
    vtkMRMLNode* targetOutputNode=browserNode->GetVirtualOutputDataNode(synchronizedRootNode);
    std::vector< vtkMRMLDisplayNode* > targetDisplayNodes;
    if (targetOutputNode!=NULL)
    {
      // a virtual output node with the requested role exists already
      if (strcmp(targetOutputNode->GetClassName(),sourceNode->GetClassName())!=0)
      {
        // this node is of a different class, cannot be reused
        targetOutputNode=NULL;
      }
      else
      {
          vtkMRMLDisplayableNode* targetDisplayableNode =vtkMRMLDisplayableNode::SafeDownCast(targetOutputNode);
          if (targetDisplayableNode!=NULL)
          {
            int numOfDisplayNodes=targetDisplayableNode->GetNumberOfDisplayNodes();
            for (int displayNodeIndex=0; displayNodeIndex<numOfDisplayNodes; displayNodeIndex++)
            {
              targetDisplayNodes.push_back(targetDisplayableNode->GetNthDisplayNode(displayNodeIndex));
            }
          }
      }
    }
    if (targetOutputNode==NULL)
    {
      // haven't found virtual output node that we could reuse, so create a new targetOutputNode and display nodes

      // Add the display nodes      
      std::vector< vtkMRMLDisplayNode* > sourceDisplayNodes;
      synchronizedRootNode->GetDisplayNodesAtValue(sourceDisplayNodes, parameterValue.c_str());

      for (std::vector< vtkMRMLDisplayNode* >::iterator sourceDisplayNodeIt=sourceDisplayNodes.begin(); sourceDisplayNodeIt!=sourceDisplayNodes.end(); ++sourceDisplayNodeIt)
      {
        vtkMRMLDisplayNode* sourceDisplayNode=(*sourceDisplayNodeIt);
        vtkMRMLDisplayNode* targetOutputDisplayNode=vtkMRMLDisplayNode::SafeDownCast(scene->CopyNode(sourceDisplayNode));
        targetDisplayNodes.push_back(targetOutputDisplayNode);
      }

      // Add the data node      
      targetOutputNode=sourceNode->CreateNodeInstance();
      targetOutputNode->SetHideFromEditors(false);
      scene->AddNode(targetOutputNode);
      targetOutputNode->Delete(); // ownership transferred to the scene, so we can release the pointer

      // add the new data and display nodes to the virtual outputs      
      browserNode->AddVirtualOutputNodes(targetOutputNode,targetDisplayNodes,synchronizedRootNode);
    }
    synchronizedRootNodesToKeep.insert(synchronizedRootNode);

    // Update the target node with the contents of the source node    

    // Slice browser is updated when there is a rename, but we want to avoid update, because
    // the source node may be hidden from editors and it would result in removing the target node
    // from the slicer browser. To avoid update of the slice browser, we set the name in advance.
    targetOutputNode->SetName(sourceNode->GetName());

    // Mostly it is a shallow copy (for example for volumes, models)
    targetOutputNode->CopyWithSingleModifiedEvent(sourceNode);

    //targetOutputNode->SetHideFromEditors(false);
    // The following renaming a hack is for making sure the volume appears in the slice viewer GUI
    std::string name=targetOutputNode->GetName();
    targetOutputNode->SetName("");
    targetOutputNode->SetName(name.c_str());

    vtkMRMLDisplayableNode* targetDisplayableNode=vtkMRMLDisplayableNode::SafeDownCast(targetOutputNode);
    if (targetDisplayableNode!=NULL)
    {
      // Get the minimum of the display nodes that are already in the target displayable node
      // and the number of display nodes that we need at the end
      int numOfDisplayNodes=targetDisplayNodes.size();
      if (numOfDisplayNodes>targetDisplayableNode->GetNumberOfDisplayNodes())
      {
        numOfDisplayNodes=targetDisplayableNode->GetNumberOfDisplayNodes();
      }

      for (int displayNodeIndex=0; displayNodeIndex<numOfDisplayNodes; displayNodeIndex++)
      {
        targetDisplayableNode->SetAndObserveNthDisplayNodeID(displayNodeIndex,targetDisplayNodes[displayNodeIndex]->GetID());
      }
      if (targetDisplayableNode->GetNumberOfDisplayNodes()>targetDisplayNodes.size())
      {
        // there are some extra display nodes in the target data node that we have to delete
        while (targetDisplayableNode->GetNumberOfDisplayNodes()>targetDisplayNodes.size())
        {
          // remove last display node
          targetDisplayableNode->RemoveNthDisplayNodeID(targetDisplayableNode->GetNumberOfDisplayNodes()-1);
        }
      }
      if (targetDisplayableNode->GetNumberOfDisplayNodes()<targetDisplayNodes.size())
      {
        // we need to add some more display nodes
        for (int displayNodeIndex=numOfDisplayNodes; displayNodeIndex<targetDisplayNodes.size(); displayNodeIndex++)
        {
          targetDisplayableNode->AddAndObserveDisplayNodeID(targetDisplayNodes[displayNodeIndex]->GetID());
        }
      }
    }

  }

  // Remove orphaned virtual output nodes
  std::vector< vtkMRMLMultidimDataNode* > rootNodes;
  browserNode->GetSynchronizedRootNodes(rootNodes, true);
  for (std::vector< vtkMRMLMultidimDataNode* >::iterator rootNodeIt=rootNodes.begin(); rootNodeIt!=rootNodes.end(); ++rootNodeIt)
  {
    if (synchronizedRootNodesToKeep.find(*rootNodeIt)==synchronizedRootNodesToKeep.end())
    {
      // this root is not in the list of synchronized nodes (the root node is removed or there is no corresponding parameter value)
      // so remove the data from the scene
      vtkMRMLNode* dataNode=browserNode->GetVirtualOutputDataNode(*rootNodeIt);
      std::vector< vtkMRMLDisplayNode* > displayNodes;
      browserNode->GetVirtualOutputDisplayNodes(*rootNodeIt, displayNodes);

      // Remove from the browser node
      browserNode->RemoveVirtualOutputNodes(*rootNodeIt);
      browserNode->RemoveSynchronizedRootNode((*rootNodeIt)->GetID());

      // Remove from the scene
      scene->RemoveNode(dataNode);
      for (std::vector< vtkMRMLDisplayNode* > :: iterator displayNodesIt = displayNodes.begin(); displayNodesIt!=displayNodes.end(); ++displayNodesIt)
      {
        scene->RemoveNode(*displayNodesIt);
      }
      //scene->RemoveNode(*rootNodeIt);
    }
  }

  this->UpdateVirtualOutputNodesInProgress=false;
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
    targetScalarVolumeNode->Modified();

      // TODO: copy attributes and node references, storage node?
  }
  else
  {
    target->CopyWithSingleModifiedEvent(source);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerMultidimDataBrowserLogic::GetCompatibleNodesFromScene(vtkCollection* compatibleNodes, vtkMRMLMultidimDataNode* multidimDataRootNode)
{
  if (compatibleNodes==NULL)
  {
    vtkErrorMacro("vtkSlicerMultidimDataBrowserLogic::GetCompatibleNodesFromScene failed: compatibleNodes is invalid");
    return;
  }
  compatibleNodes->RemoveAllItems();
  if (multidimDataRootNode==NULL)
  {
    // if root node is invalid then there is no compatible node
    return;
  }
  if (this->GetMRMLScene()==NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
  if (multidimDataRootNode->GetDimensionName()==NULL)
  {
    vtkErrorMacro("vtkSlicerMultidimDataBrowserLogic::GetCompatibleNodesFromScene failed: root node dimension name is invalid");
    return;
  }
  std::string masterRootNodeDimension=multidimDataRootNode->GetDimensionName();
  vtkSmartPointer<vtkCollection> multidimNodes = vtkSmartPointer<vtkCollection>::Take(this->GetMRMLScene()->GetNodesByClass("vtkMRMLMultidimDataNode"));
  vtkObject* nextObject = NULL;
  for (multidimNodes->InitTraversal(); (nextObject = multidimNodes->GetNextItemAsObject()); )
  {
    vtkMRMLMultidimDataNode* multidimNode = vtkMRMLMultidimDataNode::SafeDownCast(nextObject);
    if (multidimNode==NULL)
    {
      continue;
    }
    if (multidimNode==multidimDataRootNode)
    {
      // do not add the master node itself to the list of compatible nodes
      continue;
    }
    if (multidimNode->GetDimensionName()==NULL)
    {
      vtkErrorMacro("vtkSlicerMultidimDataBrowserLogic::GetCompatibleNodesFromScene failed: potential compatible root node dimension name is invalid");
      continue;
    }
    if (masterRootNodeDimension.compare(multidimNode->GetDimensionName())==0)
    {
      // dimension name is matching, so we consider it compatible
      compatibleNodes->AddItem(multidimNode);
    }
  }
}
