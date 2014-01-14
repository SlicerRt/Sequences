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

class vtkMRMLDisplayNode;

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
  /// If 'node' is in the scene already then a copy will be added to the sequence
  void SetDataNodeAtValue(vtkMRMLNode* node, const char* parameterValue);

  void RemoveDataNodeAtValue(const char* parameterValue);

  void RemoveAllDataNodes();

  /// Get the node corresponding to the specified parameter value
  vtkMRMLNode* GetDataNodeAtValue(const char* parameterValue);

  /// Get the all the display nodes corresponding to the specified parameter value
  void GetDisplayNodesAtValue(std::vector< vtkMRMLDisplayNode* > &dataNodes, const char* parameterValue);

  /// Get the data node corresponding to the n-th parameter value
  vtkMRMLNode* GetNthDataNode(int bundleIndex);

  std::string GetNthParameterValue(int bundleIndex);

  void UpdateParameterValue(const char* oldParameterValue, const char* newParameterValue);

  int GetNumberOfDataNodes();

  /// Return the class name of the data nodes. If there are no data nodes yet then it returns empty string.
  std::string GetDataNodeClassName();

public:

protected:
  vtkMRMLMultidimDataNode();
  ~vtkMRMLMultidimDataNode();
  vtkMRMLMultidimDataNode(const vtkMRMLMultidimDataNode&);
  void operator=(const vtkMRMLMultidimDataNode&);

  int GetSequenceItemIndex(const char* parameterValue);

  struct SequenceItemType
  {
    std::string ParameterValue;
    vtkMRMLNode* DataNode;
  };

protected:

  /// Describes a dimension of the multidimensional cube
  char* DimensionName;
  char* Unit;  

  /// Need to store the nodes in the scene, because for reading/writing nodes
  /// we need MRML storage nodes, which only work if they refer to a data node in the same scene
  vtkMRMLScene* SequenceScene;

  /// List of data items (the scene may contain some more nodes, such as storage nodes)
  std::deque< SequenceItemType > SequenceItems;

};

#endif
