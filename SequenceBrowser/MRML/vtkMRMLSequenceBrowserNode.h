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

  /// Deprecated. Use IsSynchronizedSequenceNodeID instead.
  bool IsSynchronizedSequenceNode(const char* sequenceNodeId, bool includeMasterNode = false);

  /// Returns true if the node is selected for synchronized browsing
  bool IsSynchronizedSequenceNodeID(const char* sequenceNodeId, bool includeMasterNode = false);
  bool IsSynchronizedSequenceNode(vtkMRMLSequenceNode* sequenceNode, bool includeMasterNode = false);

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

  /// Get/set recording of proxy nodes
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

  /// Adds proxy nodes from another scene (typically from the main scene). The data node is optionally copied.
  vtkMRMLNode* AddProxyNode(vtkMRMLNode* sourceProxyNode, std::vector< vtkMRMLDisplayNode* > &displayNodes, vtkMRMLSequenceNode* sequenceNode, bool copy=true);
  vtkMRMLNode* AddProxyNode(vtkMRMLNode* sourceProxyNode, vtkCollection* displayNodes, vtkMRMLSequenceNode* sequenceNode, bool copy=true);

  /// Get proxy corresponding to a sequence node.
  vtkMRMLNode* GetProxyNode(vtkMRMLSequenceNode* sequenceNode);

  /// Get sequence node corresponding to a proxy node.
  vtkMRMLSequenceNode* GetSequenceNode(vtkMRMLNode* proxyNode);

  void GetProxyDisplayNodes(vtkMRMLSequenceNode* sequenceNode, std::vector< vtkMRMLDisplayNode* > &displayNodes);
  void GetProxyDisplayNodes(vtkMRMLSequenceNode* sequenceNode, vtkCollection* displayNodes);

  void GetAllProxyNodes(std::vector< vtkMRMLNode* > &nodes);
  void GetAllProxyNodes(vtkCollection* nodes);

  /// Deprecated. Use IsProxyNodeID instead.
  bool IsProxyNode(const char* nodeId);
  
  /// Returns true if the nodeId belongs to a proxy node managed by this browser node.
  bool IsProxyNodeID(const char* nodeId);

  // TODO: Should these methods be protected? Probably the "world" shouldn't need to know about the postfixes.
  void RemoveProxyNode(const std::string& postfix);

  void RemoveProxyDisplayNodes(const std::string& postfix);

  void RemoveAllProxyNodes();

  /// Get the synchrnization properties for the given sequence/proxy/displays tuple
  bool GetRecording(vtkMRMLSequenceNode* sequenceNode);
  bool GetPlayback(vtkMRMLSequenceNode* sequenceNode);
  bool GetOverwriteProxyName(vtkMRMLSequenceNode* sequenceNode);
  bool GetSaveChanges(vtkMRMLSequenceNode* sequenceNode);

  /// Set the synchrnization properties for the given sequence/proxy/displays tuple
  void SetRecording(vtkMRMLSequenceNode* sequenceNode, bool recording);
  void SetPlayback(vtkMRMLSequenceNode* sequenceNode, bool playback);
  void SetOverwriteProxyName(vtkMRMLSequenceNode* sequenceNode, bool overwrite);
  void SetSaveChanges(vtkMRMLSequenceNode* sequenceNode, bool save);

  /// Helper function for performance optimization of volume browsing
  /// It disables auto WW/WL computation in scalar display nodes, as WW/WL would be recomputed on each volume change,
  /// this significantly slowing down browsing.
  /// The method has no effect if there is no output display node or it is not scalar volume display node type.
  void ScalarVolumeAutoWindowLevelOff();

  /// Process MRML node events for recording of the proxy nodes
  void ProcessMRMLEvents( vtkObject *caller, unsigned long event, void *callData );

protected:
  vtkMRMLSequenceBrowserNode();
  ~vtkMRMLSequenceBrowserNode();
  vtkMRMLSequenceBrowserNode(const vtkMRMLSequenceBrowserNode&);
  void operator=(const vtkMRMLSequenceBrowserNode&);

  /// Earlier (before November 2015) sequenceNodeRef role name was rootNodeRef.
  /// Change the role name to the new one for compatibility with old data.
  void FixSequenceNodeReferenceRoleName();

  /// Called whenever a new node reference is added
  virtual void OnNodeReferenceAdded(vtkMRMLNodeReference* nodeReference);

  std::string GenerateSynchronizationPostfix();
  std::string GetSynchronizationPostfixFromSequence(vtkMRMLSequenceNode* sequenceNode);

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

  // Unique postfixes for storing references to sequence nodes, proxy nodes, proxy display nodes, and properties
  // For example, a sequence node reference role name is SEQUENCE_NODE_REFERENCE_ROLE_BASE+synchronizationPostfix
  std::vector< std::string > SynchronizationPostfixes;

  // Counter that is used for generating the unique (only for this class) proxy node postfix strings
  int LastPostfixIndex;

private:
  struct SynchronizationProperties;
  std::map< std::string, SynchronizationProperties* > SynchronizationPropertiesMap;
};

#endif
