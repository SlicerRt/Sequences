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

#ifndef __vtkMRMLMultidimDataBrowserNode_h
#define __vtkMRMLMultidimDataBrowserNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

// STD includes
#include <set>
#include <map>

#include "vtkSlicerMultidimDataBrowserModuleLogicExport.h"

class vtkMRMLMultidimDataNode;
class vtkMRMLDisplayNode;

class VTK_SLICER_MULTIDIMDATABROWSER_MODULE_LOGIC_EXPORT vtkMRMLMultidimDataBrowserNode : public vtkMRMLNode
{
public:
  static vtkMRMLMultidimDataBrowserNode *New();
  vtkTypeMacro(vtkMRMLMultidimDataBrowserNode,vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Create instance of a GAD node. 
  virtual vtkMRMLNode* CreateNodeInstance();

  /// Set node attributes from name/value pairs 
  virtual void ReadXMLAttributes( const char** atts);

  /// Write this node's information to a MRML file in XML format. 
  virtual void WriteXML(ostream& of, int indent);

  /// Copy the node's attributes to this object 
  virtual void Copy(vtkMRMLNode *node);

  /// Get unique node XML tag name (like Volume, Model) 
  virtual const char* GetNodeTagName() {return "MultidimDataBrowser";};

  /// Set the multidimensional data node root
  void SetAndObserveRootNodeID(const char *rootNodeID);
  /// Get the multidimensional data node root
  vtkMRMLMultidimDataNode* GetRootNode();
  
  /// Get/Set automatic playback (automatic continuous changing of selected sequence nodes)
  vtkGetMacro(PlaybackActive, bool);
  vtkSetMacro(PlaybackActive, bool);
  vtkBooleanMacro(PlaybackActive, bool);

  /// Get/Set playback rate in fps (frames per second)
  vtkGetMacro(PlaybackRateFps, double);
  vtkSetMacro(PlaybackRateFps, double);

  /// Get/Set playback looping (restart from the first sequence node when reached the last one)
  vtkGetMacro(PlaybackLooped, bool);
  vtkSetMacro(PlaybackLooped, bool);
  vtkBooleanMacro(PlaybackLooped, bool);
  
  /// Get/Set selected bundle index
  vtkGetMacro(SelectedBundleIndex, int);
  vtkSetMacro(SelectedBundleIndex, int);

  void RemoveAllVirtualOutputNodes();

  vtkMRMLNode* GetVirtualOutputDataNode(vtkMRMLMultidimDataNode* rootNode);

  void GetVirtualOutputDisplayNodes(vtkMRMLMultidimDataNode* rootNode, std::vector< vtkMRMLDisplayNode* > &displayNodes);

  void AddVirtualOutputNodes(vtkMRMLNode* dataNode, std::vector< vtkMRMLDisplayNode* > &displayNodes, vtkMRMLMultidimDataNode* rootNode);

  /// Removed output data and display nodes
  void RemoveVirtualOutputNodes(vtkMRMLMultidimDataNode* rootNode);

  void GetAllVirtualOutputDataNodes(std::vector< vtkMRMLNode* > &nodes);

  /// Returns true if the node is selected for synchronized browsing
  bool IsSynchronizedRootNode(const char* nodeId);
  
  /// Adds a node for synchronized browsing. Returns the new virtual output node postfix.
  std::string AddSynchronizedRootNode(const char* synchronizedRootNodeId);

  /// Removes a node from synchronized browsing
  void RemoveSynchronizedRootNode(const char* nodeId);

  /// Returns all synchronized root nodes (does not include the master root node)
  void GetSynchronizedRootNodes(std::vector< vtkMRMLMultidimDataNode* > &synchronizedDataNodes, bool includeMasterNode=false);

  /// Returns all data nodes at a given index of the master root node (includes data node of the master root node)
  void GetNthSynchronizedDataNodes(std::vector< vtkMRMLNode* > &dataNodes, int selectedBundleIndex);

protected:
  vtkMRMLMultidimDataBrowserNode();
  ~vtkMRMLMultidimDataBrowserNode();
  vtkMRMLMultidimDataBrowserNode(const vtkMRMLMultidimDataBrowserNode&);
  void operator=(const vtkMRMLMultidimDataBrowserNode&);

  std::string GenerateVirtualNodePostfix();
  std::string GetVirtualNodePostfixFromRoot(vtkMRMLMultidimDataNode* rootNode);

protected:
  bool PlaybackActive;
  double PlaybackRateFps;
  bool PlaybackLooped;
  int SelectedBundleIndex;

  // Unique postfixes for storing references to root nodes, virtual data nodes, and virtual display nodes
  // For example, a root node reference role name is ROOT_NODE_REFERENCE_ROLE_BASE+virtualNodePostfix
  std::vector< std::string > VirtualNodePostfixes;

  // Counter that is used for generating the unique (only for this class) virtual node postfix strings
  int LastPostfixIndex;
};

#endif
