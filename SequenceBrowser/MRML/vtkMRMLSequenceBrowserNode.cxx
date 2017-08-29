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
#include <vtkMRMLHierarchyNode.h>

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
    OverwriteProxyName(false), // make sure proxy node names are not accidentally overwritten
    SaveChanges(false) // to prevent accidental sequence node changes by default
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
: SelectedItemNumber(-1)
, LastPostfixIndex(0)
, PlaybackActive(false)
, PlaybackRateFps(10.0)
, PlaybackItemSkippingEnabled(true)
, PlaybackLooped(true)
, RecordMasterOnly(false)
, RecordingSamplingMode(vtkMRMLSequenceBrowserNode::SamplingLimitedToPlaybackFrameRate)
, IndexDisplayMode(vtkMRMLSequenceBrowserNode::IndexDisplayAsIndexValue)
, RecordingActive(false)
{
  this->SetHideFromEditors(false);
  this->RecordingTimeOffsetSec = vtkTimerLog::GetUniversalTime();
  this->LastSaveProxyNodesStateTimeSec = vtkTimerLog::GetUniversalTime();
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
  of << indent << " recordOnMasterModifiedOnly=\"" << (this->RecordMasterOnly ? "true" : "false") << "\"";

  std::string recordingSamplingModeString = this->GetRecordingSamplingModeAsString();
  if (!recordingSamplingModeString.empty())
  {
    of << indent << " recordingSamplingMode=\"" << recordingSamplingModeString << "\"";
  }

  std::string indexDisplayModeString = this->GetIndexDisplayModeAsString();
  if (!indexDisplayModeString.empty())
  {
    of << indent << " indexDisplayMode=\"" << indexDisplayModeString << "\"";
  }

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
    else if (!strcmp(attName, "recordOnMasterModifiedOnly"))
    {
      if (!strcmp(attValue, "true"))
      {
        this->SetRecordMasterOnly(1);
      }
      else
      {
        this->SetRecordMasterOnly(0);
      }
    }
    else if (!strcmp(attName, "recordingSamplingMode"))
    {
      int recordingSamplingMode = this->GetRecordingSamplingModeFromString(attValue);
      if (recordingSamplingMode<0 || recordingSamplingMode >= vtkMRMLSequenceBrowserNode::NumberOfRecordingSamplingModes)
      {
        vtkErrorMacro("Invalid recording sampling mode: " << (attValue ? attValue : "(empty). Using LimitedToPlaybackFrameRate."));
        recordingSamplingMode = vtkMRMLSequenceBrowserNode::SamplingLimitedToPlaybackFrameRate;
      }
      SetRecordingSamplingMode(recordingSamplingMode);
    }
    else if (!strcmp(attName, "indexDisplayMode"))
    {
      int indexDisplayMode = this->GetIndexDisplayModeFromString(attValue);
      if (indexDisplayMode<0 || indexDisplayMode >= vtkMRMLSequenceBrowserNode::NumberOfIndexDisplayModes)
      {
        vtkErrorMacro("Invalid index display mode: " << (attValue ? attValue : "(empty). Using IndexDisplayAsIndexValue."));
        indexDisplayMode = vtkMRMLSequenceBrowserNode::IndexDisplayAsIndexValue;
      }
      SetIndexDisplayMode(indexDisplayMode);
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
  vtkMRMLSequenceBrowserNode* node = vtkMRMLSequenceBrowserNode::SafeDownCast(anode);
  if (node == NULL)
  {
    vtkErrorMacro("Node copy failed: not a vtkMRMLSequenceNode");
    return;
  }

  int wasModified = this->StartModify();
  Superclass::Copy(anode);

  // Note: node references are copied by the superclass
  this->SynchronizationPostfixes = node->SynchronizationPostfixes;
  this->SynchronizationPropertiesMap = node->SynchronizationPropertiesMap;
  this->RecordingTimeOffsetSec = node->RecordingTimeOffsetSec;
  this->LastSaveProxyNodesStateTimeSec = node->LastSaveProxyNodesStateTimeSec;
  this->LastPostfixIndex = node->LastPostfixIndex;
  this->SetHideFromEditors(node->GetHideFromEditors());
  this->SetPlaybackActive(node->GetPlaybackActive());
  this->SetPlaybackRateFps(node->GetPlaybackRateFps());
  this->SetPlaybackItemSkippingEnabled(node->GetPlaybackItemSkippingEnabled());
  this->SetPlaybackLooped(node->GetPlaybackLooped());
  this->SetRecordMasterOnly(node->GetRecordMasterOnly());
  this->SetRecordingSamplingMode(node->GetRecordingSamplingMode());
  this->SetIndexDisplayMode(node->GetIndexDisplayMode());
  this->SetRecordingActive(node->GetRecordingActive());

  this->SetSelectedItemNumber(node->GetSelectedItemNumber());

  this->EndModify(wasModified);
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
  os << indent << " Recording on master modified only: " << (this->RecordMasterOnly ? "true" : "false") << '\n';
  os << indent << " Recording sampling mode: " << this->GetRecordingSamplingModeAsString() << "\n";
  os << indent << " Index display mode: " << this->GetIndexDisplayModeAsString() << "\n";

  os << indent << " Sequence nodes:\n";
  if (this->SynchronizationPostfixes.empty())
  {
    os << "(empty)\n";
  }
  else
  {
    for (std::vector< std::string >::iterator rolePostfixIt = this->SynchronizationPostfixes.begin();
      rolePostfixIt != this->SynchronizationPostfixes.end(); ++rolePostfixIt)
    {
      os << indent.GetNextIndent();
      if (rolePostfixIt->empty())
      {
        os << "(invalid)\n";
        continue;
      }
      std::string sequenceNodeReferenceRole = SEQUENCE_NODE_REFERENCE_ROLE_BASE + (*rolePostfixIt);
      vtkMRMLSequenceNode* sequenceNode = vtkMRMLSequenceNode::SafeDownCast(this->GetNodeReference(sequenceNodeReferenceRole.c_str()));
      if (sequenceNode != NULL)
      {
        os << "Sequence: " << (sequenceNode->GetName() ? sequenceNode->GetName() : "(unnamed)");
      }
      else
      {
        os << "Sequence: (none)";
      }
      std::string proxyNodeReferenceRole = PROXY_NODE_REFERENCE_ROLE_BASE + (*rolePostfixIt);
      vtkMRMLNode* proxyNode = vtkMRMLSequenceNode::SafeDownCast(this->GetNodeReference(proxyNodeReferenceRole.c_str()));
      if (proxyNode != NULL)
      {
        os << ", Proxy: " << (proxyNode->GetName() ? proxyNode->GetName() : "(unnamed)");
      }
      else
      {
        os << ", Proxy: (none)";
      }
      SynchronizationProperties* syncProps = this->SynchronizationPropertiesMap[*rolePostfixIt];
      if (syncProps != NULL)
      {
        os << ", Playback: " << syncProps->Playback;
        os << ", Recording: " << syncProps->Recording;
        os << ", OverwriteProxyName: " << syncProps->OverwriteProxyName;
        os << ", SaveChanges: " << syncProps->SaveChanges;
      }
      os << "\n";
    }
  }
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
std::string vtkMRMLSequenceBrowserNode::SetAndObserveMasterSequenceNodeID(const char *sequenceNodeID)
{
  if (!sequenceNodeID)
  {
    // master node can only be NULL if there are no synchronized nodes
    this->RemoveAllSequenceNodes();
    return "";
  }
  if (this->GetMasterSequenceNode()
    && this->GetMasterSequenceNode()->GetID() != NULL
    && strcmp(this->GetMasterSequenceNode()->GetID(), sequenceNodeID) == 0)
  {
    // no change
    if (!this->SynchronizationPostfixes.empty())
    {
      return this->SynchronizationPostfixes.front();
    }
    else
    {
      return "";
    }
  }
  std::string masterPostfix = this->GetSynchronizationPostfixFromSequenceID(sequenceNodeID);
  if (masterPostfix.empty() || this->GetMasterSequenceNode() == NULL)
  {
    bool oldModify = this->StartModify();
    // Master is not among the browsed nodes, or it is an empty browser node.
    this->RemoveAllSequenceNodes();
    // Master is the first element in the postfixes vector.
    std::string rolePostfix = this->AddSynchronizedSequenceNodeID(sequenceNodeID);
    this->SetSelectedItemNumber(0);
    this->EndModify(oldModify);
    return rolePostfix;
  }
  bool oldModify = this->StartModify();
  // Get the currently selected index value (so that we can restore the closest value with the new master)
  std::string lastSelectedIndexValue = this->GetMasterSequenceNode()->GetNthIndexValue(this->GetSelectedItemNumber());
  // Move the new master's postfix to the front of the list
  std::vector< std::string >::iterator oldMasterPostfixPosition = std::find(this->SynchronizationPostfixes.begin(), this->SynchronizationPostfixes.end(), masterPostfix);
  iter_swap(oldMasterPostfixPosition, this->SynchronizationPostfixes.begin());
  std::string rolePostfix = this->SynchronizationPostfixes.front();
  this->Modified();
  // Select the closest selected item index in the new master node
  vtkMRMLSequenceNode* newMasterNode = (this->Scene ? vtkMRMLSequenceNode::SafeDownCast(this->Scene->GetNodeByID(sequenceNodeID)) : NULL);
  if (newMasterNode != NULL)
  {
    int newSelectedIndex = newMasterNode->GetItemNumberFromIndexValue(lastSelectedIndexValue, false);
    this->SetSelectedItemNumber(newSelectedIndex);
  }
  else
  {
    // We don't know what items are in this new master node
    this->SetSelectedItemNumber(0);
  }
  this->EndModify(oldModify);
  return rolePostfix;
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
  this->SetSelectedItemNumber(-1);
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
std::string vtkMRMLSequenceBrowserNode::GetSynchronizationPostfixFromSequenceID(const char* sequenceNodeID)
{
  if (sequenceNodeID == NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::GetSynchronizationPostfixFromSequenceID failed: sequenceNodeID is invalid");
    return "";
  }
  for (std::vector< std::string >::iterator rolePostfixIt = this->SynchronizationPostfixes.begin();
    rolePostfixIt != this->SynchronizationPostfixes.end(); ++rolePostfixIt)
  {
    std::string sequenceNodeRef = SEQUENCE_NODE_REFERENCE_ROLE_BASE + (*rolePostfixIt);
    if (strcmp(this->GetNodeReferenceID(sequenceNodeRef.c_str()), sequenceNodeID) == 0)
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
vtkMRMLNode* vtkMRMLSequenceBrowserNode::AddProxyNode(vtkMRMLNode* sourceProxyNode, vtkMRMLSequenceNode* sequenceNode, bool copy /* =true */)
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

  // Save base name (proxy node name may be overwritten later)
  if (sourceProxyNode->GetAttribute("Sequences.BaseName") == NULL)
  {
    sourceProxyNode->SetAttribute("Sequences.BaseName", sourceProxyNode->GetName());
  }

  // Add copy of the data node
  std::string proxyNodeRef=PROXY_NODE_REFERENCE_ROLE_BASE+rolePostfix;
  // Create a new one from scratch in the new scene to make sure only the needed parts are copied
  vtkMRMLNode* proxyNode = sourceProxyNode;
  if ( copy )
  {
    proxyNode = sourceProxyNode->CreateNodeInstance();
    std::string proxyNodeName = sequenceNode->GetName();
    const char* sequenceBaseName = sequenceNode->GetAttribute("Sequences.Source");
    if (sequenceBaseName != NULL)
    {
      proxyNodeName = std::string(this->GetName())+"-"+sequenceBaseName;
    }
    proxyNode->SetName(proxyNodeName.c_str());
    this->Scene->AddNode(proxyNode);
    proxyNode->SetAttribute(PROXY_NODE_COPY_ATTRIBUTE_NAME,"true"); // Indicate that this is a copy
    proxyNode->Delete(); // ownership transferred to the scene, so we can release the pointer
  }

  vtkMRMLNode* oldProxyNode=this->GetNodeReference(proxyNodeRef.c_str());
  if (proxyNode!=oldProxyNode)
  {
    this->RemoveProxyNode(rolePostfix); // This will also remove the proxy node from the scene if necessary
    this->SetAndObserveNodeReferenceID(proxyNodeRef.c_str(), proxyNode->GetID(), vtkMRMLNodeSequencer::GetInstance()->GetNodeSequencer(proxyNode)->GetRecordingEvents());
  }

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
  std::string rolePostfix = this->GetSynchronizationPostfixFromSequenceID(synchronizedSequenceNodeId);
  if (!rolePostfix.empty())
  {
    // already a synchronized sequence node
    return rolePostfix;
  }
  rolePostfix = this->GenerateSynchronizationPostfix();
  if (this->SynchronizationPostfixes.empty())
  {
    // first sequence, initialize selected item number
    this->SetSelectedItemNumber(0);
  }
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
      // This might have been the last node that was being replayed or recorded
      this->SetPlaybackActive(false);
      this->SetRecordingActive(false);
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
int vtkMRMLSequenceBrowserNode::GetNumberOfSynchronizedSequenceNodes(bool includeMasterNode/*=false*/)
{
  int numberOfSynchronizedNodes = this->SynchronizationPostfixes.size();
  if (!includeMasterNode)
  {
    numberOfSynchronizedNodes--;
  }
  if (numberOfSynchronizedNodes < 0)
  {
    numberOfSynchronizedNodes = 0;
  }
  return numberOfSynchronizedNodes;
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::GetSynchronizedSequenceNodes(std::vector< vtkMRMLSequenceNode* > &synchronizedSequenceNodes, bool includeMasterNode/*=false*/)
{
  synchronizedSequenceNodes.clear();

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
    synchronizedSequenceNodes.push_back(synchronizedNode);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::GetSynchronizedSequenceNodes(vtkCollection* synchronizedSequenceNodes, bool includeMasterNode /* =false */)
{
  if (synchronizedSequenceNodes==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::GetSynchronizedSequenceNodes failed: synchronizedSequenceNodes is invalid");
    return;
  }
  std::vector< vtkMRMLSequenceNode* > synchronizedDataNodesVector;
  this->GetSynchronizedSequenceNodes(synchronizedDataNodesVector, includeMasterNode);
  synchronizedSequenceNodes->RemoveAllItems();
  for (std::vector< vtkMRMLSequenceNode* >::iterator it = synchronizedDataNodesVector.begin(); it != synchronizedDataNodesVector.end(); ++it)
  {
    synchronizedSequenceNodes->AddItem(*it);
  }
}

//---------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::SetRecordingActive(bool recording)
{
  // Before activating the recording, set the initial timestamp to be correct
  this->RecordingTimeOffsetSec = vtkTimerLog::GetUniversalTime();
  if (this->GetMasterSequenceNode()!=NULL 
    && this->GetMasterSequenceNode()->GetNumberOfDataNodes()>0
    && this->GetMasterSequenceNode()->GetIndexType()==vtkMRMLSequenceNode::NumericIndex)
  {
    std::stringstream timeString;
    timeString << this->GetMasterSequenceNode()->GetNthIndexValue( this->GetMasterSequenceNode()->GetNumberOfDataNodes() - 1 );
    double timeValue = 0;
    timeString >> timeValue;
    this->RecordingTimeOffsetSec -= timeValue;
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
  this->InvokeCustomModifiedEvent(ProxyNodeModifiedEvent, modifiedNode);
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
    double currentTime = vtkTimerLog::GetUniversalTime();
    double timeElapsedSinceLastSave = currentTime - this->LastSaveProxyNodesStateTimeSec;
    if (this->RecordingSamplingMode == vtkMRMLSequenceBrowserNode::SamplingLimitedToPlaybackFrameRate)
    {
      if (this->GetPlaybackRateFps() > 0 && (timeElapsedSinceLastSave < 1.0 / this->GetPlaybackRateFps()))
      {
        // this state is too close in time to the previous saved state, don't record it
        return;
      }
    }
    this->LastSaveProxyNodesStateTimeSec = currentTime;
    currTime << (currentTime - this->RecordingTimeOffsetSec);
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
  std::vector< vtkMRMLSequenceNode* > synchronizedSequenceNodes;
  if (sequenceNode)
  {
    synchronizedSequenceNodes.push_back(sequenceNode);
  }
  else
  {
    this->GetSynchronizedSequenceNodes(synchronizedSequenceNodes, true);
  }
  bool modified = false;
  for (std::vector< vtkMRMLSequenceNode* >::iterator it = synchronizedSequenceNodes.begin(); it != synchronizedSequenceNodes.end(); ++it)
  {
    std::string rolePostfix = this->GetSynchronizationPostfixFromSequence(*it);
    SynchronizationProperties* syncProps = this->SynchronizationPropertiesMap[rolePostfix];
    if (rolePostfix == "" || syncProps == NULL)
    {
      continue;
    }
    if (syncProps->Recording != recording)
    {
      syncProps->Recording = recording;
      modified = true;
    }
  }
  if (modified)
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::SetPlayback(vtkMRMLSequenceNode* sequenceNode, bool playback)
{
  std::vector< vtkMRMLSequenceNode* > synchronizedSequenceNodes;
  if (sequenceNode)
  {
    synchronizedSequenceNodes.push_back(sequenceNode);
  }
  else
  {
    this->GetSynchronizedSequenceNodes(synchronizedSequenceNodes, true);
  }
  bool modified = false;
  for (std::vector< vtkMRMLSequenceNode* >::iterator it = synchronizedSequenceNodes.begin(); it != synchronizedSequenceNodes.end(); ++it)
  {
    std::string rolePostfix = this->GetSynchronizationPostfixFromSequence(*it);
    SynchronizationProperties* syncProps = this->SynchronizationPropertiesMap[rolePostfix];
    if (rolePostfix == "" || syncProps == NULL)
    {
      continue;
    }
    if (syncProps->Playback != playback)
    {
      syncProps->Playback = playback;
      modified = true;
    }
  }
  if (modified)
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::SetOverwriteProxyName(vtkMRMLSequenceNode* sequenceNode, bool overwrite)
{
  std::vector< vtkMRMLSequenceNode* > synchronizedSequenceNodes;
  if (sequenceNode)
  {
    synchronizedSequenceNodes.push_back(sequenceNode);
  }
  else
  {
    this->GetSynchronizedSequenceNodes(synchronizedSequenceNodes, true);
  }
  bool modified = false;
  for (std::vector< vtkMRMLSequenceNode* >::iterator it = synchronizedSequenceNodes.begin(); it != synchronizedSequenceNodes.end(); ++it)
  {
    std::string rolePostfix = this->GetSynchronizationPostfixFromSequence(*it);
    SynchronizationProperties* syncProps = this->SynchronizationPropertiesMap[rolePostfix];
    if (rolePostfix == "" || syncProps == NULL)
    {
      continue;
    }
    if (syncProps->OverwriteProxyName != overwrite)
    {
      syncProps->OverwriteProxyName = overwrite;
      modified = true;
    }
  }
  if (modified)
  {
    this->Modified();
  }
}

//---------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::SetSaveChanges(vtkMRMLSequenceNode* sequenceNode, bool save)
{
  std::vector< vtkMRMLSequenceNode* > synchronizedSequenceNodes;
  if (sequenceNode)
  {
    synchronizedSequenceNodes.push_back(sequenceNode);
  }
  else
  {
    this->GetSynchronizedSequenceNodes(synchronizedSequenceNodes, true);
  }
  bool modified = false;
  for (std::vector< vtkMRMLSequenceNode* >::iterator it = synchronizedSequenceNodes.begin(); it != synchronizedSequenceNodes.end(); ++it)
  {
    std::string rolePostfix = this->GetSynchronizationPostfixFromSequence(*it);
    SynchronizationProperties* syncProps = this->SynchronizationPropertiesMap[rolePostfix];
    if (rolePostfix == "" || syncProps == NULL)
    {
      continue;
    }
    if (syncProps->SaveChanges != save)
    {
      syncProps->SaveChanges = save;
      modified = true;
    }
  }
  if (modified)
  {
    this->Modified();
  }
}

//-----------------------------------------------------------
void vtkMRMLSequenceBrowserNode::SetRecordingSamplingModeFromString(const char *recordingSamplingModeString)
{
  int recordingSamplingMode = GetRecordingSamplingModeFromString(recordingSamplingModeString);
  this->SetRecordingSamplingMode(recordingSamplingMode);
}

//-----------------------------------------------------------
std::string vtkMRMLSequenceBrowserNode::GetRecordingSamplingModeAsString()
{
  return vtkMRMLSequenceBrowserNode::GetRecordingSamplingModeAsString(this->RecordingSamplingMode);
}

//-----------------------------------------------------------
std::string vtkMRMLSequenceBrowserNode::GetRecordingSamplingModeAsString(int recordingSamplingMode)
{
  switch (recordingSamplingMode)
  {
  case vtkMRMLSequenceBrowserNode::SamplingAll: return "all";
  case vtkMRMLSequenceBrowserNode::SamplingLimitedToPlaybackFrameRate: return "limitedToPlaybackFrameRate";
  default:
    return "";
  }
}

//-----------------------------------------------------------
int vtkMRMLSequenceBrowserNode::GetRecordingSamplingModeFromString(const std::string& recordingSamplingModeString)
{
  for (int i = 0; i<vtkMRMLSequenceBrowserNode::NumberOfRecordingSamplingModes; i++)
  {
    if (recordingSamplingModeString == GetRecordingSamplingModeAsString(i))
    {
      // found it
      return i;
    }
  }
  return -1;
}

//-----------------------------------------------------------
void vtkMRMLSequenceBrowserNode::SetIndexDisplayModeFromString(const char *indexDisplayModeString)
{
  int indexDisplayMode = GetIndexDisplayModeFromString(indexDisplayModeString);
  this->SetIndexDisplayMode(indexDisplayMode);
}

//-----------------------------------------------------------
std::string vtkMRMLSequenceBrowserNode::GetIndexDisplayModeAsString()
{
  return vtkMRMLSequenceBrowserNode::GetIndexDisplayModeAsString(this->IndexDisplayMode);
}

//-----------------------------------------------------------
std::string vtkMRMLSequenceBrowserNode::GetIndexDisplayModeAsString(int indexDisplayMode)
{
  switch (indexDisplayMode)
  {
  case vtkMRMLSequenceBrowserNode::IndexDisplayAsIndex: return "[index]";
  case vtkMRMLSequenceBrowserNode::IndexDisplayAsIndexValue: return "[indexValue]";
  default:
    return "";
  }
}

//-----------------------------------------------------------
int vtkMRMLSequenceBrowserNode::GetIndexDisplayModeFromString(const std::string& indexDisplayModeString)
{
  for (int i = 0; i<vtkMRMLSequenceBrowserNode::NumberOfIndexDisplayModes; i++)
  {
    if (indexDisplayModeString == GetIndexDisplayModeAsString(i))
    {
      // found it
      return i;
    }
  }
  return -1;
}
