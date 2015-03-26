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

// Slicer includes
#include "qSlicerIOManager.h"
#include "qSlicerNodeWriter.h"

// Sequence Logic includes
#include <vtkSlicerSequencesLogic.h>

// Sequence includes
#include "qSlicerSequencesModule.h"
#include "qSlicerSequencesModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerSequencesModule, qSlicerSequencesModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerSequencesModulePrivate
{
public:
  qSlicerSequencesModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerSequencesModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerSequencesModulePrivate
::qSlicerSequencesModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerSequencesModule methods

//-----------------------------------------------------------------------------
qSlicerSequencesModule
::qSlicerSequencesModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerSequencesModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerSequencesModule::~qSlicerSequencesModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerSequencesModule::helpText()const
{
  return "This is a module for creating a node sequence from regular nodes and edit an existing sequence. See more information in the  <a href=\"http://www.slicer.org/slicerWiki/index.php/Documentation/Nightly/Extensions/Sequences\">online documentation</a>.";
}

//-----------------------------------------------------------------------------
QString qSlicerSequencesModule::acknowledgementText()const
{
  return "This work was funded by CCO ACRU and OCAIRO grants.";
}

//-----------------------------------------------------------------------------
QStringList qSlicerSequencesModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Andras Lasso (PerkLab, Queen's), Matthew Holden (PerkLab, Queen's), Kevin Wang (PMH)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerSequencesModule::icon()const
{
  return QIcon(":/Icons/Sequences.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerSequencesModule::categories() const
{
  return QStringList() << "Sequences";
}

//-----------------------------------------------------------------------------
QStringList qSlicerSequencesModule::dependencies() const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerSequencesModule::setup()
{
  this->Superclass::setup();
  // Register IOs
  qSlicerIOManager* ioManager = qSlicerApplication::application()->ioManager();
  ioManager->registerIO(new qSlicerNodeWriter("Sequences", QString("SequenceFile"), QStringList() << "vtkMRMLSequenceNode", this));
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerSequencesModule
::createWidgetRepresentation()
{
  return new qSlicerSequencesModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerSequencesModule::createLogic()
{
  return vtkSlicerSequencesLogic::New();
}
