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

==============================================================================*/

// MRMLMultidimData includes
#include "vtkMRMLMultidimDataNode.h"

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkCollection.h>
#include <vtkObjectFactory.h>
//#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

#define SAFE_CHAR_POINTER(unsafeString) ( unsafeString==NULL?"":unsafeString )

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLMultidimDataNode);

//----------------------------------------------------------------------------
vtkMRMLMultidimDataNode::vtkMRMLMultidimDataNode()
: DimensionName(0)
, Unit(0)
{
  this->HideFromEditors = false;
}

//----------------------------------------------------------------------------
vtkMRMLMultidimDataNode::~vtkMRMLMultidimDataNode()
{
  SetDimensionName(NULL);
  SetUnit(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  {
    if (this->DimensionName != NULL)
    {
      of << indent << " dimensionName=\"" << this->DimensionName << "\"";
    }
    if (this->Unit != NULL)
    {
      of << indent << " unit=\"" << this->Unit << "\"";
    }

    of << indent << " bundles=\"";
    for(std::deque< MultidimBundleType >::iterator bundleIt=this->Bundles.begin(); bundleIt!=this->Bundles.end(); ++bundleIt)
    {
      if (bundleIt!=this->Bundles.begin())
      {
        // not the first bundle, add a separator before adding values
        of << ";";
      }
      of << bundleIt->ParameterValue << ":";
      for(RoleSetType::iterator roleNameIt=bundleIt->Roles.begin(); roleNameIt!=bundleIt->Roles.end(); ++roleNameIt)
      {
        if (roleNameIt!=bundleIt->Roles.begin())
        {
          // not the first role name, add a separator before adding values
          of << " ";
        }
        of << *roleNameIt;
      }
    }
    of << "\"";
  }
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName;
  const char* attValue;
  while (*atts != NULL) 
  {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "dimensionName")) 
    {
      this->SetDimensionName(attValue);
    }
    else if (!strcmp(attName, "unit")) 
    {
      this->SetUnit(attValue);
    }
    else if (!strcmp(attName, "bundles"))
    {
      this->ParseBundlesAttribute(attValue, this->Bundles);
    }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataNode::ParseBundlesAttribute(const char *attValue, std::deque< MultidimBundleType >& bundles)
{
  bundles.clear();

  /// parse refernces in the form "parameterValue1:role1 role2;parameterValue2:role1 role2;"
  std::string attribute(attValue);

  std::size_t bundleStart = 0;
  std::size_t bundleEnd = attribute.find_first_of(';', bundleStart);
  std::size_t parameterValueSep = attribute.find_first_of(':', bundleStart);
  while (bundleStart != std::string::npos && parameterValueSep != std::string::npos && bundleStart != bundleEnd && bundleStart != parameterValueSep)
  {
    std::string ref = attribute.substr(bundleStart, bundleEnd-bundleStart);
    std::string parameterValue = attribute.substr(bundleStart, parameterValueSep-bundleStart);
    if (parameterValue.empty())
    {
      vtkWarningMacro("vtkMRMLNode::ParseBundlesAttribute: parsing error, ParameterValue is empty");
      continue;
    }
    MultidimBundleType bundle;
    bundle.ParameterValue=parameterValue;
    std::string roleNames = attribute.substr(parameterValueSep+1, bundleEnd-parameterValueSep-1);
    std::stringstream ss(roleNames);
    while (!ss.eof())
    {
      std::string roleName;
      ss >> roleName;
      if (!roleName.empty())
      {
        bundle.Roles.insert(roleName);
      }
    }
    bundles.push_back(bundle);
    bundleStart = (bundleEnd == std::string::npos) ? std::string::npos : bundleEnd+1;
    bundleEnd = attribute.find_first_of(';', bundleStart);
    parameterValueSep = attribute.find_first_of(':', bundleStart);
  }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLMultidimDataNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLMultidimDataNode *node = (vtkMRMLMultidimDataNode *) anode;

  this->Bundles=node->Bundles;
  this->SetDimensionName(node->GetDimensionName());
  this->SetUnit(node->GetUnit());

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);

  //os << indent << "BeamModelOpacity:   " << this->BeamModelOpacity << "\n";
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataNode::SetDataNodeAtValue(vtkMRMLNode* node, const char* nodeRole, const char* parameterValue)
{
  if (node==NULL)
  {
    vtkErrorMacro("vtkMRMLMultidimDataNode::SetDataNodeAtValue failed, invalid node"); 
    return;
  }
  if (node->GetID()==NULL)
  {
    vtkErrorMacro("vtkMRMLMultidimDataNode::SetDataNodeAtValue failed, invalid node ID (the node must be in the scene)"); 
    return;
  }
  if (nodeRole==NULL)
  {
    vtkErrorMacro("vtkMRMLMultidimDataNode::SetDataNodeAtValue failed, invalid nodeRole value"); 
    return;
  }
  if (parameterValue==NULL)
  {
    vtkErrorMacro("vtkMRMLMultidimDataNode::SetDataNodeAtValue failed, invalid parameterValue"); 
    return;
  }
  int bundleIndex=GetBundleIndex(parameterValue);
  if (bundleIndex<0)
  {
    // The bundle doesn't exist yet
    MultidimBundleType bundle;
    bundle.ParameterValue=parameterValue;
    this->Bundles.push_back(bundle);
    bundleIndex=this->Bundles.size()-1;
  }
  else 
  {
    // The bundle exists already.
    // 1. the node may have been added already with a different role, delete the old role
    std::string roleNameInBundleAlready=GetDataNodeRoleAtValue(node, parameterValue);
    if (!roleNameInBundleAlready.empty())
    {
      // there is a node already with that role in the bundle
      if (roleNameInBundleAlready.compare(nodeRole)==0)
      {
        // found the same node with the same role name in the bundle, so there is nothing to do
        return;
      }
      // Found the same node with a different role name.
      // Remove the node with the old role, so one node is only listed in a bundle once.
      RemoveDataNodeAtValue(node, parameterValue);
    }      
    // 2. the role may exist and another node may be associated
    // that is not an issue, we will just overwrite the the bundle and the node references
  }
  this->Bundles[bundleIndex].Roles.insert(nodeRole);
  this->SetNodeReferenceID(GetNodeReferenceRoleName(parameterValue,nodeRole).c_str(),node->GetID());
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataNode::RemoveDataNodeAtValue(vtkMRMLNode* node, const char* parameterValue)
{
  if (node==NULL)
  {
    vtkErrorMacro("vtkMRMLMultidimDataNode::RemoveDataNodeAtValue failed, invalid node"); 
    return;
  } 
  if (parameterValue==NULL)
  {
    vtkErrorMacro("vtkMRMLMultidimDataNode::RemoveDataNodeAtValue failed, invalid parameterValue"); 
    return;
  }
 
  std::string nodeRoleInBundle=GetDataNodeRoleAtValue(node, parameterValue);
  if (nodeRoleInBundle.empty())
  {
    vtkWarningMacro("RemoveDataNodeAtValue failed: node is not defined for parameter value "<<parameterValue);
  }

  this->RemoveAllNodeReferenceIDs(GetNodeReferenceRoleName(parameterValue, nodeRoleInBundle).c_str());

  int bundleIndex=GetBundleIndex(parameterValue);
  if (bundleIndex<0)
  {
    vtkWarningMacro("vtkMRMLMultidimDataNode::RemoveDataNodeAtValue: the specified node was found at parameter value "<<parameterValue);
    return;
  }
  this->Bundles[bundleIndex].Roles.erase(nodeRoleInBundle);  
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataNode::RemoveAllDataNodesAtValue(const char* parameterValue)
{
  if (parameterValue==NULL)
  {
    vtkErrorMacro("vtkMRMLMultidimDataNode::RemoveAllDataNodesAtValue failed, invalid parameterValue"); 
    return;
  }
  int bundleIndex=GetBundleIndex(parameterValue);
  if (bundleIndex<0)
  {
    vtkWarningMacro("vtkMRMLMultidimDataNode::RemoveAllDataNodesAtValue: no bundle found with parameter value "<<parameterValue); 
    return;
  }
  // Remove node references
  for (RoleSetType::iterator roleNameIt=this->Bundles[bundleIndex].Roles.begin(); roleNameIt!=this->Bundles[bundleIndex].Roles.end(); ++roleNameIt)
  {
    this->RemoveAllNodeReferenceIDs(GetNodeReferenceRoleName(parameterValue, *roleNameIt).c_str());
  }
  // Remove the bundle
  this->Bundles.erase(this->Bundles.begin()+bundleIndex);
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataNode::UpdateNodeName(vtkMRMLNode* node, const char* parameterValue)
{
  if (node==NULL)
  {
    vtkErrorMacro("vtkMRMLMultidimDataNode::UpdateNodeName failed, invalid node"); 
    return;
  }
  
  std::string dataNodeName=std::string(this->GetName())+"/"+node->GetName()+std::string(" (")
    +SAFE_CHAR_POINTER(this->GetDimensionName())+"="+parameterValue
    +SAFE_CHAR_POINTER(this->GetUnit())+")";
  node->SetName( dataNodeName.c_str() );
}

//----------------------------------------------------------------------------
int vtkMRMLMultidimDataNode::GetBundleIndex(const char* parameterValue)
{
  if (parameterValue==NULL)
  {
    vtkErrorMacro("vtkMRMLMultidimDataNode::GetBundleIndex failed, invalid parameter value"); 
    return -1;
  }
  int numberOfBundles=this->Bundles.size();
  for (int i=0; i<numberOfBundles; i++)
  {
    if (this->Bundles[i].ParameterValue.compare(parameterValue)==0)
    {
      return i;
    }
  }
  return -1;
}

//---------------------------------------------------------------------------
void vtkMRMLMultidimDataNode::GetDataNodesAtValue(vtkCollection* foundNodes, const char* parameterValue)
{
  if (foundNodes==NULL)
  {
    vtkErrorMacro("GetDataNodesAtValue failed, invalid output collection"); 
    return;
  }
  if (parameterValue==NULL)
  {
    vtkErrorMacro("GetDataNodesAtValue failed, invalid parameter value"); 
    return;
  }
  if (this->Scene==NULL)
  {
    vtkErrorMacro("GetDataNodesAtValue failed, invalid scene");
    return;
  }
  foundNodes->RemoveAllItems();
  int bundleIndex=GetBundleIndex(parameterValue);
  if (bundleIndex<0)
  {
    // bundle is not found
    return;
  }
  for (RoleSetType::iterator roleNameIt=this->Bundles[bundleIndex].Roles.begin(); roleNameIt!=this->Bundles[bundleIndex].Roles.end(); ++roleNameIt)
  {
    vtkMRMLNode* node=GetNodeAtValue(parameterValue, *roleNameIt);
    if (node!=NULL)
    {
      foundNodes->AddItem(node);
    }
  }
}

//-----------------------------------------------------------------------------
std::string vtkMRMLMultidimDataNode::GetDataNodeRoleAtValue(vtkMRMLNode* node, const char* parameterValue)
{
  if (parameterValue==NULL)
  {
    vtkErrorMacro("GetNodeRoleAtValue failed, invalid parameter value"); 
    return "";
  }
  if (node==NULL)
  {
    vtkErrorMacro("GetNodeRoleAtValue failed, invalid node");
    return "";
  }
  int bundleIndex=GetBundleIndex(parameterValue);
  if (bundleIndex<0)
  {
    // bundle is not found
    return "";
  }
  for (RoleSetType::iterator roleNameIt=this->Bundles[bundleIndex].Roles.begin(); roleNameIt!=this->Bundles[bundleIndex].Roles.end(); ++roleNameIt)
  {
    vtkMRMLNode* foundNode=GetNodeAtValue(parameterValue, *roleNameIt);
    if (node==foundNode)
    {
      return *roleNameIt;
    }    
  }
  // not in the bundle
  return "";
}

//---------------------------------------------------------------------------
std::string vtkMRMLMultidimDataNode::GetNthParameterValue(int bundleIndex)
{
  if (bundleIndex<0 || bundleIndex>=this->Bundles.size())
  {
    vtkErrorMacro("vtkMRMLMultidimDataNode::GetNthParameterValue failed, invalid bundleIndex value: "<<bundleIndex);
    return "";
  }
  return this->Bundles[bundleIndex].ParameterValue;
}

//-----------------------------------------------------------------------------
int vtkMRMLMultidimDataNode::GetNumberOfBundles()
{
  return this->Bundles.size();
}

//-----------------------------------------------------------------------------
void vtkMRMLMultidimDataNode::UpdateParameterValue(const char* oldParameterValue, const char* newParameterValue)
{
  if (oldParameterValue==NULL)
  {
    vtkErrorMacro("vtkMRMLMultidimDataNode::UpdateParameterValue failed, invalid oldParameterValue"); 
    return;
  }
  if (newParameterValue==NULL)
  {
    vtkErrorMacro("vtkMRMLMultidimDataNode::UpdateParameterValue failed, invalid newParameterValue"); 
    return;
  }
  if (strcmp(oldParameterValue,newParameterValue)==0)
  {
    // no change
    return;
  }
  int bundleIndex=GetBundleIndex(oldParameterValue);
  if (bundleIndex<0)
  {
    vtkErrorMacro("vtkMRMLMultidimDataNode::UpdateParameterValue failed, no bundle found with parameter value "<<oldParameterValue); 
    return;
  }

  // Change node references to the new parameter value
  for (RoleSetType::iterator roleNameIt=this->Bundles[bundleIndex].Roles.begin(); roleNameIt!=this->Bundles[bundleIndex].Roles.end(); ++roleNameIt)
  {
    vtkMRMLNode* node=this->GetNodeReference(GetNodeReferenceRoleName(oldParameterValue, *roleNameIt).c_str());    
    this->RemoveAllNodeReferenceIDs(GetNodeReferenceRoleName(oldParameterValue, *roleNameIt).c_str());    
    this->SetNodeReferenceID(GetNodeReferenceRoleName(newParameterValue, *roleNameIt).c_str(),node->GetID());
  }

  // Update the parameter value
  this->Bundles[bundleIndex].ParameterValue=newParameterValue;
}

//-----------------------------------------------------------------------------
void vtkMRMLMultidimDataNode::AddBundle(const char* parameterValue)
{
  int bundleIndex=GetBundleIndex(parameterValue);
  if (bundleIndex>0)
  {
    vtkWarningMacro("vtkMRMLMultidimDataNode::AddBundle: bundle already exists with parameterValue "<<parameterValue);
    return;
  }
  MultidimBundleType bundle;
  bundle.ParameterValue=parameterValue;
  this->Bundles.push_back(bundle);
}

//-----------------------------------------------------------------------------
std::string vtkMRMLMultidimDataNode::GetNodeReferenceRoleName(const std::string &parameterValue, const std::string &roleInBundle)
{
  return parameterValue+"/"+roleInBundle;
}

//-----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLMultidimDataNode::GetNodeAtValue(const std::string &parameterValue, const std::string &roleInBundle)
{
  std::string nodeReferenceRoleName=GetNodeReferenceRoleName(parameterValue, roleInBundle);
  return this->GetNodeReference(nodeReferenceRoleName.c_str());
}
