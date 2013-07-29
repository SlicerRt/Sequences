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

// .NAME vtkSlicerMetafileImporterLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerMetafileImporterLogic_h
#define __vtkSlicerMetafileImporterLogic_h

// STD includes
#include <cstdlib>

// VTK includes
#include "vtkMatrix4x4.h"
#include "vtkMetaImageReader.h"

// ITK includes

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MetfileImporter includes
#include "vtkSlicerMetafileImporterModuleLogicExport.h"
#include "vtkSlicerMultiDimensionLogic.h"

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_METAFILEIMPORTER_MODULE_LOGIC_EXPORT vtkSlicerMetafileImporterLogic :
  public vtkSlicerModuleLogic
{
public:

  static vtkSlicerMetafileImporterLogic *New();
  vtkTypeMacro(vtkSlicerMetafileImporterLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSlicerMetafileImporterLogic();
  virtual ~vtkSlicerMetafileImporterLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
private:

  vtkSlicerMetafileImporterLogic(const vtkSlicerMetafileImporterLogic&); // Not implemented
  void operator=(const vtkSlicerMetafileImporterLogic&);               // Not implemented

public:

  /*! Read file contents into the object */
  void Read( std::string fileName );

  /*! Logic for MultiDimension hierarchy to manipulate nodes */
  vtkSlicerMultiDimensionLogic* MultiDimensionLogic;

  /*! The root node for the multi-dimension hierarchy */
  vtkMRMLNode* rootNode;

  /*! Map the frame numbers to timestamps */
  std::map< std::string, std::string > frameToTimeMap;


protected:

  /*! Read all the fields in the metaimage file header */
  void ReadTransforms( std::string fileName );

  /*! Read pixel data from the metaimage */
  void ReadImages( std::string fileName );
};

#endif
