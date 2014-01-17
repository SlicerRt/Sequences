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
#include "qSlicerSequencesFooBarWidget.h"
#include "ui_qSlicerSequencesFooBarWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Sequence
class qSlicerSequencesFooBarWidgetPrivate
  : public Ui_qSlicerSequencesFooBarWidget
{
  Q_DECLARE_PUBLIC(qSlicerSequencesFooBarWidget);
protected:
  qSlicerSequencesFooBarWidget* const q_ptr;

public:
  qSlicerSequencesFooBarWidgetPrivate(
    qSlicerSequencesFooBarWidget& object);
  virtual void setupUi(qSlicerSequencesFooBarWidget*);
};

// --------------------------------------------------------------------------
qSlicerSequencesFooBarWidgetPrivate
::qSlicerSequencesFooBarWidgetPrivate(
  qSlicerSequencesFooBarWidget& object)
  : q_ptr(&object)
{
}

// --------------------------------------------------------------------------
void qSlicerSequencesFooBarWidgetPrivate
::setupUi(qSlicerSequencesFooBarWidget* widget)
{
  this->Ui_qSlicerSequencesFooBarWidget::setupUi(widget);
}

//-----------------------------------------------------------------------------
// qSlicerSequencesFooBarWidget methods

//-----------------------------------------------------------------------------
qSlicerSequencesFooBarWidget
::qSlicerSequencesFooBarWidget(QWidget* parentWidget)
  : Superclass( parentWidget )
  , d_ptr( new qSlicerSequencesFooBarWidgetPrivate(*this) )
{
  Q_D(qSlicerSequencesFooBarWidget);
  d->setupUi(this);
}

//-----------------------------------------------------------------------------
qSlicerSequencesFooBarWidget
::~qSlicerSequencesFooBarWidget()
{
}
