/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// .NAME vtkSlicerSequenceBrowserLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerSequenceBrowserLogic_h
#define __vtkSlicerSequenceBrowserLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerSequenceBrowserModuleLogicExport.h"
#include "vtkMRMLSequenceBrowserNode.h" // Forward class declaration does not work with enum

class vtkMRMLNode;
class vtkMRMLSequenceNode;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_SEQUENCEBROWSER_MODULE_LOGIC_EXPORT vtkSlicerSequenceBrowserLogic :
  public vtkSlicerModuleLogic
{
public:

  static vtkSlicerSequenceBrowserLogic *New();
  vtkTypeMacro(vtkSlicerSequenceBrowserLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Refreshes the output of all the active browser nodes. Called regularly by a timer.
  void UpdateAllProxyNodes();

  /// Updates the contents of all the proxy nodes (all the nodes copied from the master and synchronized sequences to the scene)
  void UpdateProxyNodesFromSequences(vtkMRMLSequenceBrowserNode* browserNode);

  /// Updates the sequence from a changed proxy node (if saving of state changes is allowed)
  void UpdateSequencesFromProxyNodes(vtkMRMLSequenceBrowserNode* browserNode, vtkMRMLNode* proxyNode);

  /// Deprecated method!
  void UpdateVirtualOutputNodes(vtkMRMLSequenceBrowserNode* browserNode)
  {
    static bool warningLogged = false;
    if (!warningLogged)
    {
      vtkWarningMacro("vtkSlicerSequenceBrowserLogic::UpdateVirtualOutputNodes is deprecated, use vtkSlicerSequenceBrowserLogic::UpdateProxyNodes method instead");
      warningLogged = true;
    }
    this->UpdateProxyNodesFromSequences(browserNode);
  }

  /// Add a synchronized sequence node and virtual output node pair to the browser node for playback/recording
  /// \param sequenceNode Sequence node to add. If NULL, then a new node is created.
  /// \param proxyNode Proxy node to use to represent selected item in the scene. May be NULL.
  /// Returns the added/created sequence node, NULL on error.
  vtkMRMLSequenceNode* AddSynchronizedNode(vtkMRMLNode* sequenceNode, vtkMRMLNode* proxyNode, vtkMRMLNode* browserNode);

  void GetCompatibleNodesFromScene(vtkCollection* compatibleNodes, vtkMRMLSequenceNode* sequenceNode);

  static bool IsNodeCompatibleForBrowsing(vtkMRMLSequenceNode* masterNode, vtkMRMLSequenceNode* testedNode);

  /// Get collection of browser nodes that use a specific sequence node.
  void GetBrowserNodesForSequenceNode(vtkMRMLSequenceNode* sequenceNode, vtkCollection* foundBrowserNodes);

  /// Get first browser node that use a specific sequence node. This is a convenience method for
  /// cases when it is known that a sequence is only used in one browser node. In general case,
  /// use GetBrowserNodesForSequenceNode instead.
  vtkMRMLSequenceBrowserNode* GetFirstBrowserNodeForSequenceNode(vtkMRMLSequenceNode* sequenceNode);

protected:
  vtkSlicerSequenceBrowserLogic();
  virtual ~vtkSlicerSequenceBrowserLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene) override;
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes() override;
  virtual void UpdateFromMRMLScene() override;
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node) override;
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node) override;
  virtual void ProcessMRMLNodesEvents(vtkObject *caller, unsigned long event, void *callData) override;

  bool IsDataConnectorNode(vtkMRMLNode*);

  // Time of the last update of each browser node (in universal time)
  std::map< vtkMRMLSequenceBrowserNode*, double > LastSequenceBrowserUpdateTimeSec;

private:

  bool UpdateProxyNodesFromSequencesInProgress;
  bool UpdateSequencesFromProxyNodesInProgress;

  vtkSlicerSequenceBrowserLogic(const vtkSlicerSequenceBrowserLogic&); // Not implemented
  void operator=(const vtkSlicerSequenceBrowserLogic&);               // Not implemented
};

#endif
