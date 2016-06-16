/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// MRMLSequence includes
#include "vtkMRMLSequenceBrowserNode.h"
#include "vtkMRMLSequenceNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLHierarchyNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>

// VTK includes
#include <vtkNew.h>
#include <vtkIntArray.h>
#include <vtkCommand.h>
#include <vtkCollection.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkTimerLog.h>

// STD includes
#include <sstream>
#include <algorithm> // for std::find

// First reference is the master sequence node, subsequent references are the synchronized sequence nodes
static const char* SEQUENCE_NODE_REFERENCE_ROLE_BASE = "sequenceNodeRef"; // Old: rootNodeRef
static const char* DATA_NODE_REFERENCE_ROLE_BASE = "dataNodeRef";
static const char* DISPLAY_NODES_REFERENCE_ROLE_BASE = "displayNodesRef";
static const char* SYNCHRONIZED_SEQUENCE_NODE_REFERENCE_ROLE_BASE = "syncedSequenceNodeRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLSequenceBrowserNode);

//----------------------------------------------------------------------------
vtkMRMLSequenceBrowserNode::vtkMRMLSequenceBrowserNode()
{
  this->SetHideFromEditors(false);
  this->PlaybackActive = false;
  this->PlaybackRateFps = 10.0;
  this->PlaybackItemSkippingEnabled = true;
  this->PlaybackLooped = true;
  this->SelectedItemNumber = 0;

  this->RecordMasterOnly = false;
  this->RecordingActive = false;
  this->InitialTime = vtkTimerLog::GetUniversalTime();

  this->LastPostfixIndex = 0;

  // TODO: Is this list of events complete. Is there a less arbitrary way to come up with the list of events
  // vtkCommand::AnyEvent grabs all of the events, but is very slow
  //this->RecordingEvents->InsertNextValue( vtkCommand::ModifiedEvent );
  this->RecordingEvents->InsertNextValue( vtkMRMLTransformableNode::TransformModifiedEvent );
  this->RecordingEvents->InsertNextValue( vtkMRMLVolumeNode::ImageDataModifiedEvent );
}

//----------------------------------------------------------------------------
vtkMRMLSequenceBrowserNode::~vtkMRMLSequenceBrowserNode()
{
 
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  vtkIndent indent(nIndent);

  of << indent << " playbackActive=\"" << (this->PlaybackActive ? "true" : "false") << "\"";
  of << indent << " playbackRateFps=\"" << this->PlaybackRateFps << "\""; 
  of << indent << " playbackItemSkippingEnabled=\"" << (this->PlaybackItemSkippingEnabled ? "true" : "false") << "\"";
  of << indent << " playbackLooped=\"" << (this->PlaybackLooped ? "true" : "false") << "\"";  
  of << indent << " selectedItemNumber=\"" << this->SelectedItemNumber << "\"";
  of << indent << " recordingActive=\"" << (this->RecordingActive ? "true" : "false") << "\"";

  of << indent << " virtualNodePostfixes=\"";
  for(std::vector< std::string >::iterator roleNameIt=this->VirtualNodePostfixes.begin();
    roleNameIt!=this->VirtualNodePostfixes.end(); ++roleNameIt)
  { 
    if (roleNameIt!=this->VirtualNodePostfixes.begin())
    {
      // print separator before printing the (if not the first element)
      of << " ";
    }
    of << roleNameIt->c_str();
  }
  of << "\"";

}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName;
  const char* attValue;
  while (*atts != NULL) 
  {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "playbackActive")) 
    {
      if (!strcmp(attValue,"true")) 
      {
        this->SetPlaybackActive(1);
      }
      else
      {
        this->SetPlaybackActive(0);
      }
    }
    else if (!strcmp(attName, "playbackRateFps")) 
    {
      std::stringstream ss;
      ss << attValue;
      double playbackRateFps=10;
      ss >> playbackRateFps;
      this->SetPlaybackRateFps(playbackRateFps);
    }
    else if (!strcmp(attName, "playbackItemSkippingEnabled"))
    {
      if (!strcmp(attValue, "true"))
      {
        this->SetPlaybackItemSkippingEnabled(1);
      }
      else
      {
        this->SetPlaybackItemSkippingEnabled(0);
      }
    }
    else if (!strcmp(attName, "playbackLooped")) 
    {
      if (!strcmp(attValue,"true")) 
      {
        this->SetPlaybackLooped(1);
      }
      else
      {
        this->SetPlaybackLooped(0);
      }
    }
    else if (!strcmp(attName, "selectedItemNumber")) 
    {
      std::stringstream ss;
      ss << attValue;
      int selectedItemNumber=0;
      ss >> selectedItemNumber;
      this->SetSelectedItemNumber(selectedItemNumber);
    }
    else if (!strcmp(attName, "recordingActive")) 
    {
      if (!strcmp(attValue,"true")) 
      {
        this->SetRecordingActive(1);
      }
      else
      {
        this->SetRecordingActive(0);
      }
    }
    else if (!strcmp(attName, "virtualNodePostfixes"))
    {
      this->VirtualNodePostfixes.clear();
      std::stringstream ss(attValue);
      while (!ss.eof())
      {
        std::string roleName;
        ss >> roleName;
        if (!roleName.empty())
        {
          this->VirtualNodePostfixes.push_back(roleName);
        }
      }
    }
  }
  this->FixSequenceNodeReferenceRoleName();
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLSequenceBrowserNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  vtkMRMLSequenceBrowserNode* node=vtkMRMLSequenceBrowserNode::SafeDownCast(anode);
  if (node==NULL)
  {
    vtkErrorMacro("Node copy failed: not a vtkMRMLSequenceNode");
    return;
  }
  this->VirtualNodePostfixes=node->VirtualNodePostfixes;
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << " Playback active: " << (this->PlaybackActive ? "true" : "false") << '\n';
  os << indent << " Playback rate (fps): " << this->PlaybackRateFps << '\n';
  os << indent << " Playback item skipping enabled: " << (this->PlaybackItemSkippingEnabled ? "true" : "false") << '\n';
  os << indent << " Playback looped: " << (this->PlaybackLooped ? "true" : "false") << '\n';
  os << indent << " Selected item number: " << this->SelectedItemNumber << '\n';
  os << indent << " Recording active: " << (this->RecordingActive ? "true" : "false") << '\n';
}

//----------------------------------------------------------------------------
std::string vtkMRMLSequenceBrowserNode::GenerateVirtualNodePostfix()
{
  while (1)
  {
    std::stringstream postfix;
    postfix << this->LastPostfixIndex;
    this->LastPostfixIndex++;
    bool isUnique=(std::find(this->VirtualNodePostfixes.begin(), this->VirtualNodePostfixes.end(), postfix.str())==this->VirtualNodePostfixes.end());
    if (isUnique)
    {
      return postfix.str();
    }
  };
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::SetAndObserveMasterSequenceNodeID(const char *sequenceNodeID)
{
  bool oldModify=this->StartModify();
  std::string rolePostfix;
  if (!this->VirtualNodePostfixes.empty())
  {
    this->RemoveAllSequenceNodes();
  }
  if (sequenceNodeID!=NULL)
  {
    rolePostfix=GenerateVirtualNodePostfix();
    this->VirtualNodePostfixes.push_back(rolePostfix);
    std::string sequenceNodeReferenceRole=SEQUENCE_NODE_REFERENCE_ROLE_BASE+rolePostfix;
    this->SetAndObserveNodeReferenceID(sequenceNodeReferenceRole.c_str(), sequenceNodeID);
    // The master node is necessarily recorded and played back
    this->SequenceNodeSynchronizationTypeOn(sequenceNodeID, SynchronizationTypes::Playback);
    this->SequenceNodeSynchronizationTypeOn(sequenceNodeID, SynchronizationTypes::Recording);
  }
  this->EndModify(oldModify);
}

//----------------------------------------------------------------------------
vtkMRMLSequenceNode* vtkMRMLSequenceBrowserNode::GetMasterSequenceNode()
{
  if (this->VirtualNodePostfixes.empty())
  {
    return NULL;
  }  
  std::string sequenceNodeReferenceRole=SEQUENCE_NODE_REFERENCE_ROLE_BASE+this->VirtualNodePostfixes[0];  
  vtkMRMLSequenceNode* node=vtkMRMLSequenceNode::SafeDownCast(this->GetNodeReference(sequenceNodeReferenceRole.c_str()));
  return node;
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::RemoveAllVirtualOutputNodes()
{
  bool oldModify=this->StartModify();
  for (std::vector< std::string >::iterator rolePostfixIt=this->VirtualNodePostfixes.begin();
    rolePostfixIt!=this->VirtualNodePostfixes.end(); ++rolePostfixIt)
  {
    this->RemoveVirtualOutputDataNode(*rolePostfixIt);
    this->RemoveVirtualOutputDisplayNodes(*rolePostfixIt);
  }
  this->EndModify(oldModify);
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::RemoveAllSequenceNodes()
{
  bool oldModify=this->StartModify();
  // need to make a copy as this->VirtualNodePostfixes changes as we remove nodes
  std::vector< std::string > virtualNodePostfixes=this->VirtualNodePostfixes;
  // start from the end to delete the master sequence node last
  for (std::vector< std::string >::reverse_iterator rolePostfixIt=virtualNodePostfixes.rbegin();
    rolePostfixIt!=virtualNodePostfixes.rend(); ++rolePostfixIt)
  {
    std::string sequenceNodeReferenceRole=SEQUENCE_NODE_REFERENCE_ROLE_BASE+(*rolePostfixIt);
    vtkMRMLSequenceNode* node=vtkMRMLSequenceNode::SafeDownCast(this->GetNodeReference(sequenceNodeReferenceRole.c_str()));
    if (node==NULL)
    {
      vtkErrorMacro("Invalid sequence node");
      std::vector< std::string >::iterator rolePostfixInOriginalIt=std::find(this->VirtualNodePostfixes.begin(), this->VirtualNodePostfixes.end(), (*rolePostfixIt));
      if (rolePostfixInOriginalIt!=this->VirtualNodePostfixes.end())
      {
        this->VirtualNodePostfixes.erase(rolePostfixInOriginalIt);
      }
      continue;
    }
    this->RemoveSynchronizedSequenceNode(node->GetID());
  }
  this->EndModify(oldModify);
}


//----------------------------------------------------------------------------
std::string vtkMRMLSequenceBrowserNode::GetVirtualNodePostfixFromSequence(vtkMRMLSequenceNode* sequenceNode)
{
  if (sequenceNode==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::GetVirtualNodePostfixFromSequence failed: sequenceNode is invalid");
    return "";
  }
  for (std::vector< std::string >::iterator rolePostfixIt=this->VirtualNodePostfixes.begin();
    rolePostfixIt!=this->VirtualNodePostfixes.end(); ++rolePostfixIt)
  {
    std::string dataNodeRef=SEQUENCE_NODE_REFERENCE_ROLE_BASE+(*rolePostfixIt);
    if (this->GetNodeReference(dataNodeRef.c_str())==sequenceNode)
    {
      return (*rolePostfixIt);
    }
  }
  return "";
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLSequenceBrowserNode::GetVirtualOutputDataNode(vtkMRMLSequenceNode* sequenceNode)
{
  if (sequenceNode==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::GetVirtualOutputNode failed: sequenceNode is invalid");
    return NULL;
  }
  std::string rolePostfix=GetVirtualNodePostfixFromSequence(sequenceNode);
  if (rolePostfix.empty())
  {
    return NULL;
  }
  std::string dataNodeRef=DATA_NODE_REFERENCE_ROLE_BASE+rolePostfix;
  return this->GetNodeReference(dataNodeRef.c_str());
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::GetVirtualOutputDisplayNodes(vtkMRMLSequenceNode* sequenceNode, std::vector< vtkMRMLDisplayNode* > &displayNodes)
{
  displayNodes.clear();
  vtkMRMLNode* dataNode=GetVirtualOutputDataNode(sequenceNode);
  if (dataNode==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::GetVirtualOutputDisplayNodes failed: sequenceNode is invalid");
    return;
  }
  vtkMRMLDisplayableNode* displayableNode=vtkMRMLDisplayableNode::SafeDownCast(dataNode);
  if (displayableNode==NULL)
  {
    // not a displayable node so there are no display nodes
    return;
  }
  int numOfDisplayNodes=displayableNode->GetNumberOfDisplayNodes();
  for (int displayNodeIndex=0; displayNodeIndex<numOfDisplayNodes; displayNodeIndex++)
  {
    vtkMRMLDisplayNode* displayNode=vtkMRMLDisplayNode::SafeDownCast(displayableNode->GetNthDisplayNode(displayNodeIndex));
    if (displayNode==NULL)
    {
      continue;
    }
    displayNodes.push_back(displayNode);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::GetVirtualOutputDisplayNodes(vtkMRMLSequenceNode* sequenceNode, vtkCollection* displayNodes)
{
  if (displayNodes==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::GetVirtualOutputDisplayNodes failed: displayNodes is invalid");
    return;
  }
  std::vector< vtkMRMLDisplayNode* > displayNodesVector;
  this->GetVirtualOutputDisplayNodes(sequenceNode, displayNodesVector);
  displayNodes->RemoveAllItems();
  for (std::vector< vtkMRMLDisplayNode* >::iterator it = displayNodesVector.begin(); it != displayNodesVector.end(); ++it)
  {
    displayNodes->AddItem(*it);
  }
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLSequenceBrowserNode::AddVirtualOutputNodes(vtkMRMLNode* sourceDataNode, std::vector< vtkMRMLDisplayNode* > &sourceDisplayNodes, vtkMRMLSequenceNode* sequenceNode, bool copy)
{
  if (sequenceNode==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::AddVirtualOutputNode failed: sequenceNode is invalid");
    return NULL;
  }
  if (this->Scene==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::AddVirtualOutputNode failed: scene is invalid");
    return NULL;
  }

  bool oldModify=this->StartModify();
  
  std::string rolePostfix=GetVirtualNodePostfixFromSequence(sequenceNode);
  if (rolePostfix.empty())
  {
    // Add reference to the sequence node
    rolePostfix=AddSynchronizedSequenceNodeID(sequenceNode->GetID());
  }

  // Add copy of the data node
  std::string dataNodeRef=DATA_NODE_REFERENCE_ROLE_BASE+rolePostfix;
  // Create a new one from scratch in the new scene to make sure only the needed parts are copied
  vtkMRMLNode* dataNode = sourceDataNode;
  if ( copy )
  {
    dataNode = sourceDataNode->CreateNodeInstance();
    this->Scene->AddNode(dataNode);
    dataNode->Delete(); // ownership transferred to the scene, so we can release the pointer
  }

  this->AddNodeReferenceRole(dataNodeRef.c_str());
  this->SetAndObserveNodeReferenceID(dataNodeRef.c_str(), dataNode->GetID(), this->RecordingEvents.GetPointer());
  vtkMRMLDisplayableNode* displayableNode=vtkMRMLDisplayableNode::SafeDownCast(dataNode);
  
  // Add copy of the display node(s)
  std::string displayNodesRef=DISPLAY_NODES_REFERENCE_ROLE_BASE+rolePostfix;
  this->RemoveVirtualOutputDisplayNodes(rolePostfix);
  for (std::vector< vtkMRMLDisplayNode* >::iterator sourceDisplayNodeIt=sourceDisplayNodes.begin(); sourceDisplayNodeIt!=sourceDisplayNodes.end(); ++sourceDisplayNodeIt)
  {
    vtkMRMLDisplayNode* sourceDisplayNode=(*sourceDisplayNodeIt);
    vtkMRMLDisplayNode* displayNode = sourceDisplayNode;
    if (copy)
    {
      displayNode = vtkMRMLDisplayNode::SafeDownCast(this->Scene->CopyNode(sourceDisplayNode));
    }
    if (displayNode) // Added to check if displayNode is valid
    {
      this->AddNodeReferenceID(displayNodesRef.c_str(), displayNode->GetID());
      if (displayableNode)
      {
        displayableNode->AddAndObserveDisplayNodeID(displayNode->GetID());
      }
    }
  }

  this->EndModify(oldModify);
  return dataNode;
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::GetAllVirtualOutputDataNodes(std::vector< vtkMRMLNode* >& nodes)
{
  nodes.clear();
  for (std::vector< std::string >::iterator rolePostfixIt=this->VirtualNodePostfixes.begin();
    rolePostfixIt!=this->VirtualNodePostfixes.end(); ++rolePostfixIt)
  {
    std::string dataNodeRef=DATA_NODE_REFERENCE_ROLE_BASE+(*rolePostfixIt);
    vtkMRMLNode* node = this->GetNodeReference(dataNodeRef.c_str());
    if (node==NULL)
    {
      continue;
    }
    nodes.push_back(node);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::GetAllVirtualOutputDataNodes(vtkCollection* nodes)
{
  if (nodes==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::GetAllVirtualOutputDataNodes failed: nodes is invalid");
    return;
  }
  std::vector< vtkMRMLSequenceNode* > nodesVector;
  this->GetSynchronizedSequenceNodes(nodesVector);
  nodes->RemoveAllItems();
  for (std::vector< vtkMRMLSequenceNode* >::iterator it = nodesVector.begin(); it != nodesVector.end(); ++it)
  {
    nodes->AddItem(*it);
  }
}

//----------------------------------------------------------------------------
bool vtkMRMLSequenceBrowserNode::IsVirtualOutputDataNode(const char* nodeId)
{
  return this->IsVirtualOutputDataNodeID(nodeId);
}

//----------------------------------------------------------------------------
bool vtkMRMLSequenceBrowserNode::IsVirtualOutputDataNodeID(const char* nodeId)
{
  std::vector< vtkMRMLNode* > nodesVector;
  this->GetAllVirtualOutputDataNodes(nodesVector);
  for (std::vector< vtkMRMLNode* >::iterator it = nodesVector.begin(); it != nodesVector.end(); ++it)
  {
    if (strcmp((*it)->GetID(), nodeId)==0)
    {
      // found node
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::RemoveVirtualOutputDataNode(const std::string& postfix)
{
  bool oldModify=this->StartModify();
  std::string dataNodeRef=DATA_NODE_REFERENCE_ROLE_BASE+postfix;
  vtkMRMLNode* dataNode=this->GetNodeReference(dataNodeRef.c_str());
  if (dataNode!=NULL)
  {
    this->Scene->RemoveNode(dataNode);
    this->RemoveNodeReferenceIDs(dataNodeRef.c_str());
  }
  this->EndModify(oldModify);
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::RemoveVirtualOutputDisplayNodes(const std::string& postfix)
{
  bool oldModify=this->StartModify();
  std::vector< vtkMRMLNode* > displayNodes;
  std::string displayNodesRef=DISPLAY_NODES_REFERENCE_ROLE_BASE+postfix;
  GetNodeReferences(displayNodesRef.c_str(), displayNodes);
  for (std::vector< vtkMRMLNode* >::iterator displayNodeIt=displayNodes.begin(); displayNodeIt!=displayNodes.end(); ++displayNodeIt)
  {
    this->Scene->RemoveNode(*displayNodeIt);
  }
  this->RemoveNodeReferenceIDs(displayNodesRef.c_str());
  this->EndModify(oldModify);
}

//----------------------------------------------------------------------------
bool vtkMRMLSequenceBrowserNode::IsSynchronizedSequenceNode(const char* nodeId, bool includeMasterNode/*=false*/)
{
  return this->IsSynchronizedSequenceNodeID(nodeId, includeMasterNode);
}

//----------------------------------------------------------------------------
bool vtkMRMLSequenceBrowserNode::IsSynchronizedSequenceNode(vtkMRMLSequenceNode* sequenceNode, bool includeMasterNode/*=false*/)
{
  if (sequenceNode == NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::IsSynchronizedSequenceNode failed: sequenceNode is invalid");
    return false;
  }
  return this->IsSynchronizedSequenceNodeID(sequenceNode->GetID(), includeMasterNode);
}

//----------------------------------------------------------------------------
bool vtkMRMLSequenceBrowserNode::IsSynchronizedSequenceNodeID(const char* nodeId, bool includeMasterNode/*=false*/)
{
  if (nodeId==NULL)
  {
    vtkWarningMacro("vtkMRMLSequenceBrowserNode::IsSynchronizedSequenceNode nodeId is NULL");
    return false;
  }
  for (std::vector< std::string >::iterator rolePostfixIt=this->VirtualNodePostfixes.begin();
    rolePostfixIt!=this->VirtualNodePostfixes.end(); ++rolePostfixIt)
  {
    if (!includeMasterNode && rolePostfixIt==this->VirtualNodePostfixes.begin())
    {
      // the first one is the master sequence node, don't consider as a synchronized sequence node
      continue;
    }
    std::string sequenceNodeRef=SEQUENCE_NODE_REFERENCE_ROLE_BASE+(*rolePostfixIt);
    const char* foundNodeId=this->GetNodeReferenceID(sequenceNodeRef.c_str());
    if (foundNodeId==NULL)
    {
      continue;
    }
    if (strcmp(foundNodeId,nodeId)==0)
    {
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkMRMLSequenceBrowserNode::IsSynchronizedSequenceNode(vtkMRMLSequenceNode* sequenceNode, SynchronizationTypes syncType, bool includeMasterNode/*=false*/)
{
  if (sequenceNode == NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::IsSynchronizedSequenceNode failed: sequenceNode is invalid");
    return false;
  }
  return this->IsSynchronizedSequenceNodeID(sequenceNode->GetID(), syncType, includeMasterNode);
}

//----------------------------------------------------------------------------
bool vtkMRMLSequenceBrowserNode::IsSynchronizedSequenceNodeID(const char* nodeId, SynchronizationTypes syncType, bool includeMasterNode/*=false*/)
{
  if (nodeId == NULL)
  {
    vtkWarningMacro("vtkMRMLSequenceBrowserNode::IsSynchronizedSequenceNode nodeId is NULL");
    return false;
  }
  std::vector< vtkMRMLNode* > syncedNodes;
  std::stringstream syncedNodeRef;
  syncedNodeRef << SYNCHRONIZED_SEQUENCE_NODE_REFERENCE_ROLE_BASE << syncType;
  this->GetNodeReferences(syncedNodeRef.str().c_str(), syncedNodes);
  for (std::vector< vtkMRMLNode* >::iterator syncedNodeIt = syncedNodes.begin();
    syncedNodeIt != syncedNodes.end(); ++syncedNodeIt)
  {
    if (!includeMasterNode && (*syncedNodeIt)->GetID() == this->GetMasterSequenceNode()->GetID())
    {
      // if it is the master node, don't consider as a synchronized sequence node
      continue;
    }
    if (strcmp((*syncedNodeIt)->GetID(), nodeId) == 0)
    {
      return true;
    }
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::SequenceNodeSynchronizationTypeOn(const char* nodeId, SynchronizationTypes syncType)
{
  if (nodeId == NULL)
  {
    vtkWarningMacro("vtkMRMLSequenceBrowserNode::SetSequenceNodeSynchronizationType nodeId is NULL");
    return;
  }
  std::stringstream syncedNodeRef;
  syncedNodeRef << SYNCHRONIZED_SEQUENCE_NODE_REFERENCE_ROLE_BASE << syncType;
  if (!this->IsSynchronizedSequenceNodeID(nodeId, syncType, true))
  {
    this->AddAndObserveNodeReferenceID(syncedNodeRef.str().c_str(), nodeId);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::SequenceNodeSynchronizationTypeOff(const char* nodeId, SynchronizationTypes syncType)
{
  if (nodeId == NULL)
  {
    vtkWarningMacro("vtkMRMLSequenceBrowserNode::SetSequenceNodeSynchronizationType nodeId is NULL");
    return;
  }
  std::vector< vtkMRMLNode* > syncedNodes;
  std::stringstream syncedNodeRef;
  syncedNodeRef << SYNCHRONIZED_SEQUENCE_NODE_REFERENCE_ROLE_BASE << syncType;
  this->GetNodeReferences(syncedNodeRef.str().c_str(), syncedNodes);
  for (std::vector< vtkMRMLNode* >::iterator syncedNodeIt = syncedNodes.begin();
    syncedNodeIt != syncedNodes.end(); ++syncedNodeIt)
  {
    if (strcmp((*syncedNodeIt)->GetID(), nodeId) == 0)
    {
      this->RemoveNthNodeReferenceID(syncedNodeRef.str().c_str(), syncedNodeIt-syncedNodes.begin());
    }
  }
}

//----------------------------------------------------------------------------
std::string vtkMRMLSequenceBrowserNode::AddSynchronizedSequenceNode(vtkMRMLSequenceNode* sequenceNode)
{
  if (sequenceNode == NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::AddSynchronizedSequenceNode failed: sequenceNode is invalid");
    return "";
  }
  return this->AddSynchronizedSequenceNodeID(sequenceNode->GetID());
}

//----------------------------------------------------------------------------
std::string vtkMRMLSequenceBrowserNode::AddSynchronizedSequenceNode(const char* synchronizedSequenceNodeId)
{
  return this->AddSynchronizedSequenceNodeID(synchronizedSequenceNodeId);
}

//----------------------------------------------------------------------------
std::string vtkMRMLSequenceBrowserNode::AddSynchronizedSequenceNodeID(const char* synchronizedSequenceNodeId)
{
  bool oldModify = this->StartModify();
  std::string rolePostfix = GenerateVirtualNodePostfix();
  this->VirtualNodePostfixes.push_back(rolePostfix);
  std::string sequenceNodeReferenceRole = SEQUENCE_NODE_REFERENCE_ROLE_BASE + rolePostfix;
  this->SetAndObserveNodeReferenceID(sequenceNodeReferenceRole.c_str(), synchronizedSequenceNodeId);
  this->EndModify(oldModify);
  // Probably if the sequence node is being synced, we want to playback and record it
  this->SequenceNodeSynchronizationTypeOn(synchronizedSequenceNodeId, SynchronizationTypes::Playback);
  this->SequenceNodeSynchronizationTypeOn(synchronizedSequenceNodeId, SynchronizationTypes::Recording);
  return rolePostfix;
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::RemoveSynchronizedSequenceNode(const char* nodeId)
{
  if (this->Scene==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::RemoveSynchronizedSequenceNode failed: scene is invalid");
    return;
  }
  if (nodeId==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::RemoveSynchronizedSequenceNode failed: nodeId is invalid");
    return;
  }

  for (std::vector< std::string >::iterator rolePostfixIt=this->VirtualNodePostfixes.begin();
    rolePostfixIt!=this->VirtualNodePostfixes.end(); ++rolePostfixIt)
  {
    std::string sequenceNodeRef=SEQUENCE_NODE_REFERENCE_ROLE_BASE+(*rolePostfixIt);
    const char* foundNodeId=this->GetNodeReferenceID(sequenceNodeRef.c_str());
    if (foundNodeId==NULL)
    {
      continue;
    }
    if (strcmp(foundNodeId,nodeId)==0)
    {
      // the iterator will become invalid, so make a copy of its content
      std::string rolePostfix=(*rolePostfixIt);
      bool oldModify=this->StartModify();
      this->VirtualNodePostfixes.erase(rolePostfixIt);
      this->RemoveNodeReferenceIDs(sequenceNodeRef.c_str());
      this->RemoveVirtualOutputDataNode(rolePostfix);
      this->RemoveVirtualOutputDisplayNodes(rolePostfix);      
      this->EndModify(oldModify);
      return;
    }
  }
  vtkWarningMacro("vtkMRMLSequenceBrowserNode::RemoveSynchronizedSequenceNode did nothing, the specified node was not synchronized");
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::GetSynchronizedSequenceNodes(std::vector< vtkMRMLSequenceNode* > &synchronizedDataNodes, bool includeMasterNode/*=false*/)
{
  synchronizedDataNodes.clear();

  for (std::vector< std::string >::iterator rolePostfixIt=this->VirtualNodePostfixes.begin();
    rolePostfixIt!=this->VirtualNodePostfixes.end(); ++rolePostfixIt)
  {
    if (!includeMasterNode && rolePostfixIt==this->VirtualNodePostfixes.begin())
    {
      // the first one is the master sequence node, don't consider as a synchronized sequence node
      continue;
    }
    std::string sequenceNodeRef=SEQUENCE_NODE_REFERENCE_ROLE_BASE+(*rolePostfixIt);
    vtkMRMLSequenceNode* synchronizedNode=vtkMRMLSequenceNode::SafeDownCast(this->GetNodeReference(sequenceNodeRef.c_str()));
    if (synchronizedNode==NULL)
    {
      // valid case during scene updates
      continue;
    }
    synchronizedDataNodes.push_back(synchronizedNode);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::GetSynchronizedSequenceNodes(vtkCollection* synchronizedDataNodes, bool includeMasterNode /* =false */)
{
  if (synchronizedDataNodes==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::GetSynchronizedSequenceNodes failed: synchronizedDataNodes is invalid");
    return;
  }
  std::vector< vtkMRMLSequenceNode* > synchronizedDataNodesVector;
  this->GetSynchronizedSequenceNodes(synchronizedDataNodesVector, includeMasterNode);
  synchronizedDataNodes->RemoveAllItems();
  for (std::vector< vtkMRMLSequenceNode* >::iterator it = synchronizedDataNodesVector.begin(); it != synchronizedDataNodesVector.end(); ++it)
  {
    synchronizedDataNodes->AddItem(*it);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::GetSynchronizedSequenceNodes(std::vector< vtkMRMLSequenceNode* > &synchronizedDataNodes, SynchronizationTypes syncType, bool includeMasterNode/*=false*/)
{
  synchronizedDataNodes.clear();

  std::vector< vtkMRMLNode* > syncedNodes;
  std::stringstream syncedNodeRef;
  syncedNodeRef << SYNCHRONIZED_SEQUENCE_NODE_REFERENCE_ROLE_BASE << syncType;
  this->GetNodeReferences(syncedNodeRef.str().c_str(), syncedNodes);
  for (std::vector< vtkMRMLNode* >::iterator syncedNodeIt = syncedNodes.begin();
    syncedNodeIt != syncedNodes.end(); ++syncedNodeIt)
  {
    if (!includeMasterNode && (*syncedNodeIt)->GetID() == this->GetMasterSequenceNode()->GetID())
    {
      // if it is the master node, don't consider as a synchronized sequence node
      continue;
    }
    vtkMRMLSequenceNode* synchronizedNode = vtkMRMLSequenceNode::SafeDownCast(*syncedNodeIt);
    if (synchronizedNode == NULL)
    {
      // valid case during scene updates
      continue;
    }
    synchronizedDataNodes.push_back(synchronizedNode);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::GetSynchronizedSequenceNodes(vtkCollection* synchronizedDataNodes, SynchronizationTypes syncType, bool includeMasterNode /* =false */)
{
  if (synchronizedDataNodes == NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::GetSynchronizedSequenceNodes failed: synchronizedDataNodes is invalid");
    return;
  }
  std::vector< vtkMRMLSequenceNode* > synchronizedDataNodesVector;
  this->GetSynchronizedSequenceNodes(synchronizedDataNodesVector, syncType, includeMasterNode);
  synchronizedDataNodes->RemoveAllItems();
  for (std::vector< vtkMRMLSequenceNode* >::iterator it = synchronizedDataNodesVector.begin(); it != synchronizedDataNodesVector.end(); ++it)
  {
    synchronizedDataNodes->AddItem(*it);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::ScalarVolumeAutoWindowLevelOff()
{
  vtkMRMLSequenceNode* sequenceNode = this->GetMasterSequenceNode();
  if (sequenceNode==NULL)
  {
    vtkWarningMacro("vtkMRMLSequenceBrowserNode::ScalarVolumeAutoWindowLevelOff failed: sequence node is invalid");
    return;
  }
  std::vector< vtkMRMLDisplayNode* > displayNodes;
  this->GetVirtualOutputDisplayNodes(sequenceNode, displayNodes);
  for (std::vector< vtkMRMLDisplayNode* >::iterator displayNodePtrIt = displayNodes.begin(); displayNodePtrIt != displayNodes.end(); ++displayNodePtrIt)
  {
    vtkMRMLScalarVolumeDisplayNode *scalarVolumeDisplayNode = vtkMRMLScalarVolumeDisplayNode::SafeDownCast(*displayNodePtrIt);
    if (scalarVolumeDisplayNode==NULL)
    {
      continue;
    }
    scalarVolumeDisplayNode->AutoWindowLevelOff();
  }
}

//---------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::SetRecordingActive(bool recording)
{
  // Before activating the recording, set the initial timestamp to be correct
  this->InitialTime = vtkTimerLog::GetUniversalTime();
  if (this->GetMasterSequenceNode()!=NULL 
    && this->GetMasterSequenceNode()->GetNumberOfDataNodes()>0
    && this->GetMasterSequenceNode()->GetIndexType()==vtkMRMLSequenceNode::NumericIndex)
  {
    std::stringstream timeString;
    timeString << this->GetMasterSequenceNode()->GetNthIndexValue( this->GetMasterSequenceNode()->GetNumberOfDataNodes() - 1 );
    double timeValue; timeString >> timeValue;
    this->InitialTime = this->InitialTime - timeValue;
  }
  if (this->RecordingActive!=recording)
  {
    this->RecordingActive = recording;
    this->Modified();
  }
}

//---------------------------------------------------------------------------
int vtkMRMLSequenceBrowserNode::SelectFirstItem()
{
  vtkMRMLSequenceNode* sequenceNode = this->GetMasterSequenceNode();
  int selectedItemNumber = -1;
  if (sequenceNode && sequenceNode->GetNumberOfDataNodes()>0)
  {
    selectedItemNumber = 0;
  }
  this->SetSelectedItemNumber(selectedItemNumber );
  return selectedItemNumber;
}

//---------------------------------------------------------------------------
int vtkMRMLSequenceBrowserNode::SelectLastItem()
{
  vtkMRMLSequenceNode* sequenceNode = this->GetMasterSequenceNode();
  int selectedItemNumber = -1;
  if (sequenceNode && sequenceNode->GetNumberOfDataNodes()>0)
  {
    selectedItemNumber = sequenceNode->GetNumberOfDataNodes()-1;
  }
  this->SetSelectedItemNumber(selectedItemNumber );
  return selectedItemNumber;
}

//---------------------------------------------------------------------------
int vtkMRMLSequenceBrowserNode::SelectNextItem(int selectionIncrement/*=1*/)
{
  vtkMRMLSequenceNode* sequenceNode=this->GetMasterSequenceNode();
  if (sequenceNode==NULL || sequenceNode->GetNumberOfDataNodes()==0)
  {
    // nothing to replay
    return -1;
  }
  int selectedItemNumber=this->GetSelectedItemNumber();
  int browserNodeModify=this->StartModify(); // invoke modification event once all the modifications has been completed
  if (selectedItemNumber<0)
  {
    selectedItemNumber=0;
  }
  else
  {
    selectedItemNumber += selectionIncrement;
    if (selectedItemNumber>=sequenceNode->GetNumberOfDataNodes())
    {
      if (this->GetPlaybackLooped())
      {
        // wrap around and keep playback going
        selectedItemNumber = selectedItemNumber % sequenceNode->GetNumberOfDataNodes();
      }
      else
      {
        this->SetPlaybackActive(false);
        selectedItemNumber=0;
      }
    }
    else if (selectedItemNumber<0)
    {
      if (this->GetPlaybackLooped())
      {
        // wrap around and keep playback going
        selectedItemNumber = (selectedItemNumber % sequenceNode->GetNumberOfDataNodes()) + sequenceNode->GetNumberOfDataNodes();
      }
      else
      {
        this->SetPlaybackActive(false);
        selectedItemNumber=sequenceNode->GetNumberOfDataNodes()-1;
      }
    }
  }
  this->SetSelectedItemNumber(selectedItemNumber);
  this->EndModify(browserNodeModify);
  return selectedItemNumber;
}

//---------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::ProcessMRMLEvents( vtkObject *caller, unsigned long event, void *callData )
{
  this->vtkMRMLNode::ProcessMRMLEvents( caller, event, callData );
  
  // Do nothing if the node is not recording
  if (!this->GetRecordingActive())
  {
    return;
  }
  // Make sure it was a virtual output data node
  vtkMRMLNode* modifiedNode = vtkMRMLNode::SafeDownCast(caller);
  if (modifiedNode==NULL || !this->IsVirtualOutputDataNode(modifiedNode->GetID()))
  {
    return;
  }

  // If we only record when the master node is modified, then skip if it was not the master node that was modified
  const char* masterVirtualOutputNodeID = this->GetVirtualOutputDataNode(this->GetMasterSequenceNode())->GetID();
  if (this->GetRecordMasterOnly() && strcmp(modifiedNode->GetID(), masterVirtualOutputNodeID)!=0)
  {
    return;
  }

  // To maintain syncing, we need to record for all sequences at every given timestamp
  std::stringstream currTime;
  currTime << ( vtkTimerLog::GetUniversalTime() - this->InitialTime );

  // Record into each sequence
  std::vector< vtkMRMLSequenceNode* > sequenceNodes;
  this->GetSynchronizedSequenceNodes( sequenceNodes, SynchronizationTypes::Recording, true );
  vtkMRMLSequenceNode* sequenceNode = NULL;
  for (std::vector< vtkMRMLSequenceNode* >::iterator it = sequenceNodes.begin(); it != sequenceNodes.end(); it++ )
  {
    vtkMRMLSequenceNode* currSequenceNode = (*it);
    currSequenceNode->SetDataNodeAtValue(this->GetVirtualOutputDataNode(currSequenceNode), currTime.str().c_str());
  }
}

//---------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::FixSequenceNodeReferenceRoleName()
{
  bool oldModify=this->StartModify();
  for (std::vector< std::string >::iterator rolePostfixIt=this->VirtualNodePostfixes.begin();
    rolePostfixIt!=this->VirtualNodePostfixes.end(); ++rolePostfixIt)
  {
    std::string obsoleteSequenceNodeReferenceRole=std::string("rootNodeRef")+(*rolePostfixIt);
    std::string sequenceNodeReferenceRole=SEQUENCE_NODE_REFERENCE_ROLE_BASE+(*rolePostfixIt);
    const char* obsoleteSeqNodeId = this->GetNodeReferenceID(obsoleteSequenceNodeReferenceRole.c_str());
    const char* seqNodeId = this->GetNodeReferenceID(sequenceNodeReferenceRole.c_str());
    if (seqNodeId==NULL && obsoleteSeqNodeId!=NULL)
    {
      // we've found an obsolete reference, move it into the new reference
      this->SetNodeReferenceID(sequenceNodeReferenceRole.c_str(), obsoleteSeqNodeId);
      this->SetNodeReferenceID(obsoleteSequenceNodeReferenceRole.c_str(), NULL);
    }
  }
  this->EndModify(oldModify);
}

//---------------------------------------------------------------------------
vtkMRMLSequenceNode* vtkMRMLSequenceBrowserNode::GetSequenceNode(vtkMRMLNode* virtualOutputDataNode)
{
  if (virtualOutputDataNode == NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::GetSequenceNode failed: virtualOutputDataNode is invalid");
    return NULL;
  }
  for (std::vector< std::string >::iterator rolePostfixIt = this->VirtualNodePostfixes.begin();
    rolePostfixIt != this->VirtualNodePostfixes.end(); ++rolePostfixIt)
  {
    std::string dataNodeRef = DATA_NODE_REFERENCE_ROLE_BASE + (*rolePostfixIt);
    vtkMRMLNode* foundDataNode = this->GetNodeReference(dataNodeRef.c_str());
    if (foundDataNode == virtualOutputDataNode)
    {
      std::string sequenceNodeReferenceRole = SEQUENCE_NODE_REFERENCE_ROLE_BASE + (*rolePostfixIt);
      vtkMRMLSequenceNode* sequenceNode = vtkMRMLSequenceNode::SafeDownCast(this->GetNodeReference(sequenceNodeReferenceRole.c_str()));
      return sequenceNode;
    }
  }
  return NULL;
}
