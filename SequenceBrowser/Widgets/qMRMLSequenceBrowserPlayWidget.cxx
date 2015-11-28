/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Brigham and Women's Hospital

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// qMRML includes
#include "qMRMLSequenceBrowserPlayWidget.h"
#include "ui_qMRMLSequenceBrowserPlayWidget.h"

// MRML includes
#include <vtkMRMLSequenceBrowserNode.h>
#include <vtkMRMLSequenceNode.h>

// Qt includes
#include <QDebug>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Markups
class qMRMLSequenceBrowserPlayWidgetPrivate
  : public Ui_qMRMLSequenceBrowserPlayWidget
{
  Q_DECLARE_PUBLIC(qMRMLSequenceBrowserPlayWidget);
protected:
  qMRMLSequenceBrowserPlayWidget* const q_ptr;
public:
  qMRMLSequenceBrowserPlayWidgetPrivate(qMRMLSequenceBrowserPlayWidget& object);
  void init();

  vtkWeakPointer<vtkMRMLSequenceBrowserNode> SequenceBrowserNode;
};

//-----------------------------------------------------------------------------
// qMRMLSequenceBrowserPlayWidgetPrivate methods

//-----------------------------------------------------------------------------
qMRMLSequenceBrowserPlayWidgetPrivate::qMRMLSequenceBrowserPlayWidgetPrivate(qMRMLSequenceBrowserPlayWidget& object)
  : q_ptr(&object)
{
  this->SequenceBrowserNode = NULL;
}

//-----------------------------------------------------------------------------
void qMRMLSequenceBrowserPlayWidgetPrivate::init()
{
  Q_Q(qMRMLSequenceBrowserPlayWidget);
  this->setupUi(q);

  QObject::connect( this->pushButton_VcrFirst, SIGNAL(clicked()), q, SLOT(onVcrFirst()) );
  QObject::connect( this->pushButton_VcrPrevious, SIGNAL(clicked()), q, SLOT(onVcrPrevious()) );
  QObject::connect( this->pushButton_VcrNext, SIGNAL(clicked()), q, SLOT(onVcrNext()) );
  QObject::connect( this->pushButton_VcrLast, SIGNAL(clicked()), q, SLOT(onVcrLast()) );
  QObject::connect( this->pushButton_VcrPlayPause, SIGNAL(toggled(bool)), q, SLOT(setPlaybackEnabled(bool)) );
  QObject::connect( this->pushButton_VcrLoop, SIGNAL(toggled(bool)), q, SLOT(setPlaybackLoopEnabled(bool)) );
  QObject::connect( this->doubleSpinBox_VcrPlaybackRate, SIGNAL(valueChanged(double)), q, SLOT(setPlaybackRateFps(double)) );

  q->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
// qMRMLSequenceBrowserPlayWidget methods

//-----------------------------------------------------------------------------
qMRMLSequenceBrowserPlayWidget::qMRMLSequenceBrowserPlayWidget(QWidget *newParent) :
    Superclass(newParent)
  , d_ptr(new qMRMLSequenceBrowserPlayWidgetPrivate(*this))
{
  Q_D(qMRMLSequenceBrowserPlayWidget);
  d->init();
}

//-----------------------------------------------------------------------------
qMRMLSequenceBrowserPlayWidget::~qMRMLSequenceBrowserPlayWidget()
{
}

//-----------------------------------------------------------------------------
void qMRMLSequenceBrowserPlayWidget::setMRMLSequenceBrowserNode(vtkMRMLSequenceBrowserNode* browserNode)
{
  Q_D(qMRMLSequenceBrowserPlayWidget);

  qvtkReconnect(d->SequenceBrowserNode, browserNode, vtkCommand::ModifiedEvent,
                this, SLOT(updateWidgetFromMRML()));

  d->SequenceBrowserNode = browserNode;
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qMRMLSequenceBrowserPlayWidget::updateWidgetFromMRML()
{
  Q_D(qMRMLSequenceBrowserPlayWidget);

  vtkMRMLSequenceNode* sequenceNode = d->SequenceBrowserNode.GetPointer() ? d->SequenceBrowserNode->GetMasterSequenceNode() : NULL;
  this->setEnabled(sequenceNode != NULL);
  if (!sequenceNode)
    {
    return;
    }

  QObjectList vcrControls;
  vcrControls 
    << d->pushButton_VcrFirst << d->pushButton_VcrLast << d->pushButton_VcrLoop
    << d->pushButton_VcrNext << d->pushButton_VcrPlayPause << d->pushButton_VcrPrevious;
  bool vcrControlsEnabled=false;   

  int numberOfDataNodes=sequenceNode->GetNumberOfDataNodes();
  if (numberOfDataNodes>0)
  {
    vcrControlsEnabled=true;
    
    bool pushButton_VcrPlayPauseBlockSignals = d->pushButton_VcrPlayPause->blockSignals(true);
    d->pushButton_VcrPlayPause->setChecked(d->SequenceBrowserNode->GetPlaybackActive());
    d->pushButton_VcrPlayPause->blockSignals(pushButton_VcrPlayPauseBlockSignals);

    bool pushButton_VcrLoopBlockSignals = d->pushButton_VcrLoop->blockSignals(true);
    d->pushButton_VcrLoop->setChecked(d->SequenceBrowserNode->GetPlaybackLooped());
    d->pushButton_VcrLoop->blockSignals(pushButton_VcrLoopBlockSignals);
  }

  d->doubleSpinBox_VcrPlaybackRate->setValue(d->SequenceBrowserNode->GetPlaybackRateFps());

  foreach( QObject*w, vcrControls ) { w->setProperty( "enabled", vcrControlsEnabled ); }
}

//-----------------------------------------------------------------------------
void qMRMLSequenceBrowserPlayWidget::onVcrFirst()
{
  Q_D(qMRMLSequenceBrowserPlayWidget);
  if (d->SequenceBrowserNode==NULL)
  {
    qCritical() << "onVcrFirst failed: no active browser node is selected";
    updateWidgetFromMRML();
    return;
  }
  d->SequenceBrowserNode->SelectFirstItem();
}

//-----------------------------------------------------------------------------
void qMRMLSequenceBrowserPlayWidget::onVcrLast()
{
  Q_D(qMRMLSequenceBrowserPlayWidget);
  if (d->SequenceBrowserNode==NULL)
  {
    qCritical() << "onVcrLast failed: no active browser node is selected";
    updateWidgetFromMRML();
    return;
  }
  d->SequenceBrowserNode->SelectLastItem();
}

//-----------------------------------------------------------------------------
void qMRMLSequenceBrowserPlayWidget::onVcrPrevious()
{
  Q_D(qMRMLSequenceBrowserPlayWidget);
  if (d->SequenceBrowserNode==NULL)
  {
    qCritical() << "onVcrPrevious failed: no active browser node is selected";
    updateWidgetFromMRML();
    return;
  }
  d->SequenceBrowserNode->SelectNextItem(-1);
}

//-----------------------------------------------------------------------------
void qMRMLSequenceBrowserPlayWidget::onVcrNext()
{
  Q_D(qMRMLSequenceBrowserPlayWidget);
  if (d->SequenceBrowserNode==NULL)
  {
    qCritical() << "onVcrNext failed: no active browser node is selected";
    updateWidgetFromMRML();
    return;
  }
  d->SequenceBrowserNode->SelectNextItem(1);
}

//-----------------------------------------------------------------------------
void qMRMLSequenceBrowserPlayWidget::setPlaybackEnabled(bool play)
{
  Q_D(qMRMLSequenceBrowserPlayWidget);
  if (d->SequenceBrowserNode==NULL)
  {
    qCritical() << "onVcrPlayPauseStateChanged failed: no active browser node is selected";
    updateWidgetFromMRML();
    return;
  }
  if (play!=d->SequenceBrowserNode->GetPlaybackActive())
  {
    d->SequenceBrowserNode->SetPlaybackActive(play);
  }
}

//-----------------------------------------------------------------------------
void qMRMLSequenceBrowserPlayWidget::setPlaybackLoopEnabled(bool loopEnabled)
{
  Q_D(qMRMLSequenceBrowserPlayWidget);
  if (d->SequenceBrowserNode==NULL)
  {
    qCritical() << "onVcrPlaybackLoopStateChanged failed: no active browser node is selected";
    this->updateWidgetFromMRML();
    return;
  }
  if (loopEnabled!=d->SequenceBrowserNode->GetPlaybackLooped())
  {
    d->SequenceBrowserNode->SetPlaybackLooped(loopEnabled);
  }
}

//-----------------------------------------------------------------------------
void qMRMLSequenceBrowserPlayWidget::setPlaybackRateFps(double playbackRateFps)
{
  Q_D(qMRMLSequenceBrowserPlayWidget);
  if (d->SequenceBrowserNode==NULL)
  {
    qCritical() << "setPlaybackRateFps failed: no active browser node is selected";
    this->updateWidgetFromMRML();
    return;
  }
  if (playbackRateFps!=d->SequenceBrowserNode->GetPlaybackRateFps())
  {
    d->SequenceBrowserNode->SetPlaybackRateFps(playbackRateFps);
  }
}
