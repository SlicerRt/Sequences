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
#include <vtkMRMLHierarchyNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

static const char* ROOT_NODE_REFERENCE_ROLE = "rootNodeRef";

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

  of << indent << " virtualNodeRoleNames=\"";
  for(std::set< std::string >::iterator roleNameIt=this->VirtualNodeRoleNames.begin();
    roleNameIt!=this->VirtualNodeRoleNames.end(); ++roleNameIt)
  { 
    if (roleNameIt!=this->VirtualNodeRoleNames.begin())
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
    else if (!strcmp(attName, "virtualNodeRoleNames"))
    {
      this->VirtualNodeRoleNames.clear();
      std::stringstream ss(attValue);
      while (!ss.eof())
      {
        std::string roleName;
        ss >> roleName;
        if (!roleName.empty())
        {
          this->VirtualNodeRoleNames.insert(roleName);
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
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataBrowserNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataBrowserNode::SetAndObserveRootNodeID(const char *rootNodeID)
{
  this->SetAndObserveNodeReferenceID(ROOT_NODE_REFERENCE_ROLE, rootNodeID);
}

//----------------------------------------------------------------------------
vtkMRMLMultidimDataNode* vtkMRMLMultidimDataBrowserNode::GetRootNode()
{
  vtkMRMLMultidimDataNode* node=vtkMRMLMultidimDataNode::SafeDownCast(this->GetNodeReference(ROOT_NODE_REFERENCE_ROLE));
  return node;
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataBrowserNode::RemoveAllVirtualOutputNodes()
{
  for(std::set< std::string >::iterator roleNameIt=this->VirtualNodeRoleNames.begin();
    roleNameIt!=this->VirtualNodeRoleNames.end(); ++roleNameIt)
  {
    RemoveAllNodeReferenceIDs(roleNameIt->c_str());
  }
  this->VirtualNodeRoleNames.clear();
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLMultidimDataBrowserNode::GetVirtualOutputNode(const char* dataRole)
{
  if (dataRole==NULL)
  {
    vtkErrorMacro("vtkMRMLMultidimDataBrowserNode::GetVirtualOutputNode failed: dataRole is invalid");
    return NULL;
  }
  return this->GetNodeReference(dataRole);
}

//----------------------------------------------------------------------------
void  vtkMRMLMultidimDataBrowserNode::AddVirtualOutputNode(vtkMRMLNode* targetOutputNode, const char* dataRole)
{
  this->VirtualNodeRoleNames.insert(dataRole);
  this->SetNodeReferenceID(dataRole, targetOutputNode->GetID());
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataBrowserNode::GetAllVirtualOutputNodes(std::vector< vtkMRMLNode* >& nodes)
{
  nodes.clear();
  for(std::set< std::string >::iterator roleNameIt=this->VirtualNodeRoleNames.begin();
    roleNameIt!=this->VirtualNodeRoleNames.end(); ++roleNameIt)
  {
    nodes.push_back(this->GetNodeReference(roleNameIt->c_str()));
  }
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataBrowserNode::RemoveVirtualOutputNode(vtkMRMLNode* node)
{
  if (node==NULL)
  {
    vtkErrorMacro("vtkMRMLMultidimDataBrowserNode::RemoveVirtualOutputNode failed: node is invalid");
    return;
  }
  for(std::set< std::string >::iterator roleNameIt=this->VirtualNodeRoleNames.begin();
    roleNameIt!=this->VirtualNodeRoleNames.end(); ++roleNameIt)
  {
    if (GetNodeReference(roleNameIt->c_str())==node)
    {
      // found
      RemoveAllNodeReferenceIDs(roleNameIt->c_str());
      this->VirtualNodeRoleNames.erase(*roleNameIt);      
      return;
    }
  }
  vtkWarningMacro("vtkMRMLMultidimDataBrowserNode::RemoveVirtualOutputNode failed: node is not found");
}