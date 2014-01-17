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
#include "qSlicerSequenceBrowserFooBarWidget.h"
#include "ui_qSlicerSequenceBrowserFooBarWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_SequenceBrowser
class qSlicerSequenceBrowserFooBarWidgetPrivate
  : public Ui_qSlicerSequenceBrowserFooBarWidget
{
  Q_DECLARE_PUBLIC(qSlicerSequenceBrowserFooBarWidget);
protected:
  qSlicerSequenceBrowserFooBarWidget* const q_ptr;

public:
  qSlicerSequenceBrowserFooBarWidgetPrivate(
    qSlicerSequenceBrowserFooBarWidget& object);
  virtual void setupUi(qSlicerSequenceBrowserFooBarWidget*);
};

// --------------------------------------------------------------------------
qSlicerSequenceBrowserFooBarWidgetPrivate
::qSlicerSequenceBrowserFooBarWidgetPrivate(
  qSlicerSequenceBrowserFooBarWidget& object)
  : q_ptr(&object)
{
}

// --------------------------------------------------------------------------
void qSlicerSequenceBrowserFooBarWidgetPrivate
::setupUi(qSlicerSequenceBrowserFooBarWidget* widget)
{
  this->Ui_qSlicerSequenceBrowserFooBarWidget::setupUi(widget);
}

//-----------------------------------------------------------------------------
// qSlicerSequenceBrowserFooBarWidget methods

//-----------------------------------------------------------------------------
qSlicerSequenceBrowserFooBarWidget
::qSlicerSequenceBrowserFooBarWidget(QWidget* parentWidget)
  : Superclass( parentWidget )
  , d_ptr( new qSlicerSequenceBrowserFooBarWidgetPrivate(*this) )
{
  Q_D(qSlicerSequenceBrowserFooBarWidget);
  d->setupUi(this);
}

//-----------------------------------------------------------------------------
qSlicerSequenceBrowserFooBarWidget
::~qSlicerSequenceBrowserFooBarWidget()
{
}
