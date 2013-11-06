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

#ifndef __vtkMRMLMultidimDataNode_h
#define __vtkMRMLMultidimDataNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

// std includes
#include <deque>
#include <set>

#include "vtkSlicerMultidimDataModuleLogicExport.h"

class vtkCollection;

class VTK_SLICER_MULTIDIMDATA_MODULE_LOGIC_EXPORT vtkMRMLMultidimDataNode : public vtkMRMLNode
{
public:
  static vtkMRMLMultidimDataNode *New();
  vtkTypeMacro(vtkMRMLMultidimDataNode,vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Create instance of a multidimensional data node. 
  virtual vtkMRMLNode* CreateNodeInstance();

  /// Set node attributes from name/value pairs 
  virtual void ReadXMLAttributes( const char** atts);

  /// Write this node's information to a MRML file in XML format. 
  virtual void WriteXML(ostream& of, int indent);

  /// Copy the node's attributes to this object 
  virtual void Copy(vtkMRMLNode *node);

  /// Get unique node XML tag name (like Volume, Model) 
  virtual const char* GetNodeTagName() {return "MultidimData";};

  /// Set dimension name (e.g., time)
  vtkSetStringMacro(DimensionName);
  /// Get dimension name (e.g., time)
  vtkGetStringMacro(DimensionName);

  /// Set unit for the parameter values (e.g., sec)
  vtkSetStringMacro(Unit);
  /// Get unit for the parameter values (e.g., sec)
  vtkGetStringMacro(Unit);

  /// Add a data node to this multidimensional data node.
  /// The data node must be added to the scene before this method is called.
  void SetDataNodeAtValue(vtkMRMLNode* node, const char* nodeRole, const char* parameterValue);

  void RemoveDataNodeAtValue(vtkMRMLNode* node, const char* parameterValue);
  
  void RemoveAllDataNodesAtValue(const char* parameterValue);

  /// Update data node name (from "Image" it generates "Sequence1/Image (time=23sec)")
  void UpdateNodeName(vtkMRMLNode* node, const char* parameterValue);

  /// Get the list of nodes available for the specified parameter value
  void GetDataNodesAtValue(vtkCollection* foundNodes, const char* parameterValue);

  /// Get the role name of the specified node at the chosen parameterValue
  std::string GetDataNodeRoleAtValue(vtkMRMLNode* node, const char* parameterValue);

  std::string GetNthParameterValue(int bundleIndex);

  void UpdateParameterValue(const char* oldParameterValue, const char* newParameterValue);

  void AddBundle(const char* parameterValue);

  int GetNumberOfBundles();

public:

protected:
  vtkMRMLMultidimDataNode();
  ~vtkMRMLMultidimDataNode();
  vtkMRMLMultidimDataNode(const vtkMRMLMultidimDataNode&);
  void operator=(const vtkMRMLMultidimDataNode&);

  int GetBundleIndex(const char* parameterValue);

  std::string GetNodeReferenceRoleName(const std::string &parameterValue, const std::string &roleInBundle);

  vtkMRMLNode* GetNodeAtValue(const std::string &parameterValue, const std::string &roleInBundle);

protected:

  /// Describes a dimension of the multidimensional cube
  char* DimensionName;
  char* Unit;
  
  typedef std::set< std::string > RoleSetType;
  struct MultidimBundleType
  {
    /// Parameter value for all nodes in this item
    std::string ParameterValue;
    /// Roles that are defined for this bundle
    /// The actual referenced node is stored as MRML NodeReference (it takes care of changing node IDs, node deletions, etc).
    RoleSetType Roles;
  };

  std::deque< MultidimBundleType > Bundles;

};

#endif
