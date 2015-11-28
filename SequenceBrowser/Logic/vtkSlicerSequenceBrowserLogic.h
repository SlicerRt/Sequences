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

class vtkMRMLNode;
class vtkMRMLSequenceBrowserNode;
class vtkMRMLSequenceNode;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_SEQUENCEBROWSER_MODULE_LOGIC_EXPORT vtkSlicerSequenceBrowserLogic :
  public vtkSlicerModuleLogic
{
public:

  static vtkSlicerSequenceBrowserLogic *New();
  vtkTypeMacro(vtkSlicerSequenceBrowserLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Refreshes the output of all the active browser nodes. Called regularly by a timer.
  void UpdateAllVirtualOutputNodes();

  /// Updates the contents of all the virtual output nodes (all the nodes copied from the master and synchronized sequences to the scene)
  void UpdateVirtualOutputNodes(vtkMRMLSequenceBrowserNode* browserNode);

  void GetCompatibleNodesFromScene(vtkCollection* compatibleNodes, vtkMRMLSequenceNode* sequenceNode);

protected:
  vtkSlicerSequenceBrowserLogic();
  virtual ~vtkSlicerSequenceBrowserLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void ProcessMRMLNodesEvents(vtkObject *caller, unsigned long event, void *callData);

  bool IsDataConnectorNode(vtkMRMLNode*);

  void ShallowCopy(vtkMRMLNode* target, vtkMRMLNode* source);

  // Time of the last update of each browser node (in universal time)
  std::map< vtkMRMLSequenceBrowserNode*, double > LastSequenceBrowserUpdateTimeSec;

private:

  bool UpdateVirtualOutputNodesInProgress;

  vtkSlicerSequenceBrowserLogic(const vtkSlicerSequenceBrowserLogic&); // Not implemented
  void operator=(const vtkSlicerSequenceBrowserLogic&);               // Not implemented
};

#endif
