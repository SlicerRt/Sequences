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

// .NAME vtkSlicerSequencesLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerSequencesLogic_h
#define __vtkSlicerSequencesLogic_h

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MRML includes

// STD includes
#include <cstdlib>

#include "vtkSlicerSequencesModuleLogicExport.h"

class vtkMRMLNode;
class vtkMRMLSequenceNode;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_SEQUENCES_MODULE_LOGIC_EXPORT vtkSlicerSequencesLogic :
  public vtkSlicerModuleLogic
{
public:

  static vtkSlicerSequencesLogic *New();
  vtkTypeMacro(vtkSlicerSequencesLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  ///
  /// Add into the scene a new mrml sequence node and
  /// read its data from a specified file
  /// A storage node is also added into the scene
  vtkMRMLSequenceNode* AddSequence(const char* filename);

protected:
  vtkSlicerSequencesLogic();
  virtual ~vtkSlicerSequencesLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene) VTK_OVERRIDE;
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes() VTK_OVERRIDE;
  virtual void UpdateFromMRMLScene() VTK_OVERRIDE;
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node) VTK_OVERRIDE;
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node) VTK_OVERRIDE;

private:

  vtkSlicerSequencesLogic(const vtkSlicerSequencesLogic&); // Not implemented
  void operator=(const vtkSlicerSequencesLogic&);               // Not implemented
};

#endif
