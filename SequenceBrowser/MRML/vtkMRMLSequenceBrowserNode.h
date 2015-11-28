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

#ifndef __vtkMRMLSequenceBrowserNode_h
#define __vtkMRMLSequenceBrowserNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

// STD includes
#include <set>
#include <map>

#include "vtkSlicerSequenceBrowserModuleMRMLExport.h"

class vtkMRMLSequenceNode;
class vtkMRMLDisplayNode;

class VTK_SLICER_SEQUENCEBROWSER_MODULE_MRML_EXPORT vtkMRMLSequenceBrowserNode : public vtkMRMLNode
{
public:
  static vtkMRMLSequenceBrowserNode *New();
  vtkTypeMacro(vtkMRMLSequenceBrowserNode,vtkMRMLNode);
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
  virtual const char* GetNodeTagName() {return "SequenceBrowser";};

  /// Set the sequence data node
  void SetAndObserveMasterSequenceNodeID(const char *sequenceNodeID);
  /// Get the sequence data node
  vtkMRMLSequenceNode* GetMasterSequenceNode();
  
  /// Adds a node for synchronized browsing. Returns the new virtual output node postfix.
  std::string AddSynchronizedSequenceNode(const char* synchronizedSequenceNodeId);

  /// Removes a node from synchronized browsing
  void RemoveSynchronizedSequenceNode(const char* nodeId);

  /// Remove all sequence nodes (including the master sequence node)
  void RemoveAllSequenceNodes();

  /// Returns all synchronized sequence nodes (does not include the master sequence node)
  void GetSynchronizedSequenceNodes(std::vector< vtkMRMLSequenceNode* > &synchronizedDataNodes, bool includeMasterNode=false);

  /// Returns true if the node is selected for synchronized browsing
  bool IsSynchronizedSequenceNode(const char* nodeId);

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
  vtkGetMacro(SelectedItemNumber, int);
  vtkSetMacro(SelectedItemNumber, int);

  /// Selects the next sequence item for display, returns current selected item number
  int SelectNextItem(int selectionIncrement=1);

  /// Selects first sequence item for display, returns current selected item number
  int SelectFirstItem();

  /// Selects last sequence item for display, returns current selected item number
  int SelectLastItem();

  /// Adds virtual output nodes from another scene (typically from the main scene). The data node is not copied but a clean node is instantiated of the same node type.
  vtkMRMLNode* AddVirtualOutputNodes(vtkMRMLNode* dataNode, std::vector< vtkMRMLDisplayNode* > &displayNodes, vtkMRMLSequenceNode* sequenceNode);

  vtkMRMLNode* GetVirtualOutputDataNode(vtkMRMLSequenceNode* sequenceNode);

  void GetVirtualOutputDisplayNodes(vtkMRMLSequenceNode* sequenceNode, std::vector< vtkMRMLDisplayNode* > &displayNodes);

  void GetAllVirtualOutputDataNodes(std::vector< vtkMRMLNode* > &nodes);

  void RemoveVirtualOutputDataNode(const std::string& postfix);

  void RemoveVirtualOutputDisplayNodes(const std::string& postfix);

  void RemoveAllVirtualOutputNodes();

  /// Helper function for performance optimization of volume browsing
  /// It disables auto WW/WL computation in scalar display nodes, as WW/WL would be recomputed on each volume change,
  /// this significantly slowing down browsing.
  /// The method has no effect if there is no output display node or it is not scalar volume display node type.
  void ScalarVolumeAutoWindowLevelOff();

protected:
  vtkMRMLSequenceBrowserNode();
  ~vtkMRMLSequenceBrowserNode();
  vtkMRMLSequenceBrowserNode(const vtkMRMLSequenceBrowserNode&);
  void operator=(const vtkMRMLSequenceBrowserNode&);

  std::string GenerateVirtualNodePostfix();
  std::string GetVirtualNodePostfixFromSequence(vtkMRMLSequenceNode* sequenceNode);

protected:
  bool PlaybackActive;
  double PlaybackRateFps;
  bool PlaybackLooped;
  int SelectedItemNumber;

  // Unique postfixes for storing references to sequence nodes, virtual data nodes, and virtual display nodes
  // For example, a sequence node reference role name is SEDQUENCE_NODE_REFERENCE_ROLE_BASE+virtualNodePostfix
  std::vector< std::string > VirtualNodePostfixes;

  // Counter that is used for generating the unique (only for this class) virtual node postfix strings
  int LastPostfixIndex;
};

#endif
