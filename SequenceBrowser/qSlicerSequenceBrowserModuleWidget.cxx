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
#include <QCheckBox>
#include <QDebug>

// SlicerQt includes
#include "qMRMLSequenceBrowserToolBar.h"
#include "qSlicerSequenceBrowserModuleWidget.h"
#include "qSlicerSequenceBrowserModule.h"
#include "ui_qSlicerSequenceBrowserModuleWidget.h"

// MRML includes
#include "vtkMRMLCrosshairNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLTransformNode.h"

// Sequence includes
#include "vtkSlicerSequenceBrowserLogic.h"
#include "vtkMRMLNodeSequencer.h"
#include "vtkMRMLSequenceBrowserNode.h"
#include "vtkMRMLSequenceNode.h"

// VTK includes
#include <vtkAbstractTransform.h>
#include <vtkAxis.h>
#include <vtkChartXY.h>
#include <vtkFloatArray.h>
#include <vtkGeneralTransform.h>
#include <vtkImageData.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkPlot.h>
#include <vtkTable.h>
#include <vtkWeakPointer.h>

enum
{
  SYNCH_NODES_NAME_COLUMN=0,
  SYNCH_NODES_PROXY_COLUMN,
  SYNCH_NODES_PLAYBACK_COLUMN,
  SYNCH_NODES_RECORDING_COLUMN,
  SYNCH_NODES_OVERWRITE_PROXY_NAME_COLUMN,
  SYNCH_NODES_SAVE_CHANGES_COLUMN,
  SYNCH_NODES_NUMBER_OF_COLUMNS // this must be the last line in this enum
};

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Sequence
class qSlicerSequenceBrowserModuleWidgetPrivate: public Ui_qSlicerSequenceBrowserModuleWidget
{
  Q_DECLARE_PUBLIC(qSlicerSequenceBrowserModuleWidget);
protected:
  qSlicerSequenceBrowserModuleWidget* const q_ptr;
public:
  qSlicerSequenceBrowserModuleWidgetPrivate(qSlicerSequenceBrowserModuleWidget& object);
  ~qSlicerSequenceBrowserModuleWidgetPrivate();
  vtkSlicerSequenceBrowserLogic* logic() const;

  void init();
  void resetInteractiveCharting();
  void updateInteractiveCharting();
  void setAndObserveCrosshairNode();

  qMRMLSequenceBrowserToolBar* toolBar();

  /// Using this flag prevents overriding the parameter set node contents when the
  ///   QMRMLCombobox selects the first instance of the specified node type when initializing
  bool ModuleWindowInitialized;

  vtkWeakPointer<vtkMRMLSequenceBrowserNode> ActiveBrowserNode;

  vtkChartXY* ChartXY;
  vtkTable* ChartTable;
  vtkFloatArray* ArrayX;
  vtkFloatArray* ArrayY1;
  vtkFloatArray* ArrayY2;
  vtkFloatArray* ArrayY3;

  vtkWeakPointer<vtkMRMLCrosshairNode> CrosshairNode;
};

//-----------------------------------------------------------------------------
// qSlicerSequenceBrowserModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerSequenceBrowserModuleWidgetPrivate::qSlicerSequenceBrowserModuleWidgetPrivate(qSlicerSequenceBrowserModuleWidget& object)
  : q_ptr(&object)
  , ModuleWindowInitialized(false)
  , ChartXY(0)
  , ChartTable(0)
  , ArrayX(0)
  , ArrayY1(0)
  , ArrayY2(0)
  , ArrayY3(0)
{
  this->CrosshairNode = 0;
}

//-----------------------------------------------------------------------------
qSlicerSequenceBrowserModuleWidgetPrivate::~qSlicerSequenceBrowserModuleWidgetPrivate()
{
  if (ChartTable)
  {
    this->ChartTable->Delete();
  }
  if (ArrayX)
  {
    this->ArrayX->Delete();
  }
  if (ArrayY1)
  {
    this->ArrayY1->Delete();
  }
  if (ArrayY2)
  {
    this->ArrayY2->Delete();
  }
  if (ArrayY3)
  {
    this->ArrayY3->Delete();
  }
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidgetPrivate::setAndObserveCrosshairNode()
{
  Q_Q(qSlicerSequenceBrowserModuleWidget);

  vtkMRMLCrosshairNode* crosshairNode = 0;
  if (q->mrmlScene())
    {
    crosshairNode = vtkMRMLCrosshairNode::SafeDownCast(q->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLCrosshairNode"));
    }

  q->qvtkReconnect(this->CrosshairNode.GetPointer(), crosshairNode,
    vtkMRMLCrosshairNode::CursorPositionModifiedEvent,
    q, SLOT(updateChart()));
  this->CrosshairNode = crosshairNode;
}

//-----------------------------------------------------------------------------
vtkSlicerSequenceBrowserLogic*
qSlicerSequenceBrowserModuleWidgetPrivate::logic() const
{
  Q_Q(const qSlicerSequenceBrowserModuleWidget);

  vtkSlicerSequenceBrowserLogic* logic=vtkSlicerSequenceBrowserLogic::SafeDownCast(q->logic());
  if (logic==NULL)
  {
    qCritical() << "Sequence browser logic is invalid";
  }
  return logic;
}

//-----------------------------------------------------------------------------
qMRMLSequenceBrowserToolBar* qSlicerSequenceBrowserModuleWidgetPrivate::toolBar()
{
  Q_Q(const qSlicerSequenceBrowserModuleWidget);
  qSlicerSequenceBrowserModule* module = dynamic_cast<qSlicerSequenceBrowserModule*>(q->module());
  if (!module)
  {
    qWarning("qSlicerSequenceBrowserModuleWidget::toolBar failed: module is not set");
    return NULL;
  }
  return module->toolBar();
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidgetPrivate::init()
{
  this->ChartXY = this->ChartView_iCharting->chart();
  this->ChartTable = vtkTable::New();
  this->ArrayX = vtkFloatArray::New();
  this->ArrayY1 = vtkFloatArray::New();
  this->ArrayY2 = vtkFloatArray::New();
  this->ArrayY3 = vtkFloatArray::New();
  this->ArrayX->SetName("X axis");
  this->ArrayY1->SetName("Y1 axis");
  this->ArrayY2->SetName("Y2 axis");
  this->ArrayY3->SetName("Y3 axis");
  this->ChartTable->AddColumn(this->ArrayX);
  this->ChartTable->AddColumn(this->ArrayY1);
  this->ChartTable->AddColumn(this->ArrayY2);
  this->ChartTable->AddColumn(this->ArrayY3);

  this->resetInteractiveCharting();
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidgetPrivate::resetInteractiveCharting()
{
  this->ChartXY->RemovePlot(0);
  this->ChartXY->RemovePlot(0);
  this->ChartXY->RemovePlot(0);
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidgetPrivate::updateInteractiveCharting()
{
  Q_Q(qSlicerSequenceBrowserModuleWidget);

  if (this->CrosshairNode.GetPointer()==NULL)
  {
    qWarning() << "qSlicerSequenceBrowserModuleWidgetPrivate::updateInteractiveCharting failed: crosshair node is not available";
    resetInteractiveCharting();
    return;
  }
  vtkMRMLSequenceNode* sequenceNode = this->ActiveBrowserNode ? this->ActiveBrowserNode->GetMasterSequenceNode() : NULL;
  if (sequenceNode==NULL)
  {
    resetInteractiveCharting();
    return;
  }
  double croshairPosition_RAS[4]={0,0,0,1}; // homogeneous coordinate to allow transform by matrix multiplication
  bool validPosition = this->CrosshairNode->GetCursorPositionRAS(croshairPosition_RAS);
  if (!validPosition)
  {
    resetInteractiveCharting();
    return;
  }
  vtkMRMLNode* proxyNode = this->ActiveBrowserNode->GetProxyNode(sequenceNode);
  vtkMRMLTransformableNode* transformableProxyNode = vtkMRMLTransformableNode::SafeDownCast(proxyNode);

  int numberOfDataNodes = sequenceNode->GetNumberOfDataNodes();
  this->ChartTable->SetNumberOfRows(numberOfDataNodes);

  vtkMRMLScalarVolumeNode *vNode = vtkMRMLScalarVolumeNode::SafeDownCast(sequenceNode->GetNthDataNode(0));
  if (vNode)
  {
    int numOfScalarComponents = 0;
    numOfScalarComponents = vNode->GetImageData()->GetNumberOfScalarComponents();
    if (numOfScalarComponents > 3)
    {
      return;
    }
    vtkNew<vtkGeneralTransform> worldTransform;
    worldTransform->Identity();
    vtkMRMLTransformNode *transformNode = transformableProxyNode ? transformableProxyNode->GetParentTransformNode() : NULL;
    if ( transformNode )
    {
      transformNode->GetTransformFromWorld(worldTransform.GetPointer());
    }

    int numberOfValidPoints = 0;
    for (int i = 0; i<numberOfDataNodes; i++)
    {
      vNode = vtkMRMLScalarVolumeNode::SafeDownCast(sequenceNode->GetNthDataNode(i));
      this->ChartTable->SetValue(i, 0, i);

      vtkNew<vtkGeneralTransform> worldToIjkTransform;
      worldToIjkTransform->PostMultiply();
      worldToIjkTransform->Identity();
      vtkNew<vtkMatrix4x4> rasToIjkMatrix;
      vNode->GetRASToIJKMatrix(rasToIjkMatrix.GetPointer());
      worldToIjkTransform->Concatenate(rasToIjkMatrix.GetPointer());
      worldToIjkTransform->Concatenate(worldTransform.GetPointer());

      double *crosshairPositionDouble_IJK = worldToIjkTransform->TransformDoublePoint(croshairPosition_RAS);
      int croshairPosition_IJK[3]={vtkMath::Round(crosshairPositionDouble_IJK[0]), vtkMath::Round(crosshairPositionDouble_IJK[1]), vtkMath::Round(crosshairPositionDouble_IJK[2])};
      int* imageExtent = vNode->GetImageData()->GetExtent();
      bool isCrosshairInsideImage = imageExtent[0]<=croshairPosition_IJK[0] && croshairPosition_IJK[0]<=imageExtent[1]
          && imageExtent[2]<=croshairPosition_IJK[1] && croshairPosition_IJK[1]<=imageExtent[3]
          && imageExtent[4]<=croshairPosition_IJK[2] && croshairPosition_IJK[2]<=imageExtent[5];
      if (isCrosshairInsideImage)
      {
        numberOfValidPoints++;
      }
      for (int c = 0; c<numOfScalarComponents; c++)
      {
        double val = isCrosshairInsideImage ? vNode->GetImageData()->GetScalarComponentAsDouble(croshairPosition_IJK[0], croshairPosition_IJK[1], croshairPosition_IJK[2], c) : 0;
        this->ChartTable->SetValue(i, c+1, val);
      }
    }
    //this->ChartTable->Update();
    this->ChartXY->RemovePlot(0);
    this->ChartXY->RemovePlot(0);
    this->ChartXY->RemovePlot(0);

    if (numberOfValidPoints>0)
    {
      this->ChartXY->GetAxis(0)->SetTitle("Signal Intensity");
      this->ChartXY->GetAxis(1)->SetTitle("Time");
      for (int c = 0; c<numOfScalarComponents; c++)
      {
        vtkPlot* line = this->ChartXY->AddPlot(vtkChart::LINE);
        line->SetInputData(this->ChartTable, 0, c+1);
        //line->SetColor(255,0,0,255);
      }
    }
  }

  vtkMRMLTransformNode *tNode = vtkMRMLTransformNode::SafeDownCast(sequenceNode->GetNthDataNode(0));
  if (tNode)
  {
    for (int i = 0; i<numberOfDataNodes; i++)
    {
      tNode = vtkMRMLTransformNode::SafeDownCast(sequenceNode->GetNthDataNode(i));
      vtkAbstractTransform* trans2Parent = tNode->GetTransformToParent();

      double* transformedcroshairPosition_RAS = trans2Parent->TransformDoublePoint(croshairPosition_RAS);

      this->ChartTable->SetValue(i, 0, i);
      this->ChartTable->SetValue(i, 1, transformedcroshairPosition_RAS[0]-croshairPosition_RAS[0]);
      this->ChartTable->SetValue(i, 2, transformedcroshairPosition_RAS[1]-croshairPosition_RAS[1]);
      this->ChartTable->SetValue(i, 3, transformedcroshairPosition_RAS[2]-croshairPosition_RAS[2]);
    }
    //this->ChartTable->Update();
    this->ChartXY->RemovePlot(0);
    this->ChartXY->RemovePlot(0);
    this->ChartXY->RemovePlot(0);

    this->ChartXY->GetAxis(0)->SetTitle("Displacement");
    this->ChartXY->GetAxis(1)->SetTitle("Time");
    vtkPlot* lineX = this->ChartXY->AddPlot(vtkChart::LINE);
    vtkPlot* lineY = this->ChartXY->AddPlot(vtkChart::LINE);
    vtkPlot* lineZ = this->ChartXY->AddPlot(vtkChart::LINE);

    lineX->SetInputData(this->ChartTable, 0, 1);
    lineY->SetInputData(this->ChartTable, 0, 2);
    lineZ->SetInputData(this->ChartTable, 0, 3);

    lineX->SetColor(255,0,0,255);
    lineY->SetColor(0,255,0,255);
    lineZ->SetColor(0,0,255,255);
  }
}

//-----------------------------------------------------------------------------
// qSlicerSequenceBrowserModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerSequenceBrowserModuleWidget::qSlicerSequenceBrowserModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerSequenceBrowserModuleWidgetPrivate(*this) )
{
  Q_D(qSlicerSequenceBrowserModuleWidget);
}

//-----------------------------------------------------------------------------
qSlicerSequenceBrowserModuleWidget::~qSlicerSequenceBrowserModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerSequenceBrowserModuleWidget);

  if (this->mrmlScene() == scene)
    {
    return;
    }

  this->Superclass::setMRMLScene(scene);
  d->setAndObserveCrosshairNode();
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::setup()
{
  Q_D(qSlicerSequenceBrowserModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  d->init();

  connect(d->MRMLNodeComboBox_ActiveBrowser, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(activeBrowserNodeChanged(vtkMRMLNode*)));
  connect(d->MRMLNodeComboBox_MasterSequence, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(sequenceNodeChanged(vtkMRMLNode*)));
  connect(d->checkBox_PlaybackItemSkippingEnabled, SIGNAL(toggled(bool)), this, SLOT(playbackItemSkippingEnabledChanged(bool)));
  connect(d->checkBox_RecordMasterOnly, SIGNAL(toggled(bool)), this, SLOT(recordMasterOnlyChanged(bool)));
  connect(d->comboBox_RecordingSamplingMode, SIGNAL(currentIndexChanged(int)), this, SLOT(recordingSamplingModeChanged(int)));
  connect(d->comboBox_IndexDisplayMode, SIGNAL(currentIndexChanged(int)), this, SLOT(indexDisplayModeChanged(int)));
  connect(d->spinBox_IndexDisplayDecimals, SIGNAL(valueChanged(int)), this, SLOT(indexDisplayDecimalsChanged(int)));
  connect(d->pushButton_AddSequenceNode, SIGNAL(clicked()), this, SLOT(onAddSequenceNodeButtonClicked()));
  connect(d->pushButton_RemoveSequenceNode, SIGNAL(clicked()), this, SLOT(onRemoveSequenceNodesButtonClicked()));
  d->pushButton_AddSequenceNode->setIcon( QIcon(":/Icons/Add.png" ));
  d->pushButton_RemoveSequenceNode->setIcon(QIcon(":/Icons/Remove.png"));


  qMRMLSequenceBrowserToolBar* toolBar = d->toolBar();
  if (toolBar)
  {
    connect( toolBar, SIGNAL(activeBrowserNodeChanged(vtkMRMLNode*)), this, SLOT(activeBrowserNodeChanged(vtkMRMLNode*)) );
  }

  QHeaderView* tableWidget_SynchronizedSequenceNodes_HeaderView = d->tableWidget_SynchronizedSequenceNodes->horizontalHeader();

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
  tableWidget_SynchronizedSequenceNodes_HeaderView->setResizeMode(SYNCH_NODES_NAME_COLUMN, QHeaderView::Interactive);
  tableWidget_SynchronizedSequenceNodes_HeaderView->setResizeMode(SYNCH_NODES_PROXY_COLUMN, QHeaderView::Interactive);
  tableWidget_SynchronizedSequenceNodes_HeaderView->setResizeMode(SYNCH_NODES_PLAYBACK_COLUMN, QHeaderView::ResizeToContents);
  tableWidget_SynchronizedSequenceNodes_HeaderView->setResizeMode(SYNCH_NODES_RECORDING_COLUMN, QHeaderView::ResizeToContents);
  tableWidget_SynchronizedSequenceNodes_HeaderView->setResizeMode(SYNCH_NODES_OVERWRITE_PROXY_NAME_COLUMN, QHeaderView::ResizeToContents);
  tableWidget_SynchronizedSequenceNodes_HeaderView->setResizeMode(SYNCH_NODES_SAVE_CHANGES_COLUMN, QHeaderView::ResizeToContents);
#else
  tableWidget_SynchronizedSequenceNodes_HeaderView-> setSectionResizeMode(SYNCH_NODES_NAME_COLUMN, QHeaderView::Interactive);
  tableWidget_SynchronizedSequenceNodes_HeaderView-> setSectionResizeMode(SYNCH_NODES_PROXY_COLUMN, QHeaderView::Interactive);
  tableWidget_SynchronizedSequenceNodes_HeaderView-> setSectionResizeMode(SYNCH_NODES_PLAYBACK_COLUMN, QHeaderView::ResizeToContents);
  tableWidget_SynchronizedSequenceNodes_HeaderView-> setSectionResizeMode(SYNCH_NODES_RECORDING_COLUMN, QHeaderView::ResizeToContents);
  tableWidget_SynchronizedSequenceNodes_HeaderView-> setSectionResizeMode(SYNCH_NODES_OVERWRITE_PROXY_NAME_COLUMN, QHeaderView::ResizeToContents);
  tableWidget_SynchronizedSequenceNodes_HeaderView-> setSectionResizeMode(SYNCH_NODES_SAVE_CHANGES_COLUMN, QHeaderView::ResizeToContents);
#endif

  tableWidget_SynchronizedSequenceNodes_HeaderView->setStretchLastSection(false);

  d->tableWidget_SynchronizedSequenceNodes->setColumnWidth(SYNCH_NODES_NAME_COLUMN, 200);
  d->tableWidget_SynchronizedSequenceNodes->setColumnWidth(SYNCH_NODES_PROXY_COLUMN, 200);
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::enter()
{
  Q_D(qSlicerSequenceBrowserModuleWidget);

  if (this->mrmlScene() != NULL)
  {
    // set up mrml scene observations so that the GUI gets updated
    this->qvtkConnect(this->mrmlScene(), vtkMRMLScene::NodeAddedEvent,
      this, SLOT(onNodeAddedEvent(vtkObject*, vtkObject*)));
    this->qvtkConnect(this->mrmlScene(), vtkMRMLScene::NodeRemovedEvent,
      this, SLOT(onNodeRemovedEvent(vtkObject*, vtkObject*)));
    this->qvtkConnect(this->mrmlScene(), vtkMRMLScene::EndImportEvent,
      this, SLOT(onMRMLSceneEndImportEvent()));
    this->qvtkConnect(this->mrmlScene(), vtkMRMLScene::EndBatchProcessEvent,
      this, SLOT(onMRMLSceneEndBatchProcessEvent()));
    this->qvtkConnect(this->mrmlScene(), vtkMRMLScene::EndCloseEvent,
      this, SLOT(onMRMLSceneEndCloseEvent()));
    this->qvtkConnect(this->mrmlScene(), vtkMRMLScene::EndRestoreEvent,
      this, SLOT(onMRMLSceneEndRestoreEvent()));

    // For the user's convenience, create a browser node by default, when entering to the module and no browser node exists in the scene yet
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLSequenceBrowserNode");
    if (node == NULL)
    {
      vtkSmartPointer<vtkMRMLSequenceBrowserNode> newBrowserNode = vtkSmartPointer<vtkMRMLSequenceBrowserNode>::New();
      this->mrmlScene()->AddNode(newBrowserNode);
      this->activeBrowserNodeChanged(newBrowserNode);
    }
    else
    {
      this->activeBrowserNodeChanged(d->MRMLNodeComboBox_ActiveBrowser->currentNode());
    }
  }
  else
  {
    qCritical() << "Entering the SequenceBrowser module failed, scene is invalid";
  }

  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::onNodeAddedEvent(vtkObject* scene, vtkObject* node)
{
  Q_UNUSED(scene);
  Q_UNUSED(node);
  if (!this->mrmlScene() || this->mrmlScene()->IsBatchProcessing())
  {
    return;
  }
  this->refreshSynchronizedSequenceNodesTable();
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::onNodeRemovedEvent(vtkObject* scene, vtkObject* node)
{
  Q_UNUSED(scene);
  Q_UNUSED(node);
  if (!this->mrmlScene() || this->mrmlScene()->IsBatchProcessing())
    {
    return;
    }
  this->refreshSynchronizedSequenceNodesTable();
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::onMRMLSceneEndImportEvent()
{
  this->refreshSynchronizedSequenceNodesTable();
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::onMRMLSceneEndRestoreEvent()
{
  this->refreshSynchronizedSequenceNodesTable();
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::onMRMLSceneEndBatchProcessEvent()
{
  if (!this->mrmlScene())
    {
    return;
    }
  this->refreshSynchronizedSequenceNodesTable();
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::onMRMLSceneEndCloseEvent()
{
  if (!this->mrmlScene() || this->mrmlScene()->IsBatchProcessing())
    {
    return;
    }
  this->refreshSynchronizedSequenceNodesTable();
}


//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::exit()
{
  this->Superclass::exit();
  this->qvtkDisconnectAll();
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::activeBrowserNodeChanged(vtkMRMLNode* node)
{
  Q_D(qSlicerSequenceBrowserModuleWidget);
  vtkMRMLSequenceBrowserNode* browserNode = vtkMRMLSequenceBrowserNode::SafeDownCast(node);
  this->setActiveBrowserNode(browserNode);
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::sequenceNodeChanged(vtkMRMLNode* inputNode)
{
  vtkMRMLSequenceNode* sequenceNode = vtkMRMLSequenceNode::SafeDownCast(inputNode);
  this->setMasterSequenceNode(sequenceNode);
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::playbackItemSkippingEnabledChanged(bool enabled)
{
  Q_D(qSlicerSequenceBrowserModuleWidget);
  if (d->ActiveBrowserNode == NULL)
  {
    return; // no active node, nothing to update
  }
  d->ActiveBrowserNode->SetPlaybackItemSkippingEnabled(enabled);
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::recordMasterOnlyChanged(bool enabled)
{
  Q_D(qSlicerSequenceBrowserModuleWidget);
  if (d->ActiveBrowserNode == NULL)
  {
    return; // no active node, nothing to update
  }
  d->ActiveBrowserNode->SetRecordMasterOnly(enabled);
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::recordingSamplingModeChanged(int index)
{
  Q_D(qSlicerSequenceBrowserModuleWidget);
  if (d->ActiveBrowserNode == NULL)
  {
    return; // no active node, nothing to update
  }
  d->ActiveBrowserNode->SetRecordingSamplingMode(index);
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::indexDisplayModeChanged(int index)
{
  Q_D(qSlicerSequenceBrowserModuleWidget);
  if (d->ActiveBrowserNode == NULL)
  {
    return; // no active node, nothing to update
  }
  d->ActiveBrowserNode->SetIndexDisplayMode(index);
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::indexDisplayDecimalsChanged(int decimals)
{
  Q_D(qSlicerSequenceBrowserModuleWidget);
  if (d->ActiveBrowserNode == NULL)
  {
    return; // no active node, nothing to update
  }
  d->ActiveBrowserNode->SetIndexDisplayDecimals(decimals);
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::onActiveBrowserNodeModified(vtkObject* caller)
{
  Q_UNUSED(caller);
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::onMRMLInputSequenceInputNodeModified(vtkObject* inputNode)
{
  Q_UNUSED(inputNode);
  this->updateWidgetFromMRML();
}

// --------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::setActiveBrowserNode(vtkMRMLSequenceBrowserNode* browserNode)
{
  Q_D(qSlicerSequenceBrowserModuleWidget);
  if (!this->mrmlScene() || this->mrmlScene()->IsBatchProcessing())
  {
    return;
  }
  if (d->ActiveBrowserNode == browserNode
    && browserNode != NULL) // always update if broserNode is NULL (needed for proper update during scene close)
  {
    // no change
    return;
  }

  this->qvtkReconnect(d->ActiveBrowserNode, browserNode, vtkCommand::ModifiedEvent,
    this, SLOT(onActiveBrowserNodeModified(vtkObject*)));

  d->ActiveBrowserNode = browserNode;

  d->MRMLNodeComboBox_ActiveBrowser->setCurrentNode(browserNode);
  d->sequenceBrowserPlayWidget->setMRMLSequenceBrowserNode(browserNode);
  d->sequenceBrowserSeekWidget->setMRMLSequenceBrowserNode(browserNode);
  qMRMLSequenceBrowserToolBar* toolBar = d->toolBar();
  if (toolBar)
  {
    toolBar->setActiveBrowserNode(browserNode);
  }

  this->updateWidgetFromMRML();
}

// --------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::setMasterSequenceNode(vtkMRMLSequenceNode* sequenceNode)
{
  Q_D(qSlicerSequenceBrowserModuleWidget);
  if (d->ActiveBrowserNode==NULL)
  {
    // this happens when entering the module (the node selector already finds a suitable sequence node so it selects it, but
    // no browser node is selected yet)
    this->updateWidgetFromMRML();
    return;
  }
  if (sequenceNode!=d->ActiveBrowserNode->GetMasterSequenceNode())
  {
    bool oldModify=d->ActiveBrowserNode->StartModify();

    // Reconnect the input node's Modified() event observer
    this->qvtkReconnect(d->ActiveBrowserNode->GetMasterSequenceNode(), sequenceNode, vtkCommand::ModifiedEvent,
      this, SLOT(onMRMLInputSequenceInputNodeModified(vtkObject*)));

    char* sequenceNodeId = sequenceNode==NULL ? NULL : sequenceNode->GetID();

    d->ActiveBrowserNode->SetAndObserveMasterSequenceNodeID(sequenceNodeId);

    // Update d->ActiveBrowserNode->SetAndObserveSelectedSequenceNodeID
    if (sequenceNode!=NULL && sequenceNode->GetNumberOfDataNodes()>0)
    {
      d->ActiveBrowserNode->SetSelectedItemNumber(0);
    }
    else
    {
      d->ActiveBrowserNode->SetSelectedItemNumber(-1);
    }

    d->ActiveBrowserNode->EndModify(oldModify);
  }
}

// --------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::onAddSequenceNodeButtonClicked()
{
  Q_D(qSlicerSequenceBrowserModuleWidget);
  vtkMRMLSequenceBrowserNode* browserNode = vtkMRMLSequenceBrowserNode::SafeDownCast(d->MRMLNodeComboBox_ActiveBrowser->currentNode());
  if (!browserNode)
  {
    qWarning() << Q_FUNC_INFO << " failed: no browser node is selected";
    return;
  }
  vtkMRMLSequenceNode* sequenceNode = vtkMRMLSequenceNode::SafeDownCast(d->MRMLNodeComboBox_SynchronizeSequenceNode->currentNode());
  vtkMRMLSequenceNode* addedSequenceNode = d->logic()->AddSynchronizedNode(sequenceNode, NULL, browserNode);
  if (addedSequenceNode)
  {
    if (browserNode->GetNumberOfSynchronizedSequenceNodes() == 0)
    {
      // master node is added - if it's a new (empty) sequence node, enable recording by default
      if (sequenceNode == NULL)
      {
        browserNode->SetRecording(addedSequenceNode, true);
      }
    }
    else
    {
      // synchronized node is added - copy the recording setting from the master sequence node
      browserNode->SetRecording(addedSequenceNode, browserNode->GetRecording(browserNode->GetMasterSequenceNode()));
    }
  }
}

// --------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::onRemoveSequenceNodesButtonClicked()
{
  Q_D(qSlicerSequenceBrowserModuleWidget);
  // First, grab all of the selected rows
  QModelIndexList modelIndexList = d->tableWidget_SynchronizedSequenceNodes->selectionModel()->selectedIndexes();
  std::vector<std::string> selectedSequenceIDs;
  for (QModelIndexList::iterator index = modelIndexList.begin(); index!=modelIndexList.end(); index++)
  {
    QWidget* proxyNodeComboBox = d->tableWidget_SynchronizedSequenceNodes->cellWidget((*index).row(), SYNCH_NODES_PROXY_COLUMN);
    std::string currSelectedSequenceID = proxyNodeComboBox->property("MRMLNodeID").toString().toLatin1().constData();
	selectedSequenceIDs.push_back(currSelectedSequenceID);
    disconnect(proxyNodeComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(onProxyNodeChanged(vtkMRMLNode*))); // No need to reconnect - the entire row is going to be removed
  }
  // Now, use the MRML ID stored by the proxy node combo box to determine the sequence nodes to remove from the browser
  std::vector<std::string>::iterator sequenceIDItr;
  for (sequenceIDItr = selectedSequenceIDs.begin(); sequenceIDItr != selectedSequenceIDs.end(); sequenceIDItr++)
  {
    d->ActiveBrowserNode->RemoveSynchronizedSequenceNode((*sequenceIDItr).c_str());
  }
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerSequenceBrowserModuleWidget);

  d->SynchronizedBrowsingSection->setEnabled(d->ActiveBrowserNode != NULL);
  d->PlottingSection->setEnabled(d->ActiveBrowserNode != NULL);
  d->AdvancedSection->setEnabled(d->ActiveBrowserNode != NULL);

  if (d->ActiveBrowserNode==NULL)
  {
    this->refreshSynchronizedSequenceNodesTable();
    return;
  }

  vtkMRMLSequenceNode* sequenceNode = d->ActiveBrowserNode->GetMasterSequenceNode();

  bool wasBlocked = d->MRMLNodeComboBox_MasterSequence->blockSignals(true);
  d->MRMLNodeComboBox_MasterSequence->setCurrentNode(sequenceNode);
  d->MRMLNodeComboBox_MasterSequence->blockSignals(wasBlocked);

  wasBlocked = d->checkBox_PlaybackItemSkippingEnabled->blockSignals(true);
  d->checkBox_PlaybackItemSkippingEnabled->setChecked(d->ActiveBrowserNode->GetPlaybackItemSkippingEnabled());
  d->checkBox_PlaybackItemSkippingEnabled->blockSignals(wasBlocked);

  wasBlocked = d->checkBox_RecordMasterOnly->blockSignals(true);
  d->checkBox_RecordMasterOnly->setChecked(d->ActiveBrowserNode->GetRecordMasterOnly());
  d->checkBox_RecordMasterOnly->blockSignals(wasBlocked);

  wasBlocked = d->comboBox_RecordingSamplingMode->blockSignals(true);
  d->comboBox_RecordingSamplingMode->setCurrentIndex(d->ActiveBrowserNode->GetRecordingSamplingMode());
  d->comboBox_RecordingSamplingMode->blockSignals(wasBlocked);

  wasBlocked = d->comboBox_IndexDisplayMode->blockSignals(true);
  d->comboBox_IndexDisplayMode->setCurrentIndex(d->ActiveBrowserNode->GetIndexDisplayMode());
  d->comboBox_IndexDisplayMode->blockSignals(wasBlocked);

  wasBlocked = d->spinBox_IndexDisplayDecimals->blockSignals(true);
  d->spinBox_IndexDisplayDecimals->setValue(d->ActiveBrowserNode->GetIndexDisplayDecimals());
  d->spinBox_IndexDisplayDecimals->blockSignals(wasBlocked);

  this->refreshSynchronizedSequenceNodesTable();
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::refreshSynchronizedSequenceNodesTable()
{
  Q_D(qSlicerSequenceBrowserModuleWidget);

  if (d->ActiveBrowserNode != NULL &&
    (d->ActiveBrowserNode->GetRecordingActive() || d->ActiveBrowserNode->GetPlaybackActive()))
  {
    // this is an expensive operation, we cannot afford to do it while recording or replaying
    // TODO: make this update method much more efficient
    return;
  }


  // Clear the table
  for (int row=0; row<d->tableWidget_SynchronizedSequenceNodes->rowCount(); row++)
  {
    QCheckBox* playbackCheckbox = dynamic_cast<QCheckBox*>(d->tableWidget_SynchronizedSequenceNodes->cellWidget(row, SYNCH_NODES_PLAYBACK_COLUMN));
    disconnect(playbackCheckbox, SIGNAL(stateChanged(int)), this, SLOT(synchronizedSequenceNodePlaybackStateChanged(int)));
    QCheckBox* recordingCheckbox = dynamic_cast<QCheckBox*>(d->tableWidget_SynchronizedSequenceNodes->cellWidget(row, SYNCH_NODES_RECORDING_COLUMN));
    disconnect(recordingCheckbox, SIGNAL(stateChanged(int)), this, SLOT(synchronizedSequenceNodeRecordingStateChanged(int)));
    QCheckBox* overwriteProxyNameCheckbox = dynamic_cast<QCheckBox*>(d->tableWidget_SynchronizedSequenceNodes->cellWidget(row, SYNCH_NODES_OVERWRITE_PROXY_NAME_COLUMN));
    disconnect(overwriteProxyNameCheckbox, SIGNAL(stateChanged(int)), this, SLOT(synchronizedSequenceNodeOverwriteProxyNameStateChanged(int)));
    QCheckBox* saveChangesCheckbox = dynamic_cast<QCheckBox*>(d->tableWidget_SynchronizedSequenceNodes->cellWidget(row, SYNCH_NODES_SAVE_CHANGES_COLUMN));
    disconnect(saveChangesCheckbox, SIGNAL(stateChanged(int)), this, SLOT(synchronizedSequenceNodeSaveChangesStateChanged(int)));
    qMRMLNodeComboBox* proxyNodeComboBox = dynamic_cast<qMRMLNodeComboBox*>(d->tableWidget_SynchronizedSequenceNodes->cellWidget(row, SYNCH_NODES_PROXY_COLUMN));
    disconnect(proxyNodeComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(onProxyNodeChanged(vtkMRMLNode*)));
  }

  if (d->ActiveBrowserNode==NULL)
  {
    d->tableWidget_SynchronizedSequenceNodes->setRowCount(0); // clear() would not actually remove the rows
    return;
  }
  // A valid active browser node is selected
  vtkMRMLSequenceNode* sequenceNode = d->ActiveBrowserNode->GetMasterSequenceNode();
  if (sequenceNode==NULL)
  {
    d->tableWidget_SynchronizedSequenceNodes->setRowCount(0); // clear() would not actually remove the rows
    return;
  }

  disconnect(d->tableWidget_SynchronizedSequenceNodes, SIGNAL(cellChanged(int, int)), this, SLOT(sequenceNodeNameEdited(int, int)));

  vtkNew<vtkCollection> syncedNodes;
  d->ActiveBrowserNode->GetSynchronizedSequenceNodes(syncedNodes.GetPointer(), true);
  d->tableWidget_SynchronizedSequenceNodes->setRowCount(syncedNodes->GetNumberOfItems()); // +1 because we add the master as well

  QStringList supportedProxyNodeTypes;
  const std::vector<std::string> & supportedProxyNodeTypesStd = vtkMRMLNodeSequencer::GetInstance()->GetSupportedNodeClassNames();
  for (std::vector<std::string>::const_iterator it = supportedProxyNodeTypesStd.begin(); it != supportedProxyNodeTypesStd.end(); ++it)
  {
    supportedProxyNodeTypes << it->c_str();
  }

  // Create line for the compatible nodes
  for (int i=0; i<syncedNodes->GetNumberOfItems(); ++i)
  {
    vtkMRMLSequenceNode* syncedNode = vtkMRMLSequenceNode::SafeDownCast( syncedNodes->GetItemAsObject(i) );
    if (!syncedNode)
    {
      continue;
    }

    QTableWidgetItem* verticalHeaderItem = new QTableWidgetItem();
    if (!strcmp(syncedNode->GetID(), d->ActiveBrowserNode->GetMasterSequenceNode()->GetID()))
    {
      verticalHeaderItem->setText("M");
      verticalHeaderItem->setToolTip("Master sequence");
    }
    else
    {
      verticalHeaderItem->setText(QString::number(i));
      verticalHeaderItem->setToolTip("Synchronized sequence");
    }
    d->tableWidget_SynchronizedSequenceNodes->setVerticalHeaderItem(i, verticalHeaderItem);

    // Create checkboxes
    QCheckBox* playbackCheckbox = new QCheckBox(d->tableWidget_SynchronizedSequenceNodes);
    playbackCheckbox->setToolTip(tr("Include this node in synchronized playback"));
    playbackCheckbox->setProperty("MRMLNodeID", QString(syncedNode->GetID()));

    QCheckBox* overwriteProxyNameCheckbox = new QCheckBox(d->tableWidget_SynchronizedSequenceNodes);
    overwriteProxyNameCheckbox->setToolTip(tr("Overwrite the associated node's name during playback"));
    overwriteProxyNameCheckbox->setProperty("MRMLNodeID", QString(syncedNode->GetID()));

    QCheckBox* saveChangesCheckbox = new QCheckBox(d->tableWidget_SynchronizedSequenceNodes);
    saveChangesCheckbox->setToolTip(tr("Save changes to the node into the sequence"));
    saveChangesCheckbox->setProperty("MRMLNodeID", QString(syncedNode->GetID()));

    QCheckBox* recordingCheckbox = new QCheckBox(d->tableWidget_SynchronizedSequenceNodes);
    recordingCheckbox->setToolTip(tr("Include this node in synchronized recording"));
    recordingCheckbox->setProperty("MRMLNodeID", QString(syncedNode->GetID()));

    // Set previous checked state of the checkbox
    bool playbackChecked = d->ActiveBrowserNode->GetPlayback(syncedNode);
    playbackCheckbox->setChecked(playbackChecked);

    bool overwriteProxyNameChecked = d->ActiveBrowserNode->GetOverwriteProxyName(syncedNode);
    overwriteProxyNameCheckbox->setChecked(overwriteProxyNameChecked);

    bool saveChangesChecked = d->ActiveBrowserNode->GetSaveChanges(syncedNode);
    saveChangesCheckbox->setChecked(saveChangesChecked);

    bool recordingChecked = d->ActiveBrowserNode->GetRecording(syncedNode);
    recordingCheckbox->setChecked(recordingChecked);

    connect(playbackCheckbox, SIGNAL(stateChanged(int)), this, SLOT(synchronizedSequenceNodePlaybackStateChanged(int)));
    connect(overwriteProxyNameCheckbox, SIGNAL(stateChanged(int)), this, SLOT(synchronizedSequenceNodeOverwriteProxyNameStateChanged(int)));
    connect(saveChangesCheckbox, SIGNAL(stateChanged(int)), this, SLOT(synchronizedSequenceNodeSaveChangesStateChanged(int)));
    connect(recordingCheckbox, SIGNAL(stateChanged(int)), this, SLOT(synchronizedSequenceNodeRecordingStateChanged(int)));

    d->tableWidget_SynchronizedSequenceNodes->setCellWidget(i, SYNCH_NODES_PLAYBACK_COLUMN, playbackCheckbox);
    d->tableWidget_SynchronizedSequenceNodes->setCellWidget(i, SYNCH_NODES_RECORDING_COLUMN, recordingCheckbox);
    d->tableWidget_SynchronizedSequenceNodes->setCellWidget(i, SYNCH_NODES_OVERWRITE_PROXY_NAME_COLUMN, overwriteProxyNameCheckbox);
    d->tableWidget_SynchronizedSequenceNodes->setCellWidget(i, SYNCH_NODES_SAVE_CHANGES_COLUMN, saveChangesCheckbox);

    QTableWidgetItem* nameItem = new QTableWidgetItem( QString(syncedNode->GetName()) );
    d->tableWidget_SynchronizedSequenceNodes->setItem(i, SYNCH_NODES_NAME_COLUMN, nameItem);

    vtkMRMLNode* proxyNode = d->ActiveBrowserNode->GetProxyNode(syncedNode);
    qMRMLNodeComboBox* proxyNodeComboBox = new qMRMLNodeComboBox();
    if (!syncedNode->GetDataNodeClassName().empty())
    {
      proxyNodeComboBox->setNodeTypes(QStringList() << syncedNode->GetDataNodeClassName().c_str());
    }
    else
    {
      proxyNodeComboBox->setNodeTypes(supportedProxyNodeTypes);
    }
    proxyNodeComboBox->setAddEnabled(false);
    proxyNodeComboBox->setNoneEnabled(true);
    proxyNodeComboBox->setRemoveEnabled(true);
    proxyNodeComboBox->setRenameEnabled(true);
    proxyNodeComboBox->setShowChildNodeTypes(true);
    proxyNodeComboBox->setShowHidden(true); // display nodes are hidden by default
    proxyNodeComboBox->setMRMLScene(this->mrmlScene());
    proxyNodeComboBox->setCurrentNode(proxyNode);
    proxyNodeComboBox->setProperty("MRMLNodeID", QString(syncedNode->GetID()));
    d->tableWidget_SynchronizedSequenceNodes->setCellWidget(i, SYNCH_NODES_PROXY_COLUMN, proxyNodeComboBox);

    connect(proxyNodeComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(onProxyNodeChanged(vtkMRMLNode*)));
  }

  connect(d->tableWidget_SynchronizedSequenceNodes, SIGNAL(cellChanged(int, int)), this, SLOT(sequenceNodeNameEdited(int, int)));
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::sequenceNodeNameEdited(int row, int column)
{
  Q_D(qSlicerSequenceBrowserModuleWidget);

  if (d->ActiveBrowserNode==NULL)
  {
    qCritical() << "qSlicerSequenceBrowserModuleWidget::synchronizedSequenceNodePlaybackStateChanged: Invalid activeBrowserNode";
    return;
  }
  if (column!=SYNCH_NODES_NAME_COLUMN)
  {
    return;
  }

  std::string newSequenceNodeName = d->tableWidget_SynchronizedSequenceNodes->item(row, column)->text().toStdString();

  QWidget* proxyNodeComboBox = d->tableWidget_SynchronizedSequenceNodes->cellWidget(row, SYNCH_NODES_PROXY_COLUMN);
  std::string synchronizedNodeID = proxyNodeComboBox->property("MRMLNodeID").toString().toLatin1().constData();
  vtkMRMLSequenceNode* synchronizedNode = vtkMRMLSequenceNode::SafeDownCast( this->mrmlScene()->GetNodeByID(synchronizedNodeID) );

  synchronizedNode->SetName(newSequenceNodeName.c_str());
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::synchronizedSequenceNodePlaybackStateChanged(int aState)
{
  Q_D(qSlicerSequenceBrowserModuleWidget);

  if (d->ActiveBrowserNode==NULL)
  {
    qCritical() << "qSlicerSequenceBrowserModuleWidget::synchronizedSequenceNodePlaybackStateChanged: Invalid activeBrowserNode";
    return;
  }

  QCheckBox* senderCheckbox = dynamic_cast<QCheckBox*>(sender());
  if (!senderCheckbox)
  {
    qCritical() << "qSlicerSequenceBrowserModuleWidget::synchronizedSequenceNodePlaybackStateChanged: Invalid sender checkbox";
    return;
  }

  std::string synchronizedNodeID = senderCheckbox->property("MRMLNodeID").toString().toLatin1().constData();
  vtkMRMLSequenceNode* synchronizedNode = vtkMRMLSequenceNode::SafeDownCast( this->mrmlScene()->GetNodeByID(synchronizedNodeID) );
  d->ActiveBrowserNode->SetPlayback(synchronizedNode, aState);
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::synchronizedSequenceNodeRecordingStateChanged(int aState)
{
  Q_D(qSlicerSequenceBrowserModuleWidget);

  if (d->ActiveBrowserNode==NULL)
  {
    qCritical() << "qSlicerSequenceBrowserModuleWidget::synchronizedSequenceNodeRecordingStateChanged: Invalid activeBrowserNode";
    return;
  }

  QCheckBox* senderCheckbox = dynamic_cast<QCheckBox*>(sender());
  if (!senderCheckbox)
  {
    qCritical() << "qSlicerSequenceBrowserModuleWidget::synchronizedSequenceNodeRecordingStateChanged: Invalid sender checkbox";
    return;
  }

  std::string synchronizedNodeID = senderCheckbox->property("MRMLNodeID").toString().toLatin1().constData();
  vtkMRMLSequenceNode* synchronizedNode = vtkMRMLSequenceNode::SafeDownCast( this->mrmlScene()->GetNodeByID(synchronizedNodeID) );
  d->ActiveBrowserNode->SetRecording(synchronizedNode, aState);
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::synchronizedSequenceNodeOverwriteProxyNameStateChanged(int aState)
{
  Q_D(qSlicerSequenceBrowserModuleWidget);

  if (d->ActiveBrowserNode==NULL)
  {
    qCritical() << "qSlicerSequenceBrowserModuleWidget::synchronizedSequenceNodeOverwriteProxyNameStateChanged: Invalid activeBrowserNode";
    return;
  }

  QCheckBox* senderCheckbox = dynamic_cast<QCheckBox*>(sender());
  if (!senderCheckbox)
  {
    qCritical() << "qSlicerSequenceBrowserModuleWidget::synchronizedSequenceNodeOverwriteProxyNameStateChanged: Invalid sender checkbox";
    return;
  }

  std::string synchronizedNodeID = senderCheckbox->property("MRMLNodeID").toString().toLatin1().constData();
  vtkMRMLSequenceNode* synchronizedNode = vtkMRMLSequenceNode::SafeDownCast( this->mrmlScene()->GetNodeByID(synchronizedNodeID) );
  d->ActiveBrowserNode->SetOverwriteProxyName(synchronizedNode, aState);
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::synchronizedSequenceNodeSaveChangesStateChanged(int aState)
{
  Q_D(qSlicerSequenceBrowserModuleWidget);

  if (d->ActiveBrowserNode==NULL)
  {
    qCritical() << "qSlicerSequenceBrowserModuleWidget::synchronizedSequenceNodeSaveChangesStateChanged: Invalid activeBrowserNode";
    return;
  }

  QCheckBox* senderCheckbox = dynamic_cast<QCheckBox*>(sender());
  if (!senderCheckbox)
  {
    qCritical() << "qSlicerSequenceBrowserModuleWidget::synchronizedSequenceNodeSaveChangesStateChanged: Invalid sender checkbox";
    return;
  }

  std::string synchronizedNodeID = senderCheckbox->property("MRMLNodeID").toString().toLatin1().constData();
  vtkMRMLSequenceNode* synchronizedNode = vtkMRMLSequenceNode::SafeDownCast( this->mrmlScene()->GetNodeByID(synchronizedNodeID) );
  d->ActiveBrowserNode->SetSaveChanges(synchronizedNode, aState);
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::onProxyNodeChanged(vtkMRMLNode* newProxyNode)
{
  Q_D(qSlicerSequenceBrowserModuleWidget);

  if (d->ActiveBrowserNode==NULL)
  {
    qCritical() << "qSlicerSequenceBrowserModuleWidget::onProxyNodeChanged: Invalid activeBrowserNode";
    return;
  }

  qMRMLNodeComboBox* senderComboBox = dynamic_cast<qMRMLNodeComboBox*>(sender());
  if (!senderComboBox)
  {
    qCritical() << "qSlicerSequenceBrowserModuleWidget::onProxyNodeChanged: Invalid sender checkbox";
    return;
  }

  std::string synchronizedNodeID = senderComboBox->property("MRMLNodeID").toString().toLatin1().constData();
  vtkMRMLSequenceNode* synchronizedNode = vtkMRMLSequenceNode::SafeDownCast( this->mrmlScene()->GetNodeByID(synchronizedNodeID) );

  // If name sync is enabled between sequence and proxy node then update the sequence node name based on the proxy node
  if (newProxyNode && newProxyNode->GetName() != NULL
    && synchronizedNode && d->ActiveBrowserNode->GetOverwriteProxyName(synchronizedNode))
  {
    std::string baseName = "Data";
    if (newProxyNode->GetAttribute("Sequences.BaseName") != 0)
    {
      baseName = newProxyNode->GetAttribute("Sequences.BaseName");
    }
    else if (newProxyNode->GetName() != 0)
    {
      baseName = newProxyNode->GetName();
    }
    baseName += " sequence";
    std::string proxyNodeName = this->mrmlScene()->GetUniqueNameByString(baseName.c_str());
    synchronizedNode->SetName(proxyNodeName.c_str());
  }

  d->logic()->AddSynchronizedNode(synchronizedNode, newProxyNode, d->ActiveBrowserNode);
}

//------------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::updateChart()
{
  Q_D(qSlicerSequenceBrowserModuleWidget);
  if (  d->pushButton_iCharting->isChecked())
  {
    d->updateInteractiveCharting();
  }
}
