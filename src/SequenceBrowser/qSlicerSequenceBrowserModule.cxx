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

// SequenceBrowser Logic includes
#include <vtkSlicerSequenceBrowserLogic.h>

// SequenceBrowser includes
#include "qSlicerSequenceBrowserModule.h"
#include "qSlicerSequenceBrowserModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerSequenceBrowserModule, qSlicerSequenceBrowserModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerSequenceBrowserModulePrivate
{
public:
  qSlicerSequenceBrowserModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerSequenceBrowserModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerSequenceBrowserModulePrivate
::qSlicerSequenceBrowserModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerSequenceBrowserModule methods

//-----------------------------------------------------------------------------
qSlicerSequenceBrowserModule
::qSlicerSequenceBrowserModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerSequenceBrowserModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerSequenceBrowserModule::~qSlicerSequenceBrowserModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerSequenceBrowserModule::helpText()const
{
  return "This is a module for browsing or replaying a node sequence. See more information in the  <a href=\"http://www.slicer.org/slicerWiki/index.php/Documentation/Nightly/Extensions/Sequence\">online documentation</a>.";
}

//-----------------------------------------------------------------------------
QString qSlicerSequenceBrowserModule::acknowledgementText()const
{
  return "This work was funded by CCO ACRU and OCAIRO grants.";
}

//-----------------------------------------------------------------------------
QStringList qSlicerSequenceBrowserModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Andras Lasso (PerkLab, Queen's), Matthew Holden (PerkLab, Queen's), Kevin Wang (PMH)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerSequenceBrowserModule::icon()const
{
  return QIcon(":/Icons/SequenceBrowser.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerSequenceBrowserModule::categories() const
{
  return QStringList() << "Multidimensional data";
}

//-----------------------------------------------------------------------------
QStringList qSlicerSequenceBrowserModule::dependencies() const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerSequenceBrowserModule
::createWidgetRepresentation()
{
  return new qSlicerSequenceBrowserModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerSequenceBrowserModule::createLogic()
{
  return vtkSlicerSequenceBrowserLogic::New();
}
