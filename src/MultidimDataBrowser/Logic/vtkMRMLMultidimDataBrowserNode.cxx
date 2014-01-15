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

// MRMLMultidim includes
#include "vtkMRMLMultidimDataBrowserNode.h"
#include "vtkMRMLMultidimDataNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLHierarchyNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

// First reference is the master root node, subsequent references are the synchronized root nodes
static const char* ROOT_NODE_REFERENCE_ROLE_BASE = "rootNodeRef";
static const char* DATA_NODE_REFERENCE_ROLE_BASE = "dataNodeRef";
static const char* DISPLAY_NODES_REFERENCE_ROLE_BASE = "displayNodesRef";

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLMultidimDataBrowserNode);

//----------------------------------------------------------------------------
vtkMRMLMultidimDataBrowserNode::vtkMRMLMultidimDataBrowserNode()
{
  this->SetHideFromEditors(false);
  this->PlaybackActive=false;
  this->PlaybackRateFps=5.0;
  this->PlaybackLooped=true;
  this->SelectedBundleIndex=0;
  this->LastPostfixIndex=0;
}

//----------------------------------------------------------------------------
vtkMRMLMultidimDataBrowserNode::~vtkMRMLMultidimDataBrowserNode()
{
 
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataBrowserNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  vtkIndent indent(nIndent);

  of << indent << " playbackActive=\"" << (this->PlaybackActive ? "true" : "false") << "\"";
  of << indent << " playbackRateFps=\"" << this->PlaybackRateFps << "\""; 
  of << indent << " playbackLooped=\"" << (this->PlaybackLooped ? "true" : "false") << "\"";  
  of << indent << " selectedBundleIndex=\"" << this->SelectedBundleIndex << "\"";

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
void vtkMRMLMultidimDataBrowserNode::ReadXMLAttributes(const char** atts)
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
    else if (!strcmp(attName, "selectedBundleIndex")) 
    {
      std::stringstream ss;
      ss << attValue;
      int selectedBundleIndex=0;
      ss >> selectedBundleIndex;
      this->SetSelectedBundleIndex(selectedBundleIndex);
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
void vtkMRMLMultidimDataBrowserNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  vtkMRMLMultidimDataBrowserNode* node=vtkMRMLMultidimDataBrowserNode::SafeDownCast(anode);
  if (node==NULL)
  {
    vtkErrorMacro("Node copy failed: not a vtkMRMLMultidimDataNode");
    return;
  }
  this->VirtualNodePostfixes=node->VirtualNodePostfixes;
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataBrowserNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
std::string vtkMRMLMultidimDataBrowserNode::GenerateVirtualNodePostfix()
{
  std::stringstream postfix;
  postfix << this->LastPostfixIndex;
  this->LastPostfixIndex++;
  return postfix.str();  
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataBrowserNode::SetAndObserveRootNodeID(const char *rootNodeID)
{
  bool oldModify=this->StartModify();
  std::string rolePostfix;
  if (!this->VirtualNodePostfixes.empty())
  {
    RemoveAllRootNodes();
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
vtkMRMLMultidimDataNode* vtkMRMLMultidimDataBrowserNode::GetRootNode()
{
  if (this->VirtualNodePostfixes.empty())
  {
    return NULL;
  }  
  std::string rootNodeReferenceRole=ROOT_NODE_REFERENCE_ROLE_BASE+this->VirtualNodePostfixes[0];  
  vtkMRMLMultidimDataNode* node=vtkMRMLMultidimDataNode::SafeDownCast(this->GetNodeReference(rootNodeReferenceRole.c_str()));
  return node;
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataBrowserNode::RemoveAllVirtualOutputNodes()
{
  bool oldModify=this->StartModify();
  for (std::vector< std::string >::iterator rolePostfixIt=this->VirtualNodePostfixes.begin();
    rolePostfixIt!=this->VirtualNodePostfixes.end(); ++rolePostfixIt)
  {
    RemoveVirtualOutputDataNode(*rolePostfixIt);
    RemoveVirtualOutputDisplayNodes(*rolePostfixIt);
  }
  this->EndModify(oldModify);
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataBrowserNode::RemoveAllRootNodes()
{
  bool oldModify=this->StartModify();
  // need to make a copy as this->VirtualNodePostfixes changes as we remove nodes
  std::vector< std::string > virtualNodePostfixes=this->VirtualNodePostfixes;
  // start from the end to delete the master root node last
  for (std::vector< std::string >::reverse_iterator rolePostfixIt=virtualNodePostfixes.rbegin();
    rolePostfixIt!=virtualNodePostfixes.rend(); ++rolePostfixIt)
  {
    std::string rootNodeReferenceRole=ROOT_NODE_REFERENCE_ROLE_BASE+(*rolePostfixIt);
    vtkMRMLMultidimDataNode* node=vtkMRMLMultidimDataNode::SafeDownCast(this->GetNodeReference(rootNodeReferenceRole.c_str()));
    if (node==NULL)
    {
      vtkErrorMacro("Invalid root node");
      continue;
    }
    RemoveSynchronizedRootNode(node->GetID());
  }
  this->EndModify(oldModify);
}


//----------------------------------------------------------------------------
std::string vtkMRMLMultidimDataBrowserNode::GetVirtualNodePostfixFromRoot(vtkMRMLMultidimDataNode* rootNode)
{
  if (rootNode==NULL)
  {
    vtkErrorMacro("vtkMRMLMultidimDataBrowserNode::GetVirtualNodePostfixFromRoot failed: rootNode is invalid");
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
vtkMRMLNode* vtkMRMLMultidimDataBrowserNode::GetVirtualOutputDataNode(vtkMRMLMultidimDataNode* rootNode)
{
  if (rootNode==NULL)
  {
    vtkErrorMacro("vtkMRMLMultidimDataBrowserNode::GetVirtualOutputNode failed: rootNode is invalid");
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
void vtkMRMLMultidimDataBrowserNode::GetVirtualOutputDisplayNodes(vtkMRMLMultidimDataNode* rootNode, std::vector< vtkMRMLDisplayNode* > &displayNodes)
{
  displayNodes.clear();
  vtkMRMLNode* dataNode=GetVirtualOutputDataNode(rootNode);
  if (dataNode==NULL)
  {
    vtkErrorMacro("vtkMRMLMultidimDataBrowserNode::GetVirtualOutputDisplayNodes failed: rootNode is invalid");
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
    displayNodes.push_back(displayNode);
  }
}


//----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLMultidimDataBrowserNode::AddVirtualOutputNodes(vtkMRMLNode* sourceDataNode, std::vector< vtkMRMLDisplayNode* > &sourceDisplayNodes, vtkMRMLMultidimDataNode* rootNode)
{
  if (rootNode==NULL)
  {
    vtkErrorMacro("vtkMRMLMultidimDataBrowserNode::AddVirtualOutputNode failed: rootNode is invalid");
    return NULL;
  }
  if (this->Scene==NULL)
  {
    vtkErrorMacro("vtkMRMLMultidimDataBrowserNode::AddVirtualOutputNode failed: scene is invalid");
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
  
  // Add copy of the display node(s)  
  std::string displayNodesRef=DISPLAY_NODES_REFERENCE_ROLE_BASE+rolePostfix;
  this->RemoveVirtualOutputDisplayNodes(rolePostfix);
  for (std::vector< vtkMRMLDisplayNode* >::iterator sourceDisplayNodeIt=sourceDisplayNodes.begin(); sourceDisplayNodeIt!=sourceDisplayNodes.end(); ++sourceDisplayNodeIt)
  {
    vtkMRMLDisplayNode* sourceDisplayNode=(*sourceDisplayNodeIt);
    vtkMRMLDisplayNode* displayNode=vtkMRMLDisplayNode::SafeDownCast(this->Scene->CopyNode(sourceDisplayNode));
    this->AddNodeReferenceID(displayNodesRef.c_str(), displayNode->GetID());    
  }

  this->EndModify(oldModify);
  return dataNode;
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataBrowserNode::GetAllVirtualOutputDataNodes(std::vector< vtkMRMLNode* >& nodes)
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
void vtkMRMLMultidimDataBrowserNode::RemoveVirtualOutputDataNode(const std::string& postfix)
{
  bool oldModify=this->StartModify();
  std::string dataNodeRef=DATA_NODE_REFERENCE_ROLE_BASE+postfix;
  vtkMRMLNode* dataNode=this->GetNodeReference(dataNodeRef.c_str());
  if (dataNode!=NULL)
  {
    this->Scene->RemoveNode(dataNode);
    this->RemoveAllNodeReferenceIDs(dataNodeRef.c_str());
  }
  this->EndModify(oldModify);
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataBrowserNode::RemoveVirtualOutputDisplayNodes(const std::string& postfix)
{
  bool oldModify=this->StartModify();
  std::vector< vtkMRMLNode* > displayNodes;
  std::string displayNodesRef=DISPLAY_NODES_REFERENCE_ROLE_BASE+postfix;
  GetNodeReferences(displayNodesRef.c_str(), displayNodes);
  for (std::vector< vtkMRMLNode* >::iterator displayNodeIt=displayNodes.begin(); displayNodeIt!=displayNodes.end(); ++displayNodeIt)
  {
    this->Scene->RemoveNode(*displayNodeIt);
  }
  this->RemoveAllNodeReferenceIDs(displayNodesRef.c_str());  
  this->EndModify(oldModify);
}

//----------------------------------------------------------------------------
bool vtkMRMLMultidimDataBrowserNode::IsSynchronizedRootNode(const char* nodeId)
{
  if (nodeId==NULL)
  {
    vtkWarningMacro("vtkMRMLMultidimDataBrowserNode::IsSynchronizedRootNode nodeId is NULL");
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
std::string vtkMRMLMultidimDataBrowserNode::AddSynchronizedRootNode(const char* synchronizedRootNodeId)
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
void vtkMRMLMultidimDataBrowserNode::RemoveSynchronizedRootNode(const char* nodeId)
{
  if (this->Scene==NULL)
  {
    vtkErrorMacro("vtkMRMLMultidimDataBrowserNode::RemoveSynchronizedRootNode failed: scene is invalid");
    return;
  }
  if (nodeId==NULL)
  {
    vtkErrorMacro("vtkMRMLMultidimDataBrowserNode::RemoveSynchronizedRootNode failed: nodeId is invalid");
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
      bool oldModify=this->StartModify();
      this->RemoveAllNodeReferenceIDs(rootNodeRef.c_str());
      RemoveVirtualOutputDataNode(*rolePostfixIt);
      RemoveVirtualOutputDisplayNodes(*rolePostfixIt);
      this->VirtualNodePostfixes.erase(rolePostfixIt);
      this->EndModify(oldModify);
      return;
    }
  }
  vtkWarningMacro("vtkMRMLMultidimDataBrowserNode::RemoveSynchronizedRootNode did nothing, the specified node was not synchronized");
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataBrowserNode::GetSynchronizedRootNodes(std::vector< vtkMRMLMultidimDataNode* > &synchronizedDataNodes, bool includeMasterNode/*=false*/)
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
    vtkMRMLMultidimDataNode* synchronizedNode=vtkMRMLMultidimDataNode::SafeDownCast(this->GetNodeReference(rootNodeRef.c_str()));
    if (synchronizedNode==NULL)
    {
      // valid case during scene updates
      continue;
    }
    synchronizedDataNodes.push_back(synchronizedNode);
  }
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataBrowserNode::GetNthSynchronizedDataNodes(std::vector< vtkMRMLNode* > &dataNodes, int selectedBundleIndex)
{
  dataNodes.clear();

  for (std::vector< std::string >::iterator rolePostfixIt=this->VirtualNodePostfixes.begin();
    rolePostfixIt!=this->VirtualNodePostfixes.end(); ++rolePostfixIt)
  {  
    std::string rootNodeRef=ROOT_NODE_REFERENCE_ROLE_BASE+(*rolePostfixIt);
    vtkMRMLMultidimDataNode* synchronizedRootNode=vtkMRMLMultidimDataNode::SafeDownCast(this->GetNodeReference(rootNodeRef.c_str()));
    if (synchronizedRootNode==NULL)
    {
      // valid case during scene updates
      continue;
    }
    if (selectedBundleIndex<0 || selectedBundleIndex>synchronizedRootNode->GetNumberOfDataNodes())
    {
      vtkErrorMacro("vtkMRMLMultidimDataBrowserNode::GetNthDataNodes failed: selectedBundleIndex is out of range");
      continue;
    } 
    dataNodes.push_back(synchronizedRootNode->GetNthDataNode(selectedBundleIndex));
  }
}
