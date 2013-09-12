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

// MultiDimensionBrowser Logic includes
#include <vtkSlicerMultiDimensionBrowserLogic.h>

// MultiDimensionBrowser includes
#include "qSlicerMultiDimensionBrowserModule.h"
#include "qSlicerMultiDimensionBrowserModuleWidget.h"

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerMultiDimensionBrowserModule, qSlicerMultiDimensionBrowserModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerMultiDimensionBrowserModulePrivate
{
public:
  qSlicerMultiDimensionBrowserModulePrivate();
};

//-----------------------------------------------------------------------------
// qSlicerMultiDimensionBrowserModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerMultiDimensionBrowserModulePrivate
::qSlicerMultiDimensionBrowserModulePrivate()
{
}

//-----------------------------------------------------------------------------
// qSlicerMultiDimensionBrowserModule methods

//-----------------------------------------------------------------------------
qSlicerMultiDimensionBrowserModule
::qSlicerMultiDimensionBrowserModule(QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerMultiDimensionBrowserModulePrivate)
{
}

//-----------------------------------------------------------------------------
qSlicerMultiDimensionBrowserModule::~qSlicerMultiDimensionBrowserModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerMultiDimensionBrowserModule::helpText()const
{
  return "This is a loadable module for handling multi-dimensional data";
}

//-----------------------------------------------------------------------------
QString qSlicerMultiDimensionBrowserModule::acknowledgementText()const
{
  return "This work was funded by CCO ACRU and OCARIO";
}

//-----------------------------------------------------------------------------
QStringList qSlicerMultiDimensionBrowserModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Matthew Holden (Queen's), Kevin Wang (PMH)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerMultiDimensionBrowserModule::icon()const
{
  return QIcon(":/Icons/MultiDimensionBrowser.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerMultiDimensionBrowserModule::categories() const
{
  return QStringList() << "Multi-dimension";
}

//-----------------------------------------------------------------------------
QStringList qSlicerMultiDimensionBrowserModule::dependencies() const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserModule::setup()
{
  this->Superclass::setup();
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerMultiDimensionBrowserModule
::createWidgetRepresentation()
{
  return new qSlicerMultiDimensionBrowserModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerMultiDimensionBrowserModule::createLogic()
{
  return vtkSlicerMultiDimensionBrowserLogic::New();
}
