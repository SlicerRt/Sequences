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
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>
#include <algorithm> // for std::find

// First reference is the master root node, subsequent references are the synchronized root nodes
static const char* ROOT_NODE_REFERENCE_ROLE_BASE = "rootNodeRef";
static const char* DATA_NODE_REFERENCE_ROLE_BASE = "dataNodeRef";
static const char* DISPLAY_NODES_REFERENCE_ROLE_BASE = "displayNodesRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLSequenceBrowserNode);

//----------------------------------------------------------------------------
vtkMRMLSequenceBrowserNode::vtkMRMLSequenceBrowserNode()
{
  this->SetHideFromEditors(false);
  this->PlaybackActive=false;
  this->PlaybackRateFps=10.0;
  this->PlaybackLooped=true;
  this->SelectedItemNumber=0;
  this->LastPostfixIndex=0;
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
  of << indent << " playbackLooped=\"" << (this->PlaybackLooped ? "true" : "false") << "\"";  
  of << indent << " selectedItemNumber=\"" << this->SelectedItemNumber << "\"";

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
  vtkMRMLNode::PrintSelf(os,indent);
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
void vtkMRMLSequenceBrowserNode::SetAndObserveRootNodeID(const char *rootNodeID)
{
  bool oldModify=this->StartModify();
  std::string rolePostfix;
  if (!this->VirtualNodePostfixes.empty())
  {
    this->RemoveAllRootNodes();
  }
  if (rootNodeID!=NULL)
  {
    rolePostfix=GenerateVirtualNodePostfix();
    this->VirtualNodePostfixes.push_back(rolePostfix);
    std::string rootNodeReferenceRole=ROOT_NODE_REFERENCE_ROLE_BASE+rolePostfix;
    this->SetAndObserveNodeReferenceID(rootNodeReferenceRole.c_str(), rootNodeID);
  }
  this->EndModify(oldModify);
}

//----------------------------------------------------------------------------
vtkMRMLSequenceNode* vtkMRMLSequenceBrowserNode::GetRootNode()
{
  if (this->VirtualNodePostfixes.empty())
  {
    return NULL;
  }  
  std::string rootNodeReferenceRole=ROOT_NODE_REFERENCE_ROLE_BASE+this->VirtualNodePostfixes[0];  
  vtkMRMLSequenceNode* node=vtkMRMLSequenceNode::SafeDownCast(this->GetNodeReference(rootNodeReferenceRole.c_str()));
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
void vtkMRMLSequenceBrowserNode::RemoveAllRootNodes()
{
  bool oldModify=this->StartModify();
  // need to make a copy as this->VirtualNodePostfixes changes as we remove nodes
  std::vector< std::string > virtualNodePostfixes=this->VirtualNodePostfixes;
  // start from the end to delete the master root node last
  for (std::vector< std::string >::reverse_iterator rolePostfixIt=virtualNodePostfixes.rbegin();
    rolePostfixIt!=virtualNodePostfixes.rend(); ++rolePostfixIt)
  {
    std::string rootNodeReferenceRole=ROOT_NODE_REFERENCE_ROLE_BASE+(*rolePostfixIt);
    vtkMRMLSequenceNode* node=vtkMRMLSequenceNode::SafeDownCast(this->GetNodeReference(rootNodeReferenceRole.c_str()));
    if (node==NULL)
    {
      vtkErrorMacro("Invalid root node");
      std::vector< std::string >::iterator rolePostfixInOriginalIt=std::find(this->VirtualNodePostfixes.begin(), this->VirtualNodePostfixes.end(), (*rolePostfixIt));
      if (rolePostfixInOriginalIt!=this->VirtualNodePostfixes.end())
      {
        this->VirtualNodePostfixes.erase(rolePostfixInOriginalIt);
      }
      continue;
    }
    this->RemoveSynchronizedRootNode(node->GetID());
  }
  this->EndModify(oldModify);
}


//----------------------------------------------------------------------------
std::string vtkMRMLSequenceBrowserNode::GetVirtualNodePostfixFromRoot(vtkMRMLSequenceNode* rootNode)
{
  if (rootNode==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::GetVirtualNodePostfixFromRoot failed: rootNode is invalid");
    return "";
  }
  for (std::vector< std::string >::iterator rolePostfixIt=this->VirtualNodePostfixes.begin();
    rolePostfixIt!=this->VirtualNodePostfixes.end(); ++rolePostfixIt)
  {
    std::string dataNodeRef=ROOT_NODE_REFERENCE_ROLE_BASE+(*rolePostfixIt);
    if (this->GetNodeReference(dataNodeRef.c_str())==rootNode)
    {
      return (*rolePostfixIt);
    }
  }
  return "";
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLSequenceBrowserNode::GetVirtualOutputDataNode(vtkMRMLSequenceNode* rootNode)
{
  if (rootNode==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::GetVirtualOutputNode failed: rootNode is invalid");
    return NULL;
  }
  std::string rolePostfix=GetVirtualNodePostfixFromRoot(rootNode);
  if (rolePostfix.empty())
  {
    return NULL;
  }
  std::string dataNodeRef=DATA_NODE_REFERENCE_ROLE_BASE+rolePostfix;
  return this->GetNodeReference(dataNodeRef.c_str());
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::GetVirtualOutputDisplayNodes(vtkMRMLSequenceNode* rootNode, std::vector< vtkMRMLDisplayNode* > &displayNodes)
{
  displayNodes.clear();
  vtkMRMLNode* dataNode=GetVirtualOutputDataNode(rootNode);
  if (dataNode==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::GetVirtualOutputDisplayNodes failed: rootNode is invalid");
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
    // Do we need to check if displayNode is NULL?
    displayNodes.push_back(displayNode);
  }
}


//----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLSequenceBrowserNode::AddVirtualOutputNodes(vtkMRMLNode* sourceDataNode, std::vector< vtkMRMLDisplayNode* > &sourceDisplayNodes, vtkMRMLSequenceNode* rootNode)
{
  if (rootNode==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::AddVirtualOutputNode failed: rootNode is invalid");
    return NULL;
  }
  if (this->Scene==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::AddVirtualOutputNode failed: scene is invalid");
    return NULL;
  }

  bool oldModify=this->StartModify();
  
  std::string rolePostfix=GetVirtualNodePostfixFromRoot(rootNode);
  if (rolePostfix.empty())
  {
    // Add reference to the root node
    rolePostfix=AddSynchronizedRootNode(rootNode->GetID());
  }

  // Add copy of the data node
  std::string dataNodeRef=DATA_NODE_REFERENCE_ROLE_BASE+rolePostfix;
  // Create a new one from scratch in the new scene to make sure only the needed parts are copied
  vtkMRMLNode* dataNode=sourceDataNode->CreateNodeInstance();
  this->Scene->AddNode(dataNode);
  dataNode->Delete(); // ownership transferred to the scene, so we can release the pointer
  this->SetNodeReferenceID(dataNodeRef.c_str(), dataNode->GetID());
  vtkMRMLDisplayableNode* displayableNode=vtkMRMLDisplayableNode::SafeDownCast(dataNode);
  
  // Add copy of the display node(s)
  std::string displayNodesRef=DISPLAY_NODES_REFERENCE_ROLE_BASE+rolePostfix;
  this->RemoveVirtualOutputDisplayNodes(rolePostfix);
  for (std::vector< vtkMRMLDisplayNode* >::iterator sourceDisplayNodeIt=sourceDisplayNodes.begin(); sourceDisplayNodeIt!=sourceDisplayNodes.end(); ++sourceDisplayNodeIt)
  {
    vtkMRMLDisplayNode* sourceDisplayNode=(*sourceDisplayNodeIt);
    vtkMRMLDisplayNode* displayNode=vtkMRMLDisplayNode::SafeDownCast(this->Scene->CopyNode(sourceDisplayNode));
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
    nodes.push_back(this->GetNodeReference(dataNodeRef.c_str()));
  }
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
bool vtkMRMLSequenceBrowserNode::IsSynchronizedRootNode(const char* nodeId)
{
  if (nodeId==NULL)
  {
    vtkWarningMacro("vtkMRMLSequenceBrowserNode::IsSynchronizedRootNode nodeId is NULL");
    return false;
  }
  for (std::vector< std::string >::iterator rolePostfixIt=this->VirtualNodePostfixes.begin();
    rolePostfixIt!=this->VirtualNodePostfixes.end(); ++rolePostfixIt)
  {
    if (rolePostfixIt==this->VirtualNodePostfixes.begin())
    {
      // the first one is the master root node, don't consider as a synchronized root node
      continue;
    }
    std::string rootNodeRef=ROOT_NODE_REFERENCE_ROLE_BASE+(*rolePostfixIt);
    const char* foundNodeId=this->GetNodeReferenceID(rootNodeRef.c_str());
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
std::string vtkMRMLSequenceBrowserNode::AddSynchronizedRootNode(const char* synchronizedRootNodeId)
{
  bool oldModify=this->StartModify();
  std::string rolePostfix=GenerateVirtualNodePostfix();
  this->VirtualNodePostfixes.push_back(rolePostfix);
  std::string rootNodeReferenceRole=ROOT_NODE_REFERENCE_ROLE_BASE+rolePostfix;
  this->SetAndObserveNodeReferenceID(rootNodeReferenceRole.c_str(), synchronizedRootNodeId);
  this->EndModify(oldModify);
  return rolePostfix;
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::RemoveSynchronizedRootNode(const char* nodeId)
{
  if (this->Scene==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::RemoveSynchronizedRootNode failed: scene is invalid");
    return;
  }
  if (nodeId==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceBrowserNode::RemoveSynchronizedRootNode failed: nodeId is invalid");
    return;
  }

  for (std::vector< std::string >::iterator rolePostfixIt=this->VirtualNodePostfixes.begin();
    rolePostfixIt!=this->VirtualNodePostfixes.end(); ++rolePostfixIt)
  {
    std::string rootNodeRef=ROOT_NODE_REFERENCE_ROLE_BASE+(*rolePostfixIt);
    const char* foundNodeId=this->GetNodeReferenceID(rootNodeRef.c_str());
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
      this->RemoveNodeReferenceIDs(rootNodeRef.c_str());
      this->RemoveVirtualOutputDataNode(rolePostfix);
      this->RemoveVirtualOutputDisplayNodes(rolePostfix);      
      this->EndModify(oldModify);
      return;
    }
  }
  vtkWarningMacro("vtkMRMLSequenceBrowserNode::RemoveSynchronizedRootNode did nothing, the specified node was not synchronized");
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::GetSynchronizedRootNodes(std::vector< vtkMRMLSequenceNode* > &synchronizedDataNodes, bool includeMasterNode/*=false*/)
{
  synchronizedDataNodes.clear();

  for (std::vector< std::string >::iterator rolePostfixIt=this->VirtualNodePostfixes.begin();
    rolePostfixIt!=this->VirtualNodePostfixes.end(); ++rolePostfixIt)
  {
    if (!includeMasterNode && rolePostfixIt==this->VirtualNodePostfixes.begin())
    {
      // the first one is the master root node, don't consider as a synchronized root node
      continue;
    }
    std::string rootNodeRef=ROOT_NODE_REFERENCE_ROLE_BASE+(*rolePostfixIt);
    vtkMRMLSequenceNode* synchronizedNode=vtkMRMLSequenceNode::SafeDownCast(this->GetNodeReference(rootNodeRef.c_str()));
    if (synchronizedNode==NULL)
    {
      // valid case during scene updates
      continue;
    }
    synchronizedDataNodes.push_back(synchronizedNode);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceBrowserNode::ScalarVolumeAutoWindowLevelOff()
{
  vtkMRMLSequenceNode* rootNode = this->GetRootNode();
  if (rootNode==NULL)
  {
    vtkWarningMacro("vtkMRMLSequenceBrowserNode::ScalarVolumeAutoWindowLevelOff failed: root node is invalid");
    return;
  }
  std::vector< vtkMRMLDisplayNode* > displayNodes;
  this->GetVirtualOutputDisplayNodes(rootNode, displayNodes);
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
int vtkMRMLSequenceBrowserNode::SelectFirstItem()
{
  vtkMRMLSequenceNode* rootNode = this->GetRootNode();
  int selectedItemNumber = -1;
  if (rootNode && rootNode->GetNumberOfDataNodes()>0)
  {
    selectedItemNumber = 0;
  }
  this->SetSelectedItemNumber(selectedItemNumber );
  return selectedItemNumber;
}

//---------------------------------------------------------------------------
int vtkMRMLSequenceBrowserNode::SelectLastItem()
{
  vtkMRMLSequenceNode* rootNode = this->GetRootNode();
  int selectedItemNumber = -1;
  if (rootNode && rootNode->GetNumberOfDataNodes()>0)
  {
    selectedItemNumber = rootNode->GetNumberOfDataNodes()-1;
  }
  this->SetSelectedItemNumber(selectedItemNumber );
  return selectedItemNumber;
}

//---------------------------------------------------------------------------
int vtkMRMLSequenceBrowserNode::SelectNextItem(int selectionIncrement/*=1*/)
{
  vtkMRMLSequenceNode* rootNode=this->GetRootNode();
  if (rootNode==NULL || rootNode->GetNumberOfDataNodes()==0)
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
    if (selectedItemNumber>=rootNode->GetNumberOfDataNodes())
    {
      if (this->GetPlaybackLooped())
      {
        // wrap around and keep playback going
        selectedItemNumber = selectedItemNumber % rootNode->GetNumberOfDataNodes();
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
        selectedItemNumber = (selectedItemNumber % rootNode->GetNumberOfDataNodes()) + rootNode->GetNumberOfDataNodes();
      }
      else
      {
        this->SetPlaybackActive(false);
        selectedItemNumber=rootNode->GetNumberOfDataNodes()-1;
      }
    }
  }
  this->SetSelectedItemNumber(selectedItemNumber);
  this->EndModify(browserNodeModify);
  return selectedItemNumber;
}
