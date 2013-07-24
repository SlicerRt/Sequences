/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Jean-Christophe Fillion-Robin, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// FooBar Widgets includes
#include "qSlicerMetafileImporterFooBarWidget.h"
#include "ui_qSlicerMetafileImporterFooBarWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_MetafileImporter
class qSlicerMetafileImporterFooBarWidgetPrivate
  : public Ui_qSlicerMetafileImporterFooBarWidget
{
  Q_DECLARE_PUBLIC(qSlicerMetafileImporterFooBarWidget);
protected:
  qSlicerMetafileImporterFooBarWidget* const q_ptr;

public:
  qSlicerMetafileImporterFooBarWidgetPrivate(
    qSlicerMetafileImporterFooBarWidget& object);
  virtual void setupUi(qSlicerMetafileImporterFooBarWidget*);
};

// --------------------------------------------------------------------------
qSlicerMetafileImporterFooBarWidgetPrivate
::qSlicerMetafileImporterFooBarWidgetPrivate(
  qSlicerMetafileImporterFooBarWidget& object)
  : q_ptr(&object)
{
}

// --------------------------------------------------------------------------
void qSlicerMetafileImporterFooBarWidgetPrivate
::setupUi(qSlicerMetafileImporterFooBarWidget* widget)
{
  this->Ui_qSlicerMetafileImporterFooBarWidget::setupUi(widget);
}

//-----------------------------------------------------------------------------
// qSlicerMetafileImporterFooBarWidget methods

//-----------------------------------------------------------------------------
qSlicerMetafileImporterFooBarWidget
::qSlicerMetafileImporterFooBarWidget(QWidget* parentWidget)
  : Superclass( parentWidget )
  , d_ptr( new qSlicerMetafileImporterFooBarWidgetPrivate(*this) )
{
  Q_D(qSlicerMetafileImporterFooBarWidget);
  d->setupUi(this);
}

//-----------------------------------------------------------------------------
qSlicerMetafileImporterFooBarWidget
::~qSlicerMetafileImporterFooBarWidget()
{
}
