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
#include "vtkMRMLNodeSequencer.h"

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
#include <vtkCollectionIterator.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>
#include <vtkTimerLog.h>

// STD includes
#include <sstream>
#include <algorithm> // for std::find

// First reference is the master sequence node, subsequent references are the synchronized sequence nodes
static const char* SEQUENCE_NODE_REFERENCE_ROLE_BASE = "sequenceNodeRef"; // Old: rootNodeRef
static const char* PROXY_NODE_REFERENCE_ROLE_BASE = "dataNodeRef"; // TODO: Change this to "proxyNodeRef", but need to maintain backwards-compatibility with "dataNodeRef"

static const char* PROXY_NODE_COPY_ATTRIBUTE_NAME = "proxyNodeCopy";



// Declare the Synchronization Properties struct
struct vtkMRMLSequenceBrowserNode::SynchronizationProperties
{
  SynchronizationProperties():
    Playback(true),
    Recording(false), // to only show recording controls if it's explicitly asked by the user
    OverwriteProxyName(true),
    SaveChanges(true) // for performance reasons
  {
  }
  
  void FromString( std::string str );
  std::string ToString();

  bool Playback;
  bool Recording;
  bool OverwriteProxyName; // change proxy node name during replay (includes index value)
  bool SaveChanges; // save proxy node changes into the sequence
};

void vtkMRMLSequenceBrowserNode::SynchronizationProperties::FromString( std::string str )
{
  std::stringstream ss(str);
  while (!ss.eof())
  {
    std::string attName, attValue;
    ss >> attName;
    ss >> attValue;
    if (!attName.empty() && !attValue.empty())
    {
      std::string subAttValue;
      if (!attName.compare("playback"))
      {
        this->Playback=(!attValue.compare("true"));
      }
      if (!attName.compare("recording"))
      {
        this->Recording=(!attValue.compare("true"));
      }
      if (!attName.compare("overwriteProxyName"))
      {
        this->OverwriteProxyName=(!attValue.compare("true"));
      }
      if (!attName.compare("saveChanges"))
      {
        this->SaveChanges=(!attValue.compare("true"));
      }
    }
  }
}

std::string vtkMRMLSequenceBrowserNode::SynchronizationProperties::ToString()
{
  std::stringstream ss;
  ss << "playback" << " " << (this->Playback ? "true" : "false") << " ";
  ss << "recording" << " " << (this->Recording ? "true" : "false") << " ";
  ss << "overwriteProxyName" << " " << (this->OverwriteProxyName ? "true" : "false") << " ";
  ss << "saveChanges" << " " << (this->SaveChanges ? "true" : "false") << " ";
  return ss.str();
}




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

  of << indent << " virtualNodePostfixes=\""; // TODO: Change to "synchronizationPostfixes", but need backwards-compatibility with "virtualNodePostfixes"
  for(std::vector< std::string >::iterator roleNameIt=this->SynchronizationPostfixes.begin();
    roleNameIt!=this->SynchronizationPostfixes.end(); ++roleNameIt)
  { 
    if (roleNameIt!=this->SynchronizationPostfixes.begin())
    {
      // print separator before printing the (if not the first element)
      of << " ";
    }
    of << roleNameIt->c_str();
  }
  of << "\"";

  for(std::map< std::string, SynchronizationProperties* >::iterator rolePostfixIt=this->SynchronizationPropertiesMap.begin();
    rolePostfixIt!=this->SynchronizationPropertiesMap.end(); ++rolePostfixIt)
  {
    of << indent << " SynchronizationPropertiesMap" << rolePostfixIt->first.c_str() << "=\"" << rolePostfixIt->second->ToString() << "\"";
  }

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
    else if (!strcmp(attName, "virtualNodePostfixes")) // TODO: Change to "synchronizationPostfixes", but need backwards-compatibility with "virtualNodePostfixes"
    {
      this->SynchronizationPostfixes.clear();
      std::stringstream ss(attValue);
      while (!ss.eof())
      {
        std::string rolePostfix;
        ss >> rolePostfix;
        if (!rolePostfix.empty())
        {
          this->SynchronizationPostfixes.push_back(rolePostfix);
          if (this->SynchronizationPropertiesMap.find(rolePostfix) == this->SynchronizationPropertiesMap.end())
          {
            this->SynchronizationPropertiesMap[rolePostfix] = new SynchronizationProperties(); // Populate with default. If for any reason (e.g. old scene) the associated synchronization properties are not found, this is better than having NULL.
          }
        }
      }
    }
    else if (std::string(attName).find("SynchronizationPropertiesMap")!=std::string::npos)
    {
      SynchronizationProperties* currSyncProps = new SynchronizationProperties();
      currSyncProps->FromString(attValue);
      std::string rolePostfix = std::string(attName).substr(std::string(attName).find("SynchronizationPropertiesMap")+std::string("SynchronizationPropertiesMap").length());
      this->SynchronizationPropertiesMap[rolePostfix] = currSyncProps;  // Possibly overwriting the default, but that is ok
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
  // TODO: The referenced node copying is handled in the superclass. Do we need to copy the other attributes here?
  this->SynchronizationPostfixes=node->SynchronizationPostfixes;
  this->SynchronizationPropertiesMap=node->SynchronizationPropertiesMap;
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
std::string vtkMRMLSequenceBrowserNode::GenerateSynchronizationPostfix()
{
  while (1)
  {
    std::stringstream postfix;
    postfix << this->LastPostfixIndex;
    this->LastPostfixIndex++;
    bool isUnique=(std::find(this->SynchronizationPostfixes.begin(), this->SynchronizationPostfixes.end(), postfix.str())==this->SynchronizationPostfixes.end());
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
  if (!this->SynchronizationPostfixes.empty())
  {
    this->RemoveAllSequenceNodes();
  }
  if (sequenceNodeID!=NULL)
  {
    this->AddSynchronizedSequenceNodeID(sequenceNodeID); // The same "adding" mechanism is used, but the master must be the zeroth elements of the postfixes vector
  }
  this->EndModify(oldModify);
}

//----------------------------------------------------------------------------
vtkMRMLSequenceNode* vtkMRMLSequenceBrowserNode::GetMasterSequenceNode()
{
  if (this->SynchronizationPostfixes.empty())
  {
    return NULL;
  }  
  std::string sequenceNodeReferenceRole=SEQUENCE_NODE_REFERENCE_ROLE_BASE+this->SynchronizationPostfixes[0];  
  vtkMRMLSequenceNode* node=vtkMRMLSequenceNode::SafeDownCast(this->GetNodeReference(sequenceNodeReferenceRole.c_str()));
  return node;
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::RemoveAllProxyNodes()
{
  bool oldModify=this->StartModify();
  for (std::vector< std::string >::iterator rolePostfixIt=this->SynchronizationPostfixes.begin();
    rolePostfixIt!=this->SynchronizationPostfixes.end(); ++rolePostfixIt)
  {
    this->RemoveProxyNode(*rolePostfixIt);
  }
  this->EndModify(oldModify);
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::RemoveAllSequenceNodes()
{
  bool oldModify=this->StartModify();
  // need to make a copy as this->VirtualNodePostfixes changes as we remove nodes
  std::vector< std::string > synchronizationPostfixes=this->SynchronizationPostfixes;
  // start from the end to delete the master sequence node last
  for (std::vector< std::string >::reverse_iterator rolePostfixIt=synchronizationPostfixes.rbegin();
    rolePostfixIt!=synchronizationPostfixes.rend(); ++rolePostfixIt)
  {
    std::string sequenceNodeReferenceRole=SEQUENCE_NODE_REFERENCE_ROLE_BASE+(*rolePostfixIt);
    vtkMRMLSequenceNode* node=vtkMRMLSequenceNode::SafeDownCast(this->GetNodeReference(sequenceNodeReferenceRole.c_str()));
    if (node==NULL)
    {
      vtkErrorMacro("Invalid sequence node");
      std::vector< std::string >::iterator rolePostfixInOriginalIt=std::find(this->SynchronizationPostfixes.begin(), this->SynchronizationPostfixes.end(), (*rolePostfixIt));
      if (rolePostfixInOriginalIt!=this->SynchronizationPostfixes.end())
      {
        this->SynchronizationPostfixes.erase(rolePostfixInOriginalIt);
      }
      continue;
    }
    this->RemoveSynchronizedSequenceNode(node->GetID());
  }
  this->EndModify(oldModify);
}


//----------------------------------------------------------------------------
std::string vtkMRMLSequenceBrowserNode::GetSynchronizationPostfixFromSequence(vtkMRMLSequenceNode* sequenceNode)
{
  if (sequenceNode==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::GetSynchronizationPostfixFromSequence failed: sequenceNode is invalid");
    return "";
  }
  for (std::vector< std::string >::iterator rolePostfixIt=this->SynchronizationPostfixes.begin();
    rolePostfixIt!=this->SynchronizationPostfixes.end(); ++rolePostfixIt)
  {
    std::string sequenceNodeRef=SEQUENCE_NODE_REFERENCE_ROLE_BASE+(*rolePostfixIt);
    if (this->GetNodeReference(sequenceNodeRef.c_str())==sequenceNode)
    {
      return (*rolePostfixIt);
    }
  }
  return "";
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLSequenceBrowserNode::GetProxyNode(vtkMRMLSequenceNode* sequenceNode)
{
  if (sequenceNode==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::GetVirtualOutputNode failed: sequenceNode is invalid");
    return NULL;
  }
  std::string rolePostfix=this->GetSynchronizationPostfixFromSequence(sequenceNode);
  if (rolePostfix.empty())
  {
    return NULL;
  }
  std::string proxyNodeRef=PROXY_NODE_REFERENCE_ROLE_BASE+rolePostfix;
  return this->GetNodeReference(proxyNodeRef.c_str());
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLSequenceBrowserNode::AddProxyNode(vtkMRMLNode* sourceProxyNode, vtkMRMLSequenceNode* sequenceNode, bool copy)
{
  if (sequenceNode==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::AddProxyNode failed: sequenceNode is invalid");
    return NULL;
  }
  if (this->Scene==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::AddProxyNode failed: scene is invalid");
    return NULL;
  }

  bool oldModify=this->StartModify();
  
  std::string rolePostfix=this->GetSynchronizationPostfixFromSequence(sequenceNode);
  if (rolePostfix.empty())
  {
    // Add reference to the sequence node
    rolePostfix=AddSynchronizedSequenceNodeID(sequenceNode->GetID());
  }

  // Add copy of the data node
  std::string proxyNodeRef=PROXY_NODE_REFERENCE_ROLE_BASE+rolePostfix;
  // Create a new one from scratch in the new scene to make sure only the needed parts are copied
  vtkMRMLNode* proxyNode = sourceProxyNode;
  if ( copy )
  {
    proxyNode = sourceProxyNode->CreateNodeInstance();
    this->Scene->AddNode(proxyNode);
    proxyNode->SetAttribute(PROXY_NODE_COPY_ATTRIBUTE_NAME,"true"); // Indicate that this is a copy
    proxyNode->Delete(); // ownership transferred to the scene, so we can release the pointer
  }

  this->RemoveProxyNode(rolePostfix); // This will also remove the proxy node from the scene if necessary
  this->SetAndObserveNodeReferenceID(proxyNodeRef.c_str(), proxyNode->GetID(), vtkMRMLNodeSequencer::GetInstance()->GetNodeSequencer(proxyNode)->GetRecordingEvents());

  this->EndModify(oldModify);
  return proxyNode;
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::GetAllProxyNodes(std::vector< vtkMRMLNode* >& nodes)
{
  nodes.clear();
  for (std::vector< std::string >::iterator rolePostfixIt=this->SynchronizationPostfixes.begin();
    rolePostfixIt!=this->SynchronizationPostfixes.end(); ++rolePostfixIt)
  {
    std::string proxyNodeRef=PROXY_NODE_REFERENCE_ROLE_BASE+(*rolePostfixIt);
    vtkMRMLNode* node = this->GetNodeReference(proxyNodeRef.c_str());
    if (node==NULL)
    {
      continue;
    }
    nodes.push_back(node);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::GetAllProxyNodes(vtkCollection* nodes)
{
  if (nodes==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::GetAllProxyNodes failed: nodes is invalid");
    return;
  }
  std::vector< vtkMRMLNode* > nodesVector;
  this->GetAllProxyNodes(nodesVector);
  nodes->RemoveAllItems();
  for (std::vector< vtkMRMLNode* >::iterator it = nodesVector.begin(); it != nodesVector.end(); ++it)
  {
    nodes->AddItem(*it);
  }
}

//----------------------------------------------------------------------------
bool vtkMRMLSequenceBrowserNode::IsProxyNode(const char* nodeId)
{
  return this->IsProxyNodeID(nodeId);
}

//----------------------------------------------------------------------------
bool vtkMRMLSequenceBrowserNode::IsProxyNodeID(const char* nodeId)
{
  std::vector< vtkMRMLNode* > nodesVector;
  this->GetAllProxyNodes(nodesVector);
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
void vtkMRMLSequenceBrowserNode::RemoveProxyNode(const std::string& postfix)
{
  bool oldModify=this->StartModify();
  std::string proxyNodeRef=PROXY_NODE_REFERENCE_ROLE_BASE+postfix;
  vtkMRMLNode* proxyNode=this->GetNodeReference(proxyNodeRef.c_str());
  if (proxyNode!=NULL)
  {
    if (proxyNode->GetAttribute(PROXY_NODE_COPY_ATTRIBUTE_NAME)!=NULL)
    {
      this->Scene->RemoveNode(proxyNode);
    }
    this->RemoveNodeReferenceIDs(proxyNodeRef.c_str());
  }
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
  for (std::vector< std::string >::iterator rolePostfixIt=this->SynchronizationPostfixes.begin();
    rolePostfixIt!=this->SynchronizationPostfixes.end(); ++rolePostfixIt)
  {
    if (!includeMasterNode && rolePostfixIt==this->SynchronizationPostfixes.begin())
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
  std::string rolePostfix = this->GenerateSynchronizationPostfix();
  this->SynchronizationPostfixes.push_back(rolePostfix);
  std::string sequenceNodeReferenceRole = SEQUENCE_NODE_REFERENCE_ROLE_BASE + rolePostfix;
  this->SetAndObserveNodeReferenceID(sequenceNodeReferenceRole.c_str(), synchronizedSequenceNodeId);
  this->SynchronizationPropertiesMap[ rolePostfix ] = new SynchronizationProperties();
  this->EndModify(oldModify);
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

  for (std::vector< std::string >::iterator rolePostfixIt=this->SynchronizationPostfixes.begin();
    rolePostfixIt!=this->SynchronizationPostfixes.end(); ++rolePostfixIt)
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
      this->SynchronizationPostfixes.erase(rolePostfixIt);
      this->RemoveNodeReferenceIDs(sequenceNodeRef.c_str());
      this->RemoveProxyNode(rolePostfix);
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

  for (std::vector< std::string >::iterator rolePostfixIt=this->SynchronizationPostfixes.begin();
    rolePostfixIt!=this->SynchronizationPostfixes.end(); ++rolePostfixIt)
  {
    if (!includeMasterNode && rolePostfixIt==this->SynchronizationPostfixes.begin())
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

  vtkMRMLNode* modifiedNode = vtkMRMLNode::SafeDownCast(caller);
  if (modifiedNode == NULL || !this->IsProxyNode(modifiedNode->GetID()))
  {
    // we are only interested in proxy node modified events
    return;
  }

  // Record proxy node state into new sequence item
  if (this->GetRecordingActive())
  {
    // If we only record when the master node is modified, then skip if it was not the master node that was modified
    const char* masterVirtualOutputNodeID = this->GetProxyNode(this->GetMasterSequenceNode())->GetID();
    if (!this->GetRecordMasterOnly() || strcmp(modifiedNode->GetID(), masterVirtualOutputNodeID) == 0)
    {
      this->SaveProxyNodesState();
    }
    return;
  }

  // Update sequence item from proxy node
  vtkMRMLSequenceNode* sequenceNode = this->GetSequenceNode(modifiedNode);
  if (!this->GetPlaybackActive() && sequenceNode && this->GetSaveChanges(sequenceNode))
  {    
    sequenceNode->UpdateDataNodeAtValue(modifiedNode, sequenceNode->GetNthIndexValue(this->GetSelectedItemNumber()).c_str(), true /* shallow copy*/);
  }
}

//---------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::SaveProxyNodesState()
{
  std::stringstream currTime;
  bool continuousRecording = this->GetRecordingActive();
  if (continuousRecording)
  {
    // Continuous recording.
    // To maintain syncing, we need to record for all sequences at every given timestamp.
    currTime << (vtkTimerLog::GetUniversalTime() - this->InitialTime);
  }
  else
  {
    // Recording a single snapshot
    // TODO: add support for non-numeric index type
    double lastItemTime = 0;
    if (this->GetMasterSequenceNode()->GetNumberOfDataNodes() > 0)
    {
      std::stringstream timeString;
      timeString << this->GetMasterSequenceNode()->GetNthIndexValue(this->GetMasterSequenceNode()->GetNumberOfDataNodes() - 1);
      timeString >> lastItemTime;
    }
    double playbackRateFps = this->GetPlaybackRateFps() != 0.0 ? this->GetPlaybackRateFps() : 1.0;
    currTime << lastItemTime + 1.0 / playbackRateFps;
  }

  // Record into each sequence
  int wasModified = this->StartModify();
  std::vector< vtkMRMLSequenceNode* > sequenceNodes;
  this->GetSynchronizedSequenceNodes(sequenceNodes, true);
  vtkMRMLSequenceNode* sequenceNode = NULL;
  bool snapshotAdded = false;
  for (std::vector< vtkMRMLSequenceNode* >::iterator it = sequenceNodes.begin(); it != sequenceNodes.end(); it++)
  {
    vtkMRMLSequenceNode* currSequenceNode = (*it);
    if (this->GetRecording(currSequenceNode))
    {
      currSequenceNode->SetDataNodeAtValue(this->GetProxyNode(currSequenceNode), currTime.str().c_str());
      snapshotAdded = true;
    }
  }
  if (snapshotAdded)
  {
    this->Modified();
    this->SelectLastItem();
  }
  this->EndModify(wasModified);
}

//---------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::OnNodeReferenceAdded(vtkMRMLNodeReference* nodeReference)
{
  vtkMRMLNode::OnNodeReferenceAdded(nodeReference);
  if (std::string(nodeReference->GetReferenceRole()).find( PROXY_NODE_REFERENCE_ROLE_BASE ) != std::string::npos)
  {
    // Need to observe the correct events after scene loading
    this->SetAndObserveNodeReferenceID( nodeReference->GetReferenceRole(), nodeReference->GetReferencedNodeID(), 
      vtkMRMLNodeSequencer::GetInstance()->GetNodeSequencer(nodeReference->GetReferencedNode())->GetRecordingEvents());
    // TODO: check if nodeReference->GetReferencedNode() is already valid here
  }
}

//---------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::FixSequenceNodeReferenceRoleName()
{
  bool oldModify=this->StartModify();
  for (std::vector< std::string >::iterator rolePostfixIt=this->SynchronizationPostfixes.begin();
    rolePostfixIt!=this->SynchronizationPostfixes.end(); ++rolePostfixIt)
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
vtkMRMLSequenceNode* vtkMRMLSequenceBrowserNode::GetSequenceNode(vtkMRMLNode* proxyNode)
{
  if (proxyNode == NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::GetSequenceNode failed: virtualOutputDataNode is invalid");
    return NULL;
  }
  for (std::vector< std::string >::iterator rolePostfixIt = this->SynchronizationPostfixes.begin();
    rolePostfixIt != this->SynchronizationPostfixes.end(); ++rolePostfixIt)
  {
    std::string proxyNodeRef = PROXY_NODE_REFERENCE_ROLE_BASE + (*rolePostfixIt);
    vtkMRMLNode* foundProxyNode = this->GetNodeReference(proxyNodeRef.c_str());
    if (foundProxyNode == proxyNode)
    {
      std::string sequenceNodeReferenceRole = SEQUENCE_NODE_REFERENCE_ROLE_BASE + (*rolePostfixIt);
      vtkMRMLSequenceNode* sequenceNode = vtkMRMLSequenceNode::SafeDownCast(this->GetNodeReference(sequenceNodeReferenceRole.c_str()));
      return sequenceNode;
    }
  }
  return NULL;
}

//---------------------------------------------------------------------------
bool vtkMRMLSequenceBrowserNode::IsAnySequenceNodeRecording()
{
  for (std::vector< std::string >::iterator rolePostfixIt = this->SynchronizationPostfixes.begin();
    rolePostfixIt != this->SynchronizationPostfixes.end(); ++rolePostfixIt)
  {
    SynchronizationProperties* syncProps = this->SynchronizationPropertiesMap[(*rolePostfixIt)];
    if (syncProps == NULL)
    {
      continue;
    }
    if (syncProps->Recording)
    {
      return true;
    }
  }
  return false;
}

//---------------------------------------------------------------------------
bool vtkMRMLSequenceBrowserNode::GetRecording(vtkMRMLSequenceNode* sequenceNode)
{
  std::string rolePostfix = this->GetSynchronizationPostfixFromSequence(sequenceNode);
  SynchronizationProperties* syncProps = this->SynchronizationPropertiesMap[ rolePostfix ];
  if (rolePostfix=="" || syncProps==NULL)
  {
    return false;
  }
  return syncProps->Recording;
}

//---------------------------------------------------------------------------
bool vtkMRMLSequenceBrowserNode::GetPlayback(vtkMRMLSequenceNode* sequenceNode)
{
  std::string rolePostfix = this->GetSynchronizationPostfixFromSequence(sequenceNode);
  SynchronizationProperties* syncProps = this->SynchronizationPropertiesMap[ rolePostfix ];
  if (rolePostfix=="" || syncProps==NULL)
  {
    return false;
  }
  return syncProps->Playback;
}

//---------------------------------------------------------------------------
bool vtkMRMLSequenceBrowserNode::GetOverwriteProxyName(vtkMRMLSequenceNode* sequenceNode)
{
  std::string rolePostfix = this->GetSynchronizationPostfixFromSequence(sequenceNode);
  SynchronizationProperties* syncProps = this->SynchronizationPropertiesMap[ rolePostfix ];
  if (rolePostfix=="" || syncProps==NULL)
  {
    return false;
  }
  return syncProps->OverwriteProxyName;
}

//---------------------------------------------------------------------------
bool vtkMRMLSequenceBrowserNode::GetSaveChanges(vtkMRMLSequenceNode* sequenceNode)
{
  std::string rolePostfix = this->GetSynchronizationPostfixFromSequence(sequenceNode);
  SynchronizationProperties* syncProps = this->SynchronizationPropertiesMap[ rolePostfix ];
  if (rolePostfix=="" || syncProps==NULL)
  {
    return false;
  }
  return syncProps->SaveChanges;
}

//---------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::SetRecording(vtkMRMLSequenceNode* sequenceNode, bool recording)
{
  std::string rolePostfix = this->GetSynchronizationPostfixFromSequence(sequenceNode);
  SynchronizationProperties* syncProps = this->SynchronizationPropertiesMap[ rolePostfix ];
  if (rolePostfix=="" || syncProps==NULL)
  {
    return;
  }
  if (syncProps->Recording != recording)
  {
    syncProps->Recording = recording;
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::SetPlayback(vtkMRMLSequenceNode* sequenceNode, bool playback)
{
  std::string rolePostfix = this->GetSynchronizationPostfixFromSequence(sequenceNode);
  SynchronizationProperties* syncProps = this->SynchronizationPropertiesMap[ rolePostfix ];
  if (rolePostfix=="" || syncProps==NULL)
  {
    return;
  }
  if (syncProps->Playback != playback)
  {
    syncProps->Playback = playback;
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::SetOverwriteProxyName(vtkMRMLSequenceNode* sequenceNode, bool overwrite)
{
  std::string rolePostfix = this->GetSynchronizationPostfixFromSequence(sequenceNode);
  SynchronizationProperties* syncProps = this->SynchronizationPropertiesMap[ rolePostfix ];
  if (rolePostfix=="" || syncProps==NULL)
  {
    return;
  }
  if (syncProps->OverwriteProxyName != overwrite)
  {
    syncProps->OverwriteProxyName = overwrite;
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::SetSaveChanges(vtkMRMLSequenceNode* sequenceNode, bool save)
{
  std::string rolePostfix = this->GetSynchronizationPostfixFromSequence(sequenceNode);
  SynchronizationProperties* syncProps = this->SynchronizationPropertiesMap[ rolePostfix ];
  if (rolePostfix=="" || syncProps==NULL)
  {
    return;
  }
  if (syncProps->SaveChanges != save)
  {
    syncProps->SaveChanges = save;
    this->Modified();
  }
}