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

// .NAME vtkSlicerMultiDimensionLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerMultiDimensionLogic_h
#define __vtkSlicerMultiDimensionLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerMultiDimensionModuleLogicExport.h"

class vtkMRMLNode;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_MULTIDIMENSION_MODULE_LOGIC_EXPORT vtkSlicerMultiDimensionLogic :
  public vtkSlicerModuleLogic
{
public:

  static vtkSlicerMultiDimensionLogic *New();
  vtkTypeMacro(vtkSlicerMultiDimensionLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkMRMLNode* CreateMultiDimensionRootNode();
  vtkMRMLNode* SetMultiDimensionRootNode(vtkMRMLNode*);
  void DeleteMultiDimensionRootNode(vtkMRMLNode*);
  void AddChildNodeAtTimePoint(vtkMRMLNode*, vtkMRMLNode*, char*);
  void RemoveChildNodeAtTimePoint(vtkMRMLNode*, char*);
  vtkCollection* GetChildNodesAtTimePoint(vtkMRMLNode*, char*);

protected:
  vtkSlicerMultiDimensionLogic();
  virtual ~vtkSlicerMultiDimensionLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);

  bool IsDataConnectorNode(vtkMRMLNode*);
  vtkMRMLNode* FindChildNodeAtTimePoint(vtkMRMLNode*, char*);
private:

  vtkSlicerMultiDimensionLogic(const vtkSlicerMultiDimensionLogic&); // Not implemented
  void operator=(const vtkSlicerMultiDimensionLogic&);               // Not implemented
};

#endif
