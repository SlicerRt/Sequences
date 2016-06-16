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
#include <vtkNew.h>

// STD includes
#include <set>
#include <map>

#include "vtkSlicerSequenceBrowserModuleMRMLExport.h"

class vtkCollection;
class vtkMRMLSequenceNode;
class vtkMRMLDisplayNode;
class vtkIntArray;

class VTK_SLICER_SEQUENCEBROWSER_MODULE_MRML_EXPORT vtkMRMLSequenceBrowserNode : public vtkMRMLNode
{
public:
  static vtkMRMLSequenceBrowserNode *New();
  vtkTypeMacro(vtkMRMLSequenceBrowserNode,vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// The type of synchronization. Whether synchronized for playback, recording, etc.
  enum SynchronizationTypes
  {
    Placeholder = 0, // This is ignored, but necessary so Playback is not 0
    Playback,
    Recording,
    NumberOfSynchronizationTypes // this line must be the last one
  };

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
  
  /// Deprecated. Use AddSynchronizedSequenceNodeID instead.
  std::string AddSynchronizedSequenceNode(const char* synchronizedSequenceNodeId);

  /// Adds a node for synchronized browsing. Returns the new virtual output node postfix.
  std::string AddSynchronizedSequenceNodeID(const char* synchronizedSequenceNodeId);

  /// Adds a node for synchronized browsing. Returns the new virtual output node postfix.
  std::string AddSynchronizedSequenceNode(vtkMRMLSequenceNode* synchronizedSequenceNode);

  /// Removes a node from synchronized browsing
  void RemoveSynchronizedSequenceNode(const char* nodeId);

  /// Remove all sequence nodes (including the master sequence node)
  void RemoveAllSequenceNodes();

  /// Returns all synchronized sequence nodes (does not include the master sequence node)
  void GetSynchronizedSequenceNodes(std::vector< vtkMRMLSequenceNode* > &synchronizedDataNodes, bool includeMasterNode=false);
  void GetSynchronizedSequenceNodes(vtkCollection* synchronizedDataNodes, bool includeMasterNode=false);

  /// Returns all synchonized sequences node that have a particular type (does not include the master sequence node by default)
  void GetSynchronizedSequenceNodes(std::vector< vtkMRMLSequenceNode* > &synchronizedDataNodes, SynchronizationTypes syncType, bool includeMasterNode = false);
  void GetSynchronizedSequenceNodes(vtkCollection* synchronizedDataNodes, SynchronizationTypes syncType, bool includeMasterNode = false);

  /// Deprecated. Use IsSynchronizedSequenceNodeID instead.
  bool IsSynchronizedSequenceNode(const char* sequenceNodeId, bool includeMasterNode = false);

  /// Returns true if the node is selected for synchronized browsing
  bool IsSynchronizedSequenceNodeID(const char* sequenceNodeId, bool includeMasterNode = false);
  bool IsSynchronizedSequenceNode(vtkMRMLSequenceNode* sequenceNode, bool includeMasterNode = false);

  /// Returns true if the node has a particular type of synchronization
  bool IsSynchronizedSequenceNodeID(const char* nodeId, SynchronizationTypes syncType, bool includeMasterNode = false);
  bool IsSynchronizedSequenceNode(vtkMRMLSequenceNode* sequenceNode, SynchronizationTypes syncType, bool includeMasterNode = false);

  /// Set whether or not a node has a particular type of synchronization
  void SequenceNodeSynchronizationTypeOn(const char* nodeId, SynchronizationTypes syncType);
  void SequenceNodeSynchronizationTypeOff(const char* nodeId, SynchronizationTypes syncType);

  /// Get/Set automatic playback (automatic continuous changing of selected sequence nodes)
  vtkGetMacro(PlaybackActive, bool);
  vtkSetMacro(PlaybackActive, bool);
  vtkBooleanMacro(PlaybackActive, bool);

  /// Get/Set playback rate in fps (frames per second)
  vtkGetMacro(PlaybackRateFps, double);
  vtkSetMacro(PlaybackRateFps, double);

  /// Skipping items if necessary to reach requested playback rate. Enabled by default.
  vtkGetMacro(PlaybackItemSkippingEnabled, bool);
  vtkSetMacro(PlaybackItemSkippingEnabled, bool);
  vtkBooleanMacro(PlaybackItemSkippingEnabled, bool);

  /// Get/Set playback looping (restart from the first sequence node when reached the last one)
  vtkGetMacro(PlaybackLooped, bool);
  vtkSetMacro(PlaybackLooped, bool);
  vtkBooleanMacro(PlaybackLooped, bool);
  
  /// Get/Set selected bundle index
  vtkGetMacro(SelectedItemNumber, int);
  vtkSetMacro(SelectedItemNumber, int);

  /// Get/set recording of virtual output nodes
  vtkGetMacro(RecordingActive, bool);
  void SetRecordingActive(bool recording);
  vtkBooleanMacro(RecordingActive, bool);

  /// Get/set whether to only record when the master node is modified (or emits an observed event)
  vtkGetMacro(RecordMasterOnly, bool);
  vtkSetMacro(RecordMasterOnly, bool);
  vtkBooleanMacro(RecordMasterOnly, bool);

  /// Selects the next sequence item for display, returns current selected item number
  int SelectNextItem(int selectionIncrement=1);

  /// Selects first sequence item for display, returns current selected item number
  int SelectFirstItem();

  /// Selects last sequence item for display, returns current selected item number
  int SelectLastItem();

  /// Adds virtual output nodes from another scene (typically from the main scene). The data node is not copied but a clean node is instantiated of the same node type.
  vtkMRMLNode* AddVirtualOutputNodes(vtkMRMLNode* dataNode, std::vector< vtkMRMLDisplayNode* > &displayNodes, vtkMRMLSequenceNode* sequenceNode, bool copy=true);

  /// Get virtual output node corresponding to a sequence node.
  vtkMRMLNode* GetVirtualOutputDataNode(vtkMRMLSequenceNode* sequenceNode);

  /// Get sequence node corresponding to a virtual output data node.
  vtkMRMLSequenceNode* GetSequenceNode(vtkMRMLNode* virtualOutputDataNode);

  void GetVirtualOutputDisplayNodes(vtkMRMLSequenceNode* sequenceNode, std::vector< vtkMRMLDisplayNode* > &displayNodes);
  void GetVirtualOutputDisplayNodes(vtkMRMLSequenceNode* sequenceNode, vtkCollection* displayNodes);

  void GetAllVirtualOutputDataNodes(std::vector< vtkMRMLNode* > &nodes);
  void GetAllVirtualOutputDataNodes(vtkCollection* nodes);

  /// Deprecated. Use IsVirtualOutputDataNodeID instead.
  bool IsVirtualOutputDataNode(const char* nodeId);
  
  /// Returns true if the nodeId belongs to a virtual output data node managed by this browser node.
  bool IsVirtualOutputDataNodeID(const char* nodeId);

  void RemoveVirtualOutputDataNode(const std::string& postfix);

  void RemoveVirtualOutputDisplayNodes(const std::string& postfix);

  void RemoveAllVirtualOutputNodes();

  /// Helper function for performance optimization of volume browsing
  /// It disables auto WW/WL computation in scalar display nodes, as WW/WL would be recomputed on each volume change,
  /// this significantly slowing down browsing.
  /// The method has no effect if there is no output display node or it is not scalar volume display node type.
  void ScalarVolumeAutoWindowLevelOff();

  /// Process MRML node events for recording of the virtual output nodes
  void ProcessMRMLEvents( vtkObject *caller, unsigned long event, void *callData );

protected:
  vtkMRMLSequenceBrowserNode();
  ~vtkMRMLSequenceBrowserNode();
  vtkMRMLSequenceBrowserNode(const vtkMRMLSequenceBrowserNode&);
  void operator=(const vtkMRMLSequenceBrowserNode&);

  /// Earlier (before November 2015) sequenceNodeRef role name was rootNodeRef.
  /// Change the role name to the new one for compatibility with old data.
  void FixSequenceNodeReferenceRoleName();

  std::string GenerateVirtualNodePostfix();
  std::string GetVirtualNodePostfixFromSequence(vtkMRMLSequenceNode* sequenceNode);

protected:
  bool PlaybackActive;
  double PlaybackRateFps;
  bool PlaybackItemSkippingEnabled;
  bool PlaybackLooped;
  int SelectedItemNumber;
  
  bool RecordingActive;
  double InitialTime;
  bool RecordMasterOnly;

  vtkNew< vtkIntArray > RecordingEvents;

  // Unique postfixes for storing references to sequence nodes, virtual data nodes, and virtual display nodes
  // For example, a sequence node reference role name is SEDQUENCE_NODE_REFERENCE_ROLE_BASE+virtualNodePostfix
  std::vector< std::string > VirtualNodePostfixes;

  // Counter that is used for generating the unique (only for this class) virtual node postfix strings
  int LastPostfixIndex;
};

#endif
