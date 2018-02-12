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
#include "vtkMRMLNodeSequencer.h"
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
#include <vtkImageData.h>
#include <vtkPolyData.h>
#include <vtkAbstractTransform.h>

// STL includes
#include <algorithm>

#ifdef ENABLE_PERFORMANCE_PROFILING
#include "vtkTimerLog.h"
#endif 

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerSequenceBrowserLogic);

//----------------------------------------------------------------------------
vtkSlicerSequenceBrowserLogic::vtkSlicerSequenceBrowserLogic()
  : UpdateProxyNodesFromSequencesInProgress(false)
  , UpdateSequencesFromProxyNodesInProgress(false)
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
    vtkNew<vtkIntArray> events;
    events->InsertNextValue(vtkMRMLSequenceBrowserNode::ProxyNodeModifiedEvent);
    events->InsertNextValue(vtkCommand::ModifiedEvent);
    vtkObserveMRMLNodeEventsMacro(node, events.GetPointer());
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
    vtkErrorMacro("vtkSlicerSequenceBrowserLogic::UpdateAllProxyNodes failed: scene is invalid");
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
void vtkSlicerSequenceBrowserLogic::UpdateProxyNodesFromSequences(vtkMRMLSequenceBrowserNode* browserNode)
{
#ifdef ENABLE_PERFORMANCE_PROFILING
  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();  
#endif 

  if (this->UpdateProxyNodesFromSequencesInProgress || this->UpdateSequencesFromProxyNodesInProgress)
  {
    // avoid infinite loops (caused by triggering a node update during a node update)
    return;
  }
  if (browserNode==NULL)
  {
    vtkWarningMacro("vtkSlicerSequenceBrowserLogic::UpdateProxyNodesFromSequences failed: browserNode is invalid");
    return;
  }

  if (browserNode->GetRecordingActive())
  {
    // don't update proxy nodes while recording (sequence is updated from proxy nodes)
    return;
  }
  
  if (browserNode->GetMasterSequenceNode() == NULL)
  {
    browserNode->RemoveAllProxyNodes();
    return;
  }

  vtkMRMLScene* scene=browserNode->GetScene();
  if (scene==NULL)
  {
    vtkErrorMacro("vtkSlicerSequenceBrowserLogic::UpdateProxyNodesFromSequences failed: scene is invalid");
    return;
  }

  this->UpdateProxyNodesFromSequencesInProgress = true;
  
  int selectedItemNumber=browserNode->GetSelectedItemNumber();
  std::string indexValue("0");
  if (selectedItemNumber >= 0 && selectedItemNumber < browserNode->GetNumberOfItems())
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
    if(!browserNode->GetPlayback(synchronizedSequenceNode))
    {
      continue;
    }

    vtkMRMLNode* sourceDataNode = NULL;
    if (browserNode->GetSaveChanges(synchronizedSequenceNode))
    {
      // we want to save changes, therefore we have to make sure a data node is available for the current index
      if (synchronizedSequenceNode->GetNumberOfDataNodes() > 0)
      {
        sourceDataNode = synchronizedSequenceNode->GetDataNodeAtValue(indexValue, true /*exact match*/);
        if (sourceDataNode == NULL)
        {
          // No source node is available for the current exact index.
          // Add a copy of the closest (previous) item into the sequence at the exact index.
          sourceDataNode = synchronizedSequenceNode->GetDataNodeAtValue(indexValue, false /*closest match*/);
          if (sourceDataNode)
          {
            sourceDataNode = synchronizedSequenceNode->SetDataNodeAtValue(sourceDataNode, indexValue);
          }
        }
      }
      else
      {
        // There are no data nodes in the sequence.
        // Insert the current proxy node in the sequence.
        sourceDataNode = browserNode->GetProxyNode(synchronizedSequenceNode);
        if (sourceDataNode)
        {
          sourceDataNode = synchronizedSequenceNode->SetDataNodeAtValue(sourceDataNode, indexValue);
        }
      }
    }
    else
    {
      // we just want to show a node, therefore we can just use closest data node
      sourceDataNode = synchronizedSequenceNode->GetDataNodeAtValue(indexValue, false /*closest match*/);
    }
    if (sourceDataNode==NULL)
    {
      // no source node is available
      continue;
    }
    
    // Get the current target output node
    vtkMRMLNode* targetProxyNode=browserNode->GetProxyNode(synchronizedSequenceNode);    
    if (targetProxyNode!=NULL)
    {
      // a proxy node with the requested role exists already
      if (strcmp(targetProxyNode->GetClassName(),sourceDataNode->GetClassName())!=0)
      {
        // this node is of a different class, cannot be reused
        targetProxyNode=NULL;
      }
    }

    bool newTargetProxyNodeWasCreated = false;
    if (targetProxyNode==NULL)
    {
      // Create the proxy node (and display nodes) if they don't exist yet
      targetProxyNode=browserNode->AddProxyNode(sourceDataNode, synchronizedSequenceNode);
      newTargetProxyNodeWasCreated = true;
    }

    if (targetProxyNode==NULL)
    {
      // failed to add target output node
      continue;
    }
    
    // Update the target node with the contents of the source node    

    // Mostly it is a shallow copy (for example for volumes, models)
    std::pair<vtkMRMLNode*, int> nodeModifiedState(targetProxyNode, targetProxyNode->StartModify());
    nodeModifiedStates.push_back(nodeModifiedState);

    // Save node references of proxy node
    // (otherwise parent transform, display node, etc. would be lost)
    vtkSmartPointer<vtkMRMLNode> proxyOriginalReferenceStorage = vtkSmartPointer<vtkMRMLNode>::Take(targetProxyNode->NewInstance());
    proxyOriginalReferenceStorage->CopyReferences(targetProxyNode);
    std::string proxyOriginalName = (targetProxyNode->GetName() ? targetProxyNode->GetName() : "");

    // TODO: if we really want to force non-mutable nodes in the sequence then we have to deep-copy, but that's slow.
    // Make sure that by default/most of the time shallow-copy is used.
    bool shallowCopy = browserNode->GetSaveChanges(synchronizedSequenceNode);
    vtkMRMLNodeSequencer::GetInstance()->GetNodeSequencer(targetProxyNode)->CopyNodeReplay(sourceDataNode, targetProxyNode, shallowCopy);

    // Restore node references
    targetProxyNode->CopyReferences(proxyOriginalReferenceStorage);

    if (targetProxyNode->GetSingletonTag())
    {
      // Singleton nodes must not be renamed, as they are often expected to exist by a specific name
      targetProxyNode->SetName(proxyOriginalName.c_str());
    }
    else if (browserNode->GetOverwriteProxyName(synchronizedSequenceNode))
    {
      // Generation of target proxy node name: base node name [IndexName = IndexValue IndexUnit]
      std::string indexName = synchronizedSequenceNode->GetIndexName();
      std::string unit = synchronizedSequenceNode->GetIndexUnit();
      // Save the base name (without the index name and value)
      targetProxyNode->SetAttribute("Sequences.BaseName", synchronizedSequenceNode->GetName());
      std::string targetProxyNodeName = synchronizedSequenceNode->GetName();
      targetProxyNodeName+=" [";
      if (!indexName.empty())
      {
        targetProxyNodeName+=indexName;
        targetProxyNodeName+="=";
      }
      targetProxyNodeName+=indexValue;
      if (!unit.empty())
      {
        targetProxyNodeName+=unit;
      }
      targetProxyNodeName+="]";
      targetProxyNode->SetName(targetProxyNodeName.c_str());
    }

    if (newTargetProxyNodeWasCreated)
    {
      vtkMRMLNodeSequencer::GetInstance()->GetNodeSequencer(targetProxyNode)->AddDefaultDisplayNodes(targetProxyNode);
      // Add default storage node now to avoid proxy node update when "Add data" dialog is invoked
      vtkMRMLStorableNode* storableNode = vtkMRMLStorableNode::SafeDownCast(targetProxyNode);
      if (storableNode)
      {
        storableNode->AddDefaultStorageNode();
      }
    }
  }

  // Finalize modifications, all at once. These will fire the node modified events and update renderers.
  for (std::vector< std::pair<vtkMRMLNode*, int> >::iterator nodeModifiedStateIt = nodeModifiedStates.begin(); nodeModifiedStateIt!=nodeModifiedStates.end(); ++nodeModifiedStateIt)
  {
    (nodeModifiedStateIt->first)->EndModify(nodeModifiedStateIt->second);
  }

  this->UpdateProxyNodesFromSequencesInProgress = false;

#ifdef ENABLE_PERFORMANCE_PROFILING
  timer->StopTimer();
  vtkInfoMacro("UpdateProxyNodesFromSequences: " << timer->GetElapsedTime() << "sec\n");
#endif 
}

//---------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::UpdateSequencesFromProxyNodes(vtkMRMLSequenceBrowserNode* browserNode, vtkMRMLNode* proxyNode)
{
  if (this->UpdateProxyNodesFromSequencesInProgress || this->UpdateSequencesFromProxyNodesInProgress)
  {
    // this update is due to updating from sequence nodes
    return;
  }
  if (browserNode->GetPlaybackActive())
  {
    // don't accept node modifications while replaying
    return;
  }
  if (!browserNode)
  {
    vtkErrorMacro("vtkSlicerSequenceBrowserLogic::UpdateSequencesFromProxyNodes failed: invlid browser node");
    return;
  }
  vtkMRMLSequenceNode *masterNode = browserNode->GetMasterSequenceNode();
  if (!masterNode)
  {
    vtkErrorMacro("Cannot record node modification: master sequence node is invalid");
    return;
  }

  if (!proxyNode)
  {
    vtkErrorMacro("vtkSlicerSequenceBrowserLogic::UpdateSequencesFromProxyNodes: update if all proxy nodes is not implemented yet");
    // TODO: update all proxy nodes if there is no info about which one was modified
    return;
  }

  this->UpdateSequencesFromProxyNodesInProgress = true;

  if (browserNode->GetRecordingActive())
  {
    // Record proxy node state into new sequence item
    // If we only record when the master node is modified, then skip if it was not the master node that was modified
    bool saveState = false;
    if (browserNode->GetRecordMasterOnly())
    {
      vtkMRMLNode* masterProxyNode = browserNode->GetProxyNode(masterNode);
      if (masterProxyNode != NULL && masterProxyNode->GetID() != NULL
        && strcmp(proxyNode->GetID(), masterProxyNode->GetID()) == 0)
      {
        // master proxy node is changed
        saveState = true;
      }
    }
    else
    {
      // if we don't record on master node changes only then we always save the state
      saveState = true;
    }
    if (saveState)
    {
      browserNode->SaveProxyNodesState();
    }
  }
  else
  {
    // Update sequence item from proxy node
    vtkMRMLSequenceNode* sequenceNode = browserNode->GetSequenceNode(proxyNode);
    if (sequenceNode && browserNode->GetSaveChanges(sequenceNode) && browserNode->GetSelectedItemNumber()>=0)
    {
      std::string indexValue = masterNode->GetNthIndexValue(browserNode->GetSelectedItemNumber());
      int closestItemNumber = sequenceNode->GetItemNumberFromIndexValue(indexValue, false);
      if (closestItemNumber >= 0)
      {
        std::string closestIndexValue = sequenceNode->GetNthIndexValue(closestItemNumber);
        sequenceNode->UpdateDataNodeAtValue(proxyNode, indexValue, true /* shallow copy*/);
      }
    }
  }
  this->UpdateSequencesFromProxyNodesInProgress = false;
}

//---------------------------------------------------------------------------
vtkMRMLSequenceNode* vtkSlicerSequenceBrowserLogic::AddSynchronizedNode(vtkMRMLNode* sNode, vtkMRMLNode* proxyNode, vtkMRMLNode* bNode)
{
  vtkMRMLSequenceNode* sequenceNode = vtkMRMLSequenceNode::SafeDownCast(sNode);
  vtkMRMLSequenceBrowserNode* browserNode = vtkMRMLSequenceBrowserNode::SafeDownCast(bNode);

  if (browserNode==NULL)
  {
    vtkWarningMacro("vtkSlicerSequenceBrowserLogic::AddSynchronizedNode failed: browser node is invalid");
    return NULL;
  }

  // Be consistent with removing synchronized sequence node - stop playback/recording
  browserNode->SetPlaybackActive(false);
  browserNode->SetRecordingActive(false);

  // Create an empty sequence node if the sequence node is NULL
  if (sequenceNode==NULL)
  {
    sequenceNode = vtkMRMLSequenceNode::SafeDownCast( this->GetMRMLScene()->CreateNodeByClass("vtkMRMLSequenceNode") );
    if (browserNode->GetMasterSequenceNode() != NULL)
    {
      // Make the new sequence node compatible with the current master node
      sequenceNode->SetIndexName(browserNode->GetMasterSequenceNode()->GetIndexName());
      sequenceNode->SetIndexUnit(browserNode->GetMasterSequenceNode()->GetIndexUnit());
    }
    this->GetMRMLScene()->AddNode(sequenceNode);
    sequenceNode->Delete(); // Can release the pointer - ownership has been transferred to the scene
    if (proxyNode!=NULL)
    {
      std::stringstream sequenceNodeName;
      sequenceNodeName << proxyNode->GetName() << "-Sequence";
      sequenceNode->SetName(sequenceNodeName.str().c_str());
    }
  }

  // Check if the sequence node to add is compatible with the master
  if (browserNode->GetMasterSequenceNode() != NULL
    && !IsNodeCompatibleForBrowsing(browserNode->GetMasterSequenceNode(), sequenceNode) )
  {
    vtkWarningMacro("vtkSlicerSequenceBrowserLogic::AddSynchronizedNode failed: incompatible index name or unit");
    return NULL; // Not compatible - exit
  }

  if (!browserNode->IsSynchronizedSequenceNodeID(sequenceNode->GetID(), true))
  {
    browserNode->AddSynchronizedSequenceNodeID(sequenceNode->GetID());
  }
  if (proxyNode!=NULL)
  {
    browserNode->AddProxyNode(proxyNode, sequenceNode, false);
    vtkMRMLNodeSequencer::GetInstance()->GetNodeSequencer(proxyNode)->AddDefaultDisplayNodes(proxyNode);
  }
  return sequenceNode;
}

//---------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::ProcessMRMLNodesEvents(vtkObject *caller, unsigned long event, void *callData)
{
  vtkMRMLSequenceBrowserNode *browserNode = vtkMRMLSequenceBrowserNode::SafeDownCast(caller);
  if (browserNode==NULL)
  {
    vtkErrorMacro("Expected a vtkMRMLSequenceBrowserNode");
    return;
  }
  if (event == vtkCommand::ModifiedEvent)
  {
    // Browser node modified (e.g., switched to next frame), update proxy nodes
    this->UpdateProxyNodesFromSequences(browserNode);
  }
  else if (event == vtkMRMLSequenceBrowserNode::ProxyNodeModifiedEvent)
  {
    // During import proxy node may change but we don't want to modify the sequence node with it
    // because the saved proxy node might be obsolete (for example, not saved when the scene was saved).
    // It might be useful to update all proxy nodes on SceneEndImport/Restore to make sure the state is consistent.
    if (this->GetMRMLScene() &&
      !this->GetMRMLScene()->IsImporting() &&
      !this->GetMRMLScene()->IsRestoring())
    {
      // One of the proxy nodes changed, update the sequence as needed
      this->UpdateSequencesFromProxyNodes(browserNode, vtkMRMLNode::SafeDownCast((vtkObject*)callData));
    }
  }
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
    if (IsNodeCompatibleForBrowsing(masterSequenceNode, sequenceNode))
    {
      compatibleNodes->AddItem(sequenceNode);
    }
  }
}

//---------------------------------------------------------------------------
bool vtkSlicerSequenceBrowserLogic::IsNodeCompatibleForBrowsing(vtkMRMLSequenceNode* masterNode, vtkMRMLSequenceNode* testedNode)
{
  bool compatible = (masterNode->GetIndexName() == testedNode->GetIndexName()
    && masterNode->GetIndexUnit() == testedNode->GetIndexUnit()
    && masterNode->GetIndexType() == testedNode->GetIndexType());
  return compatible;
}

//---------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::GetBrowserNodesForSequenceNode(vtkMRMLSequenceNode* sequenceNode, vtkCollection* foundBrowserNodes)
{
  if (this->GetMRMLScene() == NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
  if (foundBrowserNodes == NULL)
  {
    vtkErrorMacro("foundBrowserNodes is invalid");
    return;
  }
  foundBrowserNodes->RemoveAllItems();
  std::vector< vtkMRMLNode* > browserNodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLSequenceBrowserNode", browserNodes);
  for (std::vector< vtkMRMLNode* >::iterator browserNodeIt = browserNodes.begin(); browserNodeIt != browserNodes.end(); ++browserNodeIt)
  {
    vtkMRMLSequenceBrowserNode* browserNode = vtkMRMLSequenceBrowserNode::SafeDownCast(*browserNodeIt);
    if (browserNode->IsSynchronizedSequenceNode(sequenceNode, true))
    {
      foundBrowserNodes->AddItem(browserNode);
    }
  }      
}

 ;

//---------------------------------------------------------------------------
vtkMRMLSequenceBrowserNode* vtkSlicerSequenceBrowserLogic::GetFirstBrowserNodeForSequenceNode(vtkMRMLSequenceNode* sequenceNode)
{
  if (this->GetMRMLScene() == NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return NULL;
  }
  std::vector< vtkMRMLNode* > browserNodes;
  this->GetMRMLScene()->GetNodesByClass("vtkMRMLSequenceBrowserNode", browserNodes);
  for (std::vector< vtkMRMLNode* >::iterator browserNodeIt = browserNodes.begin(); browserNodeIt != browserNodes.end(); ++browserNodeIt)
  {
    vtkMRMLSequenceBrowserNode* browserNode = vtkMRMLSequenceBrowserNode::SafeDownCast(*browserNodeIt);
    if (browserNode->IsSynchronizedSequenceNode(sequenceNode, true))
    {
      return browserNode;
    }
  }
  return NULL;
}
