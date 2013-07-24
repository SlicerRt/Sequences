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
#include <vtkSlicerMetafileImporterLogic.h>

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
  return "This is a loadable module bundled in an extension";
}

//-----------------------------------------------------------------------------
QString qSlicerMetafileImporterModule::acknowledgementText()const
{
  return "This work was was partially funded by NIH grant 3P41RR013218-12S1";
}

//-----------------------------------------------------------------------------
QStringList qSlicerMetafileImporterModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Jean-Christophe Fillion-Robin (Kitware)");
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
  return QStringList() << "Examples";
}

//-----------------------------------------------------------------------------
QStringList qSlicerMetafileImporterModule::dependencies() const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerMetafileImporterModule::setup()
{
  this->Superclass::setup();
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
