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
#include "vtkSlicerSequencesLogic.h"

// MetafileImporter includes
#include "qSlicerMetafileImporterModule.h"
#include "qSlicerMetafileImporterModuleWidget.h"
#include "qSlicerMetafileImporterIO.h"
#include "qSlicerVolumeSequenceImporterIO.h"

// Slicer includes
#include "qSlicerAbstractCoreModule.h"
#include "qSlicerCoreApplication.h"
#include "qSlicerCoreIOManager.h"
#include "qSlicerNodeWriter.h"
#include "qSlicerSequenceBrowserModule.h"
#include "qSlicerSequenceBrowserModuleWidget.h"

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
  return "This is a module for importing imageing and tracking information from a sequence metafile into node sequences. See more information in the  <a href=\"http://www.slicer.org/slicerWiki/index.php/Documentation/Nightly/Extensions/Sequences\">online documentation</a>.";
}

//-----------------------------------------------------------------------------
QString qSlicerMetafileImporterModule::acknowledgementText()const
{
  return "This work was funded by CCO ACRU and OCAIRO grants.";
}

//-----------------------------------------------------------------------------
QStringList qSlicerMetafileImporterModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Andras Lasso (PerkLab, Queen's), Matthew Holden (PerkLab, Queen's)");
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
  return QStringList() << "Sequences";
}

//-----------------------------------------------------------------------------
QStringList qSlicerMetafileImporterModule::dependencies() const
{
  return QStringList() << "Sequences" << "SequenceBrowser";
}

//-----------------------------------------------------------------------------
void qSlicerMetafileImporterModule::setup()
{
  this->Superclass::setup();

  qSlicerCoreApplication* app = qSlicerCoreApplication::application();

  vtkSlicerMetafileImporterLogic* metafileImporterLogic = vtkSlicerMetafileImporterLogic::SafeDownCast( this->logic() );
  qSlicerAbstractCoreModule* sequenceModule = app->moduleManager()->module( "Sequences" );
  if ( sequenceModule )
  {
    metafileImporterLogic->SetSequencesLogic(vtkSlicerSequencesLogic::SafeDownCast( sequenceModule->logic() ));
  }

  // Register the IO
  app->coreIOManager()->registerIO( new qSlicerMetafileImporterIO( metafileImporterLogic, this ) );
  app->coreIOManager()->registerIO( new qSlicerNodeWriter( "MetafileImporter", QString( "SequenceMetafile" ), QStringList(), true, this ) );

  app->coreIOManager()->registerIO( new qSlicerVolumeSequenceImporterIO( metafileImporterLogic, this ) );
  app->coreIOManager()->registerIO( new qSlicerNodeWriter( "Sequences", QString( "VolumeSequenceFile" ), QStringList() << "vtkMRMLSequenceNode", true, this ) );

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

//-----------------------------------------------------------------------------
bool qSlicerMetafileImporterModule::showSequenceBrowser(vtkMRMLSequenceBrowserNode* browserNode)
{
  qSlicerCoreApplication* app = qSlicerCoreApplication::application();
  if (!app
    || !app->moduleManager()
    || !dynamic_cast<qSlicerSequenceBrowserModule*>(app->moduleManager()->module("SequenceBrowser")) )
  {
    qCritical("SequenceBrowser module is not available");
    return false;
  }
  qSlicerSequenceBrowserModule* sequenceBrowserModule = dynamic_cast<qSlicerSequenceBrowserModule*>(app->moduleManager()->module("SequenceBrowser"));
  sequenceBrowserModule->setToolBarActiveBrowserNode(browserNode);
  sequenceBrowserModule->setToolBarVisible(true);
  return true;
}
