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

//#define ENABLE_PERFORMANCE_PROFILING

// Sequence Logic includes
#include "vtkSlicerSequenceBrowserLogic.h"
#include "vtkMRMLSequenceBrowserNode.h"
#include "vtkMRMLSequenceNode.h"

// MRML includes
#include "vtkMRMLCameraNode.h"
#include "vtkMRMLLabelMapVolumeNode.h"
#include "vtkMRMLMarkupsFiducialNode.h"
#include "vtkMRMLModelNode.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLTransformNode.h"

// VTK includes
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkTimerLog.h>

// STL includes
#include <algorithm>

#ifdef ENABLE_PERFORMANCE_PROFILING
#include "vtkTimerLog.h"
#endif 

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerSequenceBrowserLogic);

//----------------------------------------------------------------------------
vtkSlicerSequenceBrowserLogic::vtkSlicerSequenceBrowserLogic()
: UpdateProxyNodesInProgress(false)
{
}

//----------------------------------------------------------------------------
vtkSlicerSequenceBrowserLogic::~vtkSlicerSequenceBrowserLogic()
{  
}

//----------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}

//---------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::RegisterNodes()
{
  if (this->GetMRMLScene()==NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLSequenceBrowserNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::UpdateFromMRMLScene()
{
  if (this->GetMRMLScene()==NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
  // TODO: probably we need to add observer to all vtkMRMLSequenceBrowserNode-type nodes
}

//---------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (node==NULL)
  {
    vtkErrorMacro("An invalid node is attempted to be added");
    return;
  }
  if (node->IsA("vtkMRMLSequenceBrowserNode"))
  {
    vtkDebugMacro("OnMRMLSceneNodeAdded: Have a vtkMRMLSequenceBrowserNode node");
    vtkUnObserveMRMLNodeMacro(node); // remove any previous observation that might have been added
    vtkObserveMRMLNodeMacro(node);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (node==NULL)
  {
    vtkErrorMacro("An invalid node is attempted to be removed");
    return;
  }
  if (node->IsA("vtkMRMLSequenceBrowserNode"))
  {
    vtkDebugMacro("OnMRMLSceneNodeRemoved: Have a vtkMRMLSequenceBrowserNode node");
    vtkUnObserveMRMLNodeMacro(node);
  } 
}

//---------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::UpdateAllProxyNodes()
{
  double updateStartTimeSec = vtkTimerLog::GetUniversalTime();

  vtkMRMLScene* scene=this->GetMRMLScene();
  if (scene==NULL)
  {
    vtkErrorMacro("vtkSlicerSequenceBrowserLogic::UpdateAllVirtualOutputNodes failed: scene is invalid");
    return;
  }
  std::vector< vtkMRMLNode* > browserNodes;
  int numBrowserNodes = this->GetMRMLScene()->GetNodesByClass("vtkMRMLSequenceBrowserNode", browserNodes);
  for (int i = 0; i < numBrowserNodes; i++)
  {
    vtkMRMLSequenceBrowserNode* browserNode = vtkMRMLSequenceBrowserNode::SafeDownCast(browserNodes[i]);
    if (browserNode==NULL)
    {
      vtkErrorMacro("Browser node is invalid");
      continue;
    }
    if (!browserNode->GetPlaybackActive())
    {
      this->LastSequenceBrowserUpdateTimeSec.erase(browserNode);
      continue;
    }
    if ( this->LastSequenceBrowserUpdateTimeSec.find(browserNode) == this->LastSequenceBrowserUpdateTimeSec.end() )
    {
      // we just started to play now, no need to update output nodes yet
      this->LastSequenceBrowserUpdateTimeSec[browserNode] = updateStartTimeSec;
      continue;
    }
    // play is already in progress
    double elapsedTimeSec = updateStartTimeSec - this->LastSequenceBrowserUpdateTimeSec[browserNode];
    // compute how many items we need to jump; if not enough time passed to jump at least to the next item
    // then we don't do anything (let the elapsed time cumulate)
    int selectionIncrement = floor(elapsedTimeSec * browserNode->GetPlaybackRateFps()+0.5); // floor with +0.5 is rounding
    if (selectionIncrement>0)
    {
      this->LastSequenceBrowserUpdateTimeSec[browserNode] = updateStartTimeSec;
      if (!browserNode->GetPlaybackItemSkippingEnabled())
      {
        selectionIncrement = 1;
      }
      browserNode->SelectNextItem(selectionIncrement);
    }
  }
}

//---------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::UpdateProxyNodes(vtkMRMLSequenceBrowserNode* browserNode)
{
#ifdef ENABLE_PERFORMANCE_PROFILING
  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();  
#endif 

  if (this->UpdateProxyNodesInProgress)
  {
    // avoid infinite loops (caused by triggering a node update during a node update)
    return;
  }
  if (browserNode==NULL)
  {
    vtkWarningMacro("vtkSlicerSequenceBrowserLogic::UpdateProxyNodes failed: browserNode is invalid");
    return;
  }
  
  if (browserNode->GetMasterSequenceNode()==NULL)
  {
    browserNode->RemoveAllProxyNodes();
    return;
  }

  vtkMRMLScene* scene=browserNode->GetScene();
  if (scene==NULL)
  {
    vtkErrorMacro("vtkSlicerSequenceBrowserLogic::UpdateVirtualOutputNodes failed: scene is invalid");
    return;
  }

  this->UpdateProxyNodesInProgress=true;
  
  int selectedItemNumber=browserNode->GetSelectedItemNumber();
  std::string indexValue;
  if (selectedItemNumber>=0)
  {
    indexValue=browserNode->GetMasterSequenceNode()->GetNthIndexValue(selectedItemNumber);
  }

  std::vector< vtkMRMLSequenceNode* > synchronizedSequenceNodes;
  browserNode->GetSynchronizedSequenceNodes(synchronizedSequenceNodes, true);
  
  // Store the previous modified state of nodes to allow calling EndModify when all the nodes are updated (to prevent multiple renderings on partial update)
  std::vector< std::pair<vtkMRMLNode*, int> > nodeModifiedStates;

  for (std::vector< vtkMRMLSequenceNode* >::iterator sourceSequenceNodeIt=synchronizedSequenceNodes.begin(); sourceSequenceNodeIt!=synchronizedSequenceNodes.end(); ++sourceSequenceNodeIt)
  {
    vtkMRMLSequenceNode* synchronizedSequenceNode=(*sourceSequenceNodeIt);
    if (synchronizedSequenceNode==NULL)
    {
      vtkErrorMacro("Synchronized sequence node is invalid");
      continue;
    }
    vtkMRMLNode* sourceDataNode=synchronizedSequenceNode->GetDataNodeAtValue(indexValue.c_str());
    if (sourceDataNode==NULL)
    {
      // no source node is available for the chosen time point
      continue;
    }
    
    // Get the current target output node
    vtkMRMLNode* targetProxyNode=browserNode->GetProxyNode(synchronizedSequenceNode);    
    if (targetProxyNode!=NULL)
    {
      // a virtual output node with the requested role exists already
      if (strcmp(targetProxyNode->GetClassName(),sourceDataNode->GetClassName())!=0)
      {
        // this node is of a different class, cannot be reused
        targetProxyNode=NULL;
      }
    }

    // Create the virtual output node (and display nodes) if it doesn't exist yet
    if (targetProxyNode==NULL)
    {
      // Get the display nodes      
      std::vector< vtkMRMLDisplayNode* > sourceDisplayNodes;
      synchronizedSequenceNode->GetDisplayNodesAtValue(sourceDisplayNodes, indexValue.c_str());

      // Add the new data and display nodes to the virtual outputs      
      targetProxyNode=browserNode->AddProxyNode(sourceDataNode,sourceDisplayNodes,synchronizedSequenceNode);
    }

    if (targetProxyNode==NULL)
    {
      // failed to add target output node
      continue;
    }

    // Get all the display nodes
    std::vector< vtkMRMLDisplayNode* > targetDisplayNodes;
    {
      vtkMRMLDisplayableNode* targetDisplayableNode=vtkMRMLDisplayableNode::SafeDownCast(targetProxyNode);
      if (targetDisplayableNode!=NULL)
      {
        int numOfDisplayNodes=targetDisplayableNode->GetNumberOfDisplayNodes();
        for (int displayNodeIndex=0; displayNodeIndex<numOfDisplayNodes; displayNodeIndex++)
        {
          targetDisplayNodes.push_back(targetDisplayableNode->GetNthDisplayNode(displayNodeIndex));
        }
      }
    }
    
    // Update the target node with the contents of the source node    

    // Slice browser is updated when there is a rename, but we want to avoid update, because
    // the source node may be hidden from editors and it would result in removing the target node
    // from the slicer browser. To avoid update of the slice browser, we set the name in advance.
    targetProxyNode->SetName(sourceDataNode->GetName());

    // Mostly it is a shallow copy (for example for volumes, models)
    std::pair<vtkMRMLNode*, int> nodeModifiedState(targetProxyNode, targetProxyNode->StartModify());
    nodeModifiedStates.push_back(nodeModifiedState);
    this->ShallowCopy(targetProxyNode, sourceDataNode);

    // Generation of target proxy node name: sequence node name (IndexName = IndexValue IndexUnit)
    const char* sequenceName=synchronizedSequenceNode->GetName();
    const char* indexName=synchronizedSequenceNode->GetIndexName();
    const char* unit=synchronizedSequenceNode->GetIndexUnit();
    std::string targetProxyNodeName;
    targetProxyNodeName+=(sequenceName?sequenceName:"?");
    targetProxyNodeName+=" [";
    if (indexName)
    {
      targetProxyNodeName+=indexName;
      targetProxyNodeName+="=";
    }
    targetProxyNodeName+=indexValue;
    if (unit)
    {
      targetProxyNodeName+=unit;
    }
    targetProxyNodeName+="]";
    targetProxyNode->SetName(targetProxyNodeName.c_str());

    vtkMRMLDisplayableNode* targetDisplayableNode=vtkMRMLDisplayableNode::SafeDownCast(targetProxyNode);
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
        // SetAndObserveNthDisplayNodeID takes a long time, only run it if the display node ptr changed
        if (targetDisplayableNode->GetNthDisplayNode(displayNodeIndex)!=targetDisplayNodes[displayNodeIndex])
        {
          targetDisplayableNode->SetAndObserveNthDisplayNodeID(displayNodeIndex,targetDisplayNodes[displayNodeIndex]->GetID());
        }
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

  // Finalize modifications, all at once. These will fire the node modified events and update renderers.
  for (std::vector< std::pair<vtkMRMLNode*, int> >::iterator nodeModifiedStateIt = nodeModifiedStates.begin(); nodeModifiedStateIt!=nodeModifiedStates.end(); ++nodeModifiedStateIt)
  {
    (nodeModifiedStateIt->first)->EndModify(nodeModifiedStateIt->second);
  }

  this->UpdateProxyNodesInProgress=false;

#ifdef ENABLE_PERFORMANCE_PROFILING
  timer->StopTimer();
  vtkInfoMacro("UpdateVirtualOutputNodes: " << timer->GetElapsedTime() << "sec\n");
#endif 
}

//---------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::AddSynchronizedNode(vtkMRMLNode* sNode, vtkMRMLNode* proxyNode, vtkMRMLNode* bNode)
{
  vtkMRMLSequenceNode* sequenceNode = vtkMRMLSequenceNode::SafeDownCast(sNode);
  vtkMRMLSequenceBrowserNode* browserNode = vtkMRMLSequenceBrowserNode::SafeDownCast(bNode);

  if (browserNode==NULL)
  {
    vtkWarningMacro("vtkSlicerSequenceBrowserLogic::AddSynchronizedNode failed: browser node is invalid");
    return;
  }

  // Create an empty sequence node if the sequence node is NULL
  if (sequenceNode==NULL)
  {
    sequenceNode = vtkMRMLSequenceNode::SafeDownCast( this->GetMRMLScene()->CreateNodeByClass("vtkMRMLSequenceNode") );
    this->GetMRMLScene()->AddNode( sequenceNode );
    sequenceNode->Delete(); // Can release the pointer - ownership has been transferred to the scene
    if (proxyNode!=NULL)
    {
      std::stringstream sequenceNodeName;
      sequenceNodeName << proxyNode->GetName() << "-Sequence";
      sequenceNode->SetName(sequenceNodeName.str().c_str());
    }
  }


  // Check if the sequence node to add is compatible with the master
  if (browserNode->GetMasterSequenceNode()!=NULL && strcmp(browserNode->GetMasterSequenceNode()->GetIndexName(), sequenceNode->GetIndexName())!=0)
  {
    vtkWarningMacro("vtkSlicerSequenceBrowserLogic::AddSynchronizedNode failed: incompatible index name");
    return; // Not compatible - exit
  }

  if (!browserNode->IsSynchronizedSequenceNodeID(sequenceNode->GetID(), true))
  {
    browserNode->AddSynchronizedSequenceNodeID(sequenceNode->GetID());
  }
  if (proxyNode!=NULL)
  {
    std::vector< vtkMRMLDisplayNode* > displayNodes;
    vtkMRMLDisplayableNode* virtualDisplayableNode = vtkMRMLDisplayableNode::SafeDownCast(proxyNode);
    if (virtualDisplayableNode != NULL)
    {
      for (int i=0; i<virtualDisplayableNode->GetNumberOfDisplayNodes(); i++)
      {
        displayNodes.push_back(virtualDisplayableNode->GetNthDisplayNode(i));
      }
    }
    browserNode->AddProxyNode(proxyNode, displayNodes, sequenceNode, false);
  }

}

//---------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::ProcessMRMLNodesEvents(vtkObject *caller, unsigned long event, void *vtkNotUsed(callData))
{
  vtkMRMLSequenceBrowserNode *browserNode = vtkMRMLSequenceBrowserNode::SafeDownCast(caller);
  if (browserNode==NULL)
  {
    vtkErrorMacro("Expected a vtkMRMLSequenceBrowserNode");
    return;
  }

  this->UpdateProxyNodes(browserNode);
}

//---------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::ShallowCopy(vtkMRMLNode* target, vtkMRMLNode* source)
{
  int oldModified=target->StartModify();
  if (target->IsA("vtkMRMLScalarVolumeNode"))
  {
    vtkMRMLScalarVolumeNode* targetScalarVolumeNode=vtkMRMLScalarVolumeNode::SafeDownCast(target);
    vtkMRMLScalarVolumeNode* sourceScalarVolumeNode=vtkMRMLScalarVolumeNode::SafeDownCast(source);
    // targetScalarVolumeNode->SetAndObserveTransformNodeID is not called, as we want to keep the currently applied transform
    vtkSmartPointer<vtkMatrix4x4> ijkToRasmatrix=vtkSmartPointer<vtkMatrix4x4>::New();
    sourceScalarVolumeNode->GetIJKToRASMatrix(ijkToRasmatrix);
    targetScalarVolumeNode->SetIJKToRASMatrix(ijkToRasmatrix);
    targetScalarVolumeNode->SetName(sourceScalarVolumeNode->GetName());
    targetScalarVolumeNode->SetAndObserveImageData(sourceScalarVolumeNode->GetImageData()); // invokes vtkMRMLVolumeNode::ImageDataModifiedEvent, which is not masked by StartModify
  }
  else if (target->IsA("vtkMRMLLabelMapVolumeNode"))
  {
    vtkMRMLLabelMapVolumeNode* targetLabelMapVolumeNode=vtkMRMLLabelMapVolumeNode::SafeDownCast(target);
    vtkMRMLLabelMapVolumeNode* sourceLabelMapVolumeNode=vtkMRMLLabelMapVolumeNode::SafeDownCast(source);
    // targetLabelMapVolumeNode->SetAndObserveTransformNodeID is not called, as we want to keep the currently applied transform
    vtkSmartPointer<vtkMatrix4x4> ijkToRasmatrix=vtkSmartPointer<vtkMatrix4x4>::New();
    sourceLabelMapVolumeNode->GetIJKToRASMatrix(ijkToRasmatrix);
    targetLabelMapVolumeNode->SetIJKToRASMatrix(ijkToRasmatrix);
    targetLabelMapVolumeNode->SetName(sourceLabelMapVolumeNode->GetName());
    targetLabelMapVolumeNode->SetAndObserveImageData(sourceLabelMapVolumeNode->GetImageData()); // invokes vtkMRMLVolumeNode::ImageDataModifiedEvent, which is not masked by StartModify
  }
  else if (target->IsA("vtkMRMLModelNode"))
  {
    vtkMRMLModelNode* targetModelNode=vtkMRMLModelNode::SafeDownCast(target);
    vtkMRMLModelNode* sourceModelNode=vtkMRMLModelNode::SafeDownCast(source);
    targetModelNode->SetAndObservePolyData(sourceModelNode->GetPolyData());
  }
  else if (target->IsA("vtkMRMLMarkupsFiducialNode"))
  {
    vtkMRMLMarkupsFiducialNode* targetMarkupsFiducialNode=vtkMRMLMarkupsFiducialNode::SafeDownCast(target);
    vtkMRMLMarkupsFiducialNode* sourceMarkupsFiducialNode=vtkMRMLMarkupsFiducialNode::SafeDownCast(source);
    targetMarkupsFiducialNode->Copy(sourceMarkupsFiducialNode);
  }
  else if (target->IsA("vtkMRMLTransformNode"))
  {
    vtkMRMLTransformNode* targetTransformNode=vtkMRMLTransformNode::SafeDownCast(target);
    vtkMRMLTransformNode* sourceTransformNode=vtkMRMLTransformNode::SafeDownCast(source);
    vtkAbstractTransform* transform=sourceTransformNode->GetTransformToParent();
    targetTransformNode->SetAndObserveTransformToParent(transform);
  }
  else if (target->IsA("vtkMRMLCameraNode"))
  {
    vtkMRMLCameraNode* targetCameraNode=vtkMRMLCameraNode::SafeDownCast(target);
    vtkMRMLCameraNode* sourceCameraNode=vtkMRMLCameraNode::SafeDownCast(source);
    int disabledModify = targetCameraNode->StartModify();
    targetCameraNode->SetPosition(sourceCameraNode->GetPosition());
    targetCameraNode->SetFocalPoint(sourceCameraNode->GetFocalPoint());
    targetCameraNode->SetViewUp(sourceCameraNode->GetViewUp());
    targetCameraNode->SetParallelProjection(sourceCameraNode->GetParallelProjection());
    targetCameraNode->SetParallelScale(sourceCameraNode->GetParallelScale());
    // Maybe the new position and focalpoint combo doesn't fit the existing
    // clipping range
    targetCameraNode->ResetClippingRange();
    targetCameraNode->EndModify(disabledModify);
  }
  else
  {
    // TODO: maybe get parent transform, display nodes, storage node, and restore it after the copy
    target->CopyWithSingleModifiedEvent(source);
  }
  target->EndModify(oldModified);
}

//---------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::GetCompatibleNodesFromScene(vtkCollection* compatibleNodes, vtkMRMLSequenceNode* masterSequenceNode)
{
  if (compatibleNodes==NULL)
  {
    vtkErrorMacro("vtkSlicerSequenceBrowserLogic::GetCompatibleNodesFromScene failed: compatibleNodes is invalid");
    return;
  }
  compatibleNodes->RemoveAllItems();
  if (masterSequenceNode==NULL)
  {
    // if sequence node is invalid then there is no compatible node
    return;
  }
  if (this->GetMRMLScene()==NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
  if (masterSequenceNode->GetIndexName()==NULL)
  {
    vtkErrorMacro("vtkSlicerSequenceBrowserLogic::GetCompatibleNodesFromScene failed: sequence node index name is invalid");
    return;
  }
  std::string masterSequenceNodeIndexName=masterSequenceNode->GetIndexName();
  vtkSmartPointer<vtkCollection> sequenceNodes = vtkSmartPointer<vtkCollection>::Take(this->GetMRMLScene()->GetNodesByClass("vtkMRMLSequenceNode"));
  vtkObject* nextObject = NULL;
  for (sequenceNodes->InitTraversal(); (nextObject = sequenceNodes->GetNextItemAsObject()); )
  {
    vtkMRMLSequenceNode* sequenceNode = vtkMRMLSequenceNode::SafeDownCast(nextObject);
    if (sequenceNode==NULL)
    {
      continue;
    }
    if (sequenceNode==masterSequenceNode)
    {
      // do not add the master node itself to the list of compatible nodes
      continue;
    }
    if (sequenceNode->GetIndexName()==NULL)
    {
      vtkErrorMacro("vtkSlicerSequenceBrowserLogic::GetCompatibleNodesFromScene failed: potential compatible sequence node index name is invalid");
      continue;
    }
    if (masterSequenceNodeIndexName.compare(sequenceNode->GetIndexName())==0)
    {
      // index name is matching, so we consider it compatible
      compatibleNodes->AddItem(sequenceNode);
    }
  }
}
