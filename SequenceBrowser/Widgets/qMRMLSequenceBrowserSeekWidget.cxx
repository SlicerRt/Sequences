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

  This file was originally developed by Laurent Chauvin, Brigham and Women's
  Hospital. The project was supported by grants 5P01CA067165,
  5R01CA124377, 5R01CA138586, 2R44DE019322, 7R01CA124377,
  5R42CA137886, 5R42CA137886
  It was then updated for the Markups module by Nicole Aucoin, BWH.

==============================================================================*/

// qMRML includes
#include "qMRMLSequenceBrowserSeekWidget.h"
#include "ui_qMRMLSequenceBrowserSeekWidget.h"

// MRML includes
#include <vtkMRMLSequenceBrowserNode.h>
#include <vtkMRMLSequenceNode.h>

// Qt includes
#include <QDebug>

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Markups
class qMRMLSequenceBrowserSeekWidgetPrivate
  : public Ui_qMRMLSequenceBrowserSeekWidget
{
  Q_DECLARE_PUBLIC(qMRMLSequenceBrowserSeekWidget);
protected:
  qMRMLSequenceBrowserSeekWidget* const q_ptr;
public:
  qMRMLSequenceBrowserSeekWidgetPrivate(qMRMLSequenceBrowserSeekWidget& object);
  void init();

  vtkWeakPointer<vtkMRMLSequenceBrowserNode> SequenceBrowserNode;
};

//-----------------------------------------------------------------------------
// qMRMLSequenceBrowserSeekWidgetPrivate methods

//-----------------------------------------------------------------------------
qMRMLSequenceBrowserSeekWidgetPrivate
::qMRMLSequenceBrowserSeekWidgetPrivate(qMRMLSequenceBrowserSeekWidget& object)
  : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------
void qMRMLSequenceBrowserSeekWidgetPrivate
::init()
{
  Q_Q(qMRMLSequenceBrowserSeekWidget);
  this->setupUi(q);
  QObject::connect( this->slider_IndexValue, SIGNAL(valueChanged(int)), q, SLOT(setSelectedItemNumber(int)) );  
  /*
  QObject::connect(this->point2DProjectionCheckBox, SIGNAL(toggled(bool)),
                   q, SLOT(setProjectionVisibility(bool)));
  QObject::connect(this->pointProjectionColorPickerButton, SIGNAL(colorChanged(QColor)),
                   q, SLOT(setProjectionColor(QColor)));
  QObject::connect(this->pointUseFiducialColorCheckBox, SIGNAL(toggled(bool)),
                   q, SLOT(setUseFiducialColor(bool)));
  QObject::connect(this->pointOutlinedBehindSlicePlaneCheckBox, SIGNAL(toggled(bool)),
                   q, SLOT(setOutlinedBehindSlicePlane(bool)));
  QObject::connect(this->projectionOpacitySliderWidget, SIGNAL(valueChanged(double)),
                   q, SLOT(setProjectionOpacity(double)));
                   */
  q->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
// qMRMLSequenceBrowserSeekWidget methods

//-----------------------------------------------------------------------------
qMRMLSequenceBrowserSeekWidget
::qMRMLSequenceBrowserSeekWidget(QWidget *newParent) :
    Superclass(newParent)
  , d_ptr(new qMRMLSequenceBrowserSeekWidgetPrivate(*this))
{
  Q_D(qMRMLSequenceBrowserSeekWidget);
  d->init();
}

//-----------------------------------------------------------------------------
qMRMLSequenceBrowserSeekWidget
::~qMRMLSequenceBrowserSeekWidget()
{
}

//-----------------------------------------------------------------------------
void qMRMLSequenceBrowserSeekWidget
::setMRMLSequenceBrowserNode(vtkMRMLSequenceBrowserNode* browserNode)
{
  Q_D(qMRMLSequenceBrowserSeekWidget);

  qvtkReconnect(d->SequenceBrowserNode, browserNode, vtkCommand::ModifiedEvent,
                this, SLOT(updateWidgetFromMRML()));

  d->SequenceBrowserNode = browserNode;
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qMRMLSequenceBrowserSeekWidget::setSelectedItemNumber(int itemNumber)
{
  Q_D(qMRMLSequenceBrowserSeekWidget);
  if (d->SequenceBrowserNode==NULL)
  {
    qCritical("setSelectedItemNumber failed: browser node is invalid");
    this->updateWidgetFromMRML();
    return;
  }
  int selectedItemNumber=-1;
  vtkMRMLSequenceNode* sequenceNode=d->SequenceBrowserNode->GetMasterSequenceNode();  
  if (sequenceNode!=NULL && itemNumber>=0)
  {
    if (itemNumber<sequenceNode->GetNumberOfDataNodes())
    {
      selectedItemNumber=itemNumber;
    }
  }
  d->SequenceBrowserNode->SetSelectedItemNumber(selectedItemNumber);
}

//-----------------------------------------------------------------------------
void qMRMLSequenceBrowserSeekWidget::updateWidgetFromMRML()
{
  Q_D(qMRMLSequenceBrowserSeekWidget);
  vtkMRMLSequenceNode* sequenceNode = d->SequenceBrowserNode.GetPointer() ? d->SequenceBrowserNode->GetMasterSequenceNode() : NULL;
  this->setEnabled(sequenceNode != NULL);
  if (!sequenceNode)
    {
    d->label_IndexName->setText("");
    d->label_IndexUnit->setText("");
    d->label_IndexValue->setText("");
    return;
    }

  QString DEFAULT_INDEX_NAME_STRING=tr("time");
  const char* indexName=sequenceNode->GetIndexName();
  const char* sequenceNodeId = sequenceNode->GetID() ? sequenceNode->GetID() : "(none)";
  if (indexName!=NULL)
  {
    d->label_IndexName->setText(indexName);
  }
  else
  {
    qWarning() << "Index name is not specified in node" << sequenceNodeId;
    d->label_IndexName->setText(DEFAULT_INDEX_NAME_STRING);
  }

  const char* indexUnit=sequenceNode->GetIndexUnit();
  if (indexUnit!=NULL)
  {
    d->label_IndexUnit->setText(indexUnit);
  }
  else
  {
    qWarning() << "IndexUnit name is not specified in node" << sequenceNodeId;
    d->label_IndexUnit->setText("");
  }
  
  int numberOfDataNodes=sequenceNode->GetNumberOfDataNodes();
  if (numberOfDataNodes>0)
  {
    d->slider_IndexValue->setEnabled(true);
    d->slider_IndexValue->setMinimum(0);
    d->slider_IndexValue->setMaximum(numberOfDataNodes-1);
  }
  else
  {
    qDebug() << "Number of child nodes in the selected hierarchy is 0 in node "<<sequenceNodeId;
    d->slider_IndexValue->setEnabled(false);
  }   

  int selectedItemNumber=d->SequenceBrowserNode->GetSelectedItemNumber();
  if (selectedItemNumber>=0)
  {
    std::string indexValue=sequenceNode->GetNthIndexValue(selectedItemNumber);
    if (!indexValue.empty())
    {
      d->label_IndexValue->setText(indexValue.c_str());
      d->slider_IndexValue->setValue(selectedItemNumber);
    }
    else
    {
      qWarning() << "Item "<<selectedItemNumber<<" has no index value defined";
      d->label_IndexValue->setText("");
      d->slider_IndexValue->setValue(0);
    }  
  }
  else
  {
    d->label_IndexValue->setText("");
    d->slider_IndexValue->setValue(0);
  }  
}
