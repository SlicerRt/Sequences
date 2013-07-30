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

// Qt includes
#include <QtPlugin>

// MetafileImporter Logic includes
#include "vtkSlicerMetafileImporterLogic.h"
#include "vtkSlicerMultiDimensionLogic.h"

// MetafileImporter includes
#include "qSlicerMetafileImporterModule.h"
#include "qSlicerMetafileImporterModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerMetafileImporterModule, qSlicerMetafileImporterModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerMetafileImporterModulePrivate
{
public:
  qSlicerMetafileImporterModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerMetafileImporterModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerMetafileImporterModulePrivate
::qSlicerMetafileImporterModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerMetafileImporterModule methods

//-----------------------------------------------------------------------------
qSlicerMetafileImporterModule
::qSlicerMetafileImporterModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerMetafileImporterModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerMetafileImporterModule::~qSlicerMetafileImporterModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerMetafileImporterModule::helpText()const
{
  return "This module is to import sequence metafiles into MultiDimension hierarchies. For help on how to use this module please visit: <a href='http://www.slicerrt.org/'>SlicerRT</a>";
}

//-----------------------------------------------------------------------------
QString qSlicerMetafileImporterModule::acknowledgementText()const
{
  return "Supported by SparKit and the Slicer Community.";
}

//-----------------------------------------------------------------------------
QStringList qSlicerMetafileImporterModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Matthew S. Holden (Queen's University)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerMetafileImporterModule::icon()const
{
  return QIcon(":/Icons/MetafileImporter.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerMetafileImporterModule::categories() const
{
  return QStringList() << "MultiDimension";
}

//-----------------------------------------------------------------------------
QStringList qSlicerMetafileImporterModule::dependencies() const
{
  return QStringList() << "MultiDimension";
}

//-----------------------------------------------------------------------------
void qSlicerMetafileImporterModule::setup()
{
  this->Superclass::setup();

  vtkSlicerMetafileImporterLogic* MetafileImporterLogic = vtkSlicerMetafileImporterLogic::SafeDownCast( this->logic() );
  qSlicerAbstractCoreModule* MultiDimensionModule = qSlicerCoreApplication::application()->moduleManager()->module( "MultiDimension" );

  if ( MultiDimensionModule )
  {
    MetafileImporterLogic->MultiDimensionLogic = vtkSlicerMultiDimensionLogic::SafeDownCast( MultiDimensionModule->logic() );
  }
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerMetafileImporterModule
::createWidgetRepresentation()
{
  return new qSlicerMetafileImporterModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerMetafileImporterModule::createLogic()
{
  return vtkSlicerMetafileImporterLogic::New();
}
