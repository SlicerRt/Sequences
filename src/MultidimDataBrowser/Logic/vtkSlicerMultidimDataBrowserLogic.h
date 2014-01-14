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

// .NAME vtkSlicerMultidimDataBrowserLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerMultidimDataBrowserLogic_h
#define __vtkSlicerMultidimDataBrowserLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerMultidimDataBrowserModuleLogicExport.h"

class vtkMRMLNode;
class vtkMRMLMultidimDataBrowserNode;
class vtkMRMLMultidimDataNode;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_MULTIDIMDATABROWSER_MODULE_LOGIC_EXPORT vtkSlicerMultidimDataBrowserLogic :
  public vtkSlicerModuleLogic
{
public:

  static vtkSlicerMultidimDataBrowserLogic *New();
  vtkTypeMacro(vtkSlicerMultidimDataBrowserLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Updates the contents of all the virtual output nodes (all the nodes copied from the selected bundle to the scene)
  void UpdateVirtualOutputNodes(vtkMRMLMultidimDataBrowserNode* browserNode);

  void GetCompatibleNodesFromScene(vtkCollection* compatibleNodes, vtkMRMLMultidimDataNode* multidimDataRootNode);

protected:
  vtkSlicerMultidimDataBrowserLogic();
  virtual ~vtkSlicerMultidimDataBrowserLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void ProcessMRMLNodesEvents(vtkObject *caller, unsigned long event, void *callData);

  bool IsDataConnectorNode(vtkMRMLNode*);

  void ShallowCopy(vtkMRMLNode* target, vtkMRMLNode* source);

private:

  bool UpdateVirtualOutputNodesInProgress;

  vtkSlicerMultidimDataBrowserLogic(const vtkSlicerMultidimDataBrowserLogic&); // Not implemented
  void operator=(const vtkSlicerMultidimDataBrowserLogic&);               // Not implemented
};

#endif
