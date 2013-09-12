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

// .NAME vtkSlicerMultiDimensionBrowserLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerMultiDimensionBrowserLogic_h
#define __vtkSlicerMultiDimensionBrowserLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerMultiDimensionBrowserModuleLogicExport.h"

class vtkMRMLNode;
class vtkMRMLMultiDimensionBrowserNode;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_MULTIDIMENSIONBROWSER_MODULE_LOGIC_EXPORT vtkSlicerMultiDimensionBrowserLogic :
  public vtkSlicerModuleLogic
{
public:

  static vtkSlicerMultiDimensionBrowserLogic *New();
  vtkTypeMacro(vtkSlicerMultiDimensionBrowserLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkMRMLNode* SetMultiDimensionBrowserRootNode(vtkMRMLNode*);

  void UpdateVirtualOutputNode(vtkMRMLMultiDimensionBrowserNode* browserNode);

protected:
  vtkSlicerMultiDimensionBrowserLogic();
  virtual ~vtkSlicerMultiDimensionBrowserLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
  virtual void ProcessMRMLNodesEvents(vtkObject *caller, unsigned long event, void *callData);

  bool IsDataConnectorNode(vtkMRMLNode*);

private:

  vtkSlicerMultiDimensionBrowserLogic(const vtkSlicerMultiDimensionBrowserLogic&); // Not implemented
  void operator=(const vtkSlicerMultiDimensionBrowserLogic&);               // Not implemented
};

#endif
