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
#include "vtkSlicerMultidimDataLogic.h"

class vtkMRMLMultidimDataNode;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_METAFILEIMPORTER_MODULE_LOGIC_EXPORT vtkSlicerMetafileImporterLogic :
  public vtkSlicerModuleLogic
{
public:

  static vtkSlicerMetafileImporterLogic *New();
  vtkTypeMacro(vtkSlicerMetafileImporterLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);
  void SetMultidimDataLogic(vtkSlicerMultidimDataLogic* multiDimensionLogic);

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

protected:

  /*! Read all the fields in the metaimage file header */
  void ReadTransforms(const std::string &fileName );

  /*! Read pixel data from the metaimage */
  void ReadImages(const std::string& fileName );

  /*! Generate a node name that contains the hierarchy name and parameter value */
  std::string GenerateDataNodeName(const std::string &dataItemName, const std::string& parameterValue);

  /*! Logic for MultidimData hierarchy to manipulate nodes */
  vtkSlicerMultidimDataLogic* MultidimDataLogic;

  /*! Map the frame numbers to timestamps */
  std::map< int, std::string > FrameNumberToParameterValueMap;

  std::string BaseNodeName;

};

#endif
