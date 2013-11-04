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
#include "qSlicerMultidimDataBrowserFooBarWidget.h"
#include "ui_qSlicerMultidimDataBrowserFooBarWidget.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_MultidimDataBrowser
class qSlicerMultidimDataBrowserFooBarWidgetPrivate
  : public Ui_qSlicerMultidimDataBrowserFooBarWidget
{
  Q_DECLARE_PUBLIC(qSlicerMultidimDataBrowserFooBarWidget);
protected:
  qSlicerMultidimDataBrowserFooBarWidget* const q_ptr;

public:
  qSlicerMultidimDataBrowserFooBarWidgetPrivate(
    qSlicerMultidimDataBrowserFooBarWidget& object);
  virtual void setupUi(qSlicerMultidimDataBrowserFooBarWidget*);
};

// --------------------------------------------------------------------------
qSlicerMultidimDataBrowserFooBarWidgetPrivate
::qSlicerMultidimDataBrowserFooBarWidgetPrivate(
  qSlicerMultidimDataBrowserFooBarWidget& object)
  : q_ptr(&object)
{
}

// --------------------------------------------------------------------------
void qSlicerMultidimDataBrowserFooBarWidgetPrivate
::setupUi(qSlicerMultidimDataBrowserFooBarWidget* widget)
{
  this->Ui_qSlicerMultidimDataBrowserFooBarWidget::setupUi(widget);
}

//-----------------------------------------------------------------------------
// qSlicerMultidimDataBrowserFooBarWidget methods

//-----------------------------------------------------------------------------
qSlicerMultidimDataBrowserFooBarWidget
::qSlicerMultidimDataBrowserFooBarWidget(QWidget* parentWidget)
  : Superclass( parentWidget )
  , d_ptr( new qSlicerMultidimDataBrowserFooBarWidgetPrivate(*this) )
{
  Q_D(qSlicerMultidimDataBrowserFooBarWidget);
  d->setupUi(this);
}

//-----------------------------------------------------------------------------
qSlicerMultidimDataBrowserFooBarWidget
::~qSlicerMultidimDataBrowserFooBarWidget()
{
}
