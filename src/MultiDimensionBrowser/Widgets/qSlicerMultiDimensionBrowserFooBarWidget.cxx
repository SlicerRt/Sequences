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
#include "qSlicerMultiDimensionBrowserFooBarWidget.h"
#include "ui_qSlicerMultiDimensionBrowserFooBarWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_MultiDimensionBrowser
class qSlicerMultiDimensionBrowserFooBarWidgetPrivate
  : public Ui_qSlicerMultiDimensionBrowserFooBarWidget
{
  Q_DECLARE_PUBLIC(qSlicerMultiDimensionBrowserFooBarWidget);
protected:
  qSlicerMultiDimensionBrowserFooBarWidget* const q_ptr;

public:
  qSlicerMultiDimensionBrowserFooBarWidgetPrivate(
    qSlicerMultiDimensionBrowserFooBarWidget& object);
  virtual void setupUi(qSlicerMultiDimensionBrowserFooBarWidget*);
};

// --------------------------------------------------------------------------
qSlicerMultiDimensionBrowserFooBarWidgetPrivate
::qSlicerMultiDimensionBrowserFooBarWidgetPrivate(
  qSlicerMultiDimensionBrowserFooBarWidget& object)
  : q_ptr(&object)
{
}

// --------------------------------------------------------------------------
void qSlicerMultiDimensionBrowserFooBarWidgetPrivate
::setupUi(qSlicerMultiDimensionBrowserFooBarWidget* widget)
{
  this->Ui_qSlicerMultiDimensionBrowserFooBarWidget::setupUi(widget);
}

//-----------------------------------------------------------------------------
// qSlicerMultiDimensionBrowserFooBarWidget methods

//-----------------------------------------------------------------------------
qSlicerMultiDimensionBrowserFooBarWidget
::qSlicerMultiDimensionBrowserFooBarWidget(QWidget* parentWidget)
  : Superclass( parentWidget )
  , d_ptr( new qSlicerMultiDimensionBrowserFooBarWidgetPrivate(*this) )
{
  Q_D(qSlicerMultiDimensionBrowserFooBarWidget);
  d->setupUi(this);
}

//-----------------------------------------------------------------------------
qSlicerMultiDimensionBrowserFooBarWidget
::~qSlicerMultiDimensionBrowserFooBarWidget()
{
}
