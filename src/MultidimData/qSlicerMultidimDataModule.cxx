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

// MultidimData Logic includes
#include <vtkSlicerMultidimDataLogic.h>

// MultidimData includes
#include "qSlicerMultidimDataModule.h"
#include "qSlicerMultidimDataModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerMultidimDataModule, qSlicerMultidimDataModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerMultidimDataModulePrivate
{
public:
  qSlicerMultidimDataModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerMultidimDataModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerMultidimDataModulePrivate
::qSlicerMultidimDataModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerMultidimDataModule methods

//-----------------------------------------------------------------------------
qSlicerMultidimDataModule
::qSlicerMultidimDataModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerMultidimDataModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerMultidimDataModule::~qSlicerMultidimDataModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerMultidimDataModule::helpText()const
{
  return "This is a module for creating a node sequence from regular nodes and edit an existing sequence. See more information in the  <a href=\"http://www.slicer.org/slicerWiki/index.php/Documentation/Nightly/Extensions/MultidimData\">online documentation</a>.";
}

//-----------------------------------------------------------------------------
QString qSlicerMultidimDataModule::acknowledgementText()const
{
  return "This work was funded by CCO ACRU and OCAIRO grants.";
}

//-----------------------------------------------------------------------------
QStringList qSlicerMultidimDataModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Andras Lasso (PerkLab, Queen's), Matthew Holden (PerkLab, Queen's), Kevin Wang (PMH)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerMultidimDataModule::icon()const
{
  return QIcon(":/Icons/MultidimData.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerMultidimDataModule::categories() const
{
  return QStringList() << "Multidimensional data";
}

//-----------------------------------------------------------------------------
QStringList qSlicerMultidimDataModule::dependencies() const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerMultidimDataModule
::createWidgetRepresentation()
{
  return new qSlicerMultidimDataModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerMultidimDataModule::createLogic()
{
  return vtkSlicerMultidimDataLogic::New();
}
