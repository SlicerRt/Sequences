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
#include <vtkNew.h>
#include <vtkPlot.h>
#include <vtkTable.h>
#include <vtkMatrix4x4.h>

enum
{
  SYNCH_NODES_STATUS_COLUMN=0,
  SYNCH_NODES_NAME_COLUMN,
  SYNCH_NODES_PROXY_COLUMN,
  SYNCH_NODES_PLAYBACK_COLUMN,
  SYNCH_NODES_RECORDING_COLUMN,
  SYNCH_NODES_OVERWRITE_PROXY_NAME_COLUMN,
  SYNCH_NODES_NUMBER_OF_COLUMNS // this must be the last line in this enum
};
static const char* MASTER_NODE_STATUS_STRING = "M";

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
  vtkMRMLSequenceBrowserNode* activeBrowserNode() const;

  void init();
  void resetInteractiveCharting();
  void updateInteractiveCharting();
  void setAndObserveCrosshairNode();

  qMRMLSequenceBrowserToolBar* toolBar();

  /// Using this flag prevents overriding the parameter set node contents when the
  ///   QMRMLCombobox selects the first instance of the specified node type when initializing
  bool ModuleWindowInitialized;

  std::string ActiveBrowserNodeID;

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
vtkMRMLSequenceBrowserNode* qSlicerSequenceBrowserModuleWidgetPrivate::activeBrowserNode() const
{
  Q_Q(const qSlicerSequenceBrowserModuleWidget);
  if (ActiveBrowserNodeID.empty())
  {
    return NULL;
  }
  vtkMRMLScene* scene=q->mrmlScene();
  if (scene==0)
  {
    return NULL;
  }
  vtkMRMLSequenceBrowserNode* node=vtkMRMLSequenceBrowserNode::SafeDownCast(scene->GetNodeByID(ActiveBrowserNodeID.c_str()));
  return node;
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
  vtkMRMLSequenceNode* sequenceNode = this->activeBrowserNode() ? this->activeBrowserNode()->GetMasterSequenceNode() : NULL;
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
  vtkMRMLNode* proxyNode = this->activeBrowserNode()->GetProxyNode(sequenceNode);
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

  connect( d->MRMLNodeComboBox_ActiveBrowser, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(activeBrowserNodeChanged(vtkMRMLNode*)) );
  connect( d->MRMLNodeComboBox_Sequence, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(sequenceNodeChanged(vtkMRMLNode*)) );
  connect( d->checkBox_PlaybackItemSkippingEnabled, SIGNAL(toggled(bool)), this, SLOT(playbackItemSkippingEnabledChanged(bool)) );
  connect( d->checkBox_RecordMasterOnly, SIGNAL(toggled(bool)), this, SLOT(recordMasterOnlyChanged(bool)) );
  connect( d->pushButton_AddSynchronizedNode, SIGNAL(clicked()), this, SLOT(onAddSynchronizedNodeButtonClicked()) );
  d->pushButton_AddSynchronizedNode->setIcon( QApplication::style()->standardIcon( QStyle::SP_ArrowUp ) );

  
  qMRMLSequenceBrowserToolBar* toolBar = d->toolBar();
  if (toolBar)
  {
    connect( toolBar, SIGNAL(activeBrowserNodeChanged(vtkMRMLNode*)), this, SLOT(activeBrowserNodeChanged(vtkMRMLNode*)) );
  }

  QHeaderView* tableWidget_SynchronizedSequenceNodes_HeaderView = d->tableWidget_SynchronizedSequenceNodes->horizontalHeader();
  tableWidget_SynchronizedSequenceNodes_HeaderView->setResizeMode(SYNCH_NODES_STATUS_COLUMN, QHeaderView::ResizeToContents);
  tableWidget_SynchronizedSequenceNodes_HeaderView->setResizeMode(SYNCH_NODES_NAME_COLUMN, QHeaderView::Interactive);
  tableWidget_SynchronizedSequenceNodes_HeaderView->setResizeMode(SYNCH_NODES_PROXY_COLUMN, QHeaderView::Interactive);
  tableWidget_SynchronizedSequenceNodes_HeaderView->setResizeMode(SYNCH_NODES_PLAYBACK_COLUMN, QHeaderView::ResizeToContents);
  tableWidget_SynchronizedSequenceNodes_HeaderView->setResizeMode(SYNCH_NODES_RECORDING_COLUMN, QHeaderView::ResizeToContents);
  tableWidget_SynchronizedSequenceNodes_HeaderView->setResizeMode(SYNCH_NODES_OVERWRITE_PROXY_NAME_COLUMN, QHeaderView::ResizeToContents);
  tableWidget_SynchronizedSequenceNodes_HeaderView->setStretchLastSection(false);

  d->tableWidget_SynchronizedSequenceNodes->setColumnWidth(SYNCH_NODES_NAME_COLUMN, 200);
  d->tableWidget_SynchronizedSequenceNodes->setColumnWidth(SYNCH_NODES_PROXY_COLUMN, 200);

  d->ExpandButton_SynchronizeNodes->setChecked(false);
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::enter()
{
  Q_D(qSlicerSequenceBrowserModuleWidget);

  if (this->mrmlScene() != 0)
  {
    // For the user's convenience, create a browser node by default, when entering to the module and no browser node exists in the scene yet
    vtkMRMLNode* node = this->mrmlScene()->GetNthNodeByClass(0, "vtkMRMLSequenceBrowserNode");
    if (node==NULL)
    {
      vtkSmartPointer<vtkMRMLSequenceBrowserNode> newBrowserNode = vtkSmartPointer<vtkMRMLSequenceBrowserNode>::New();
      this->mrmlScene()->AddNode(newBrowserNode);      
      setActiveBrowserNode(newBrowserNode);
    }
    // set up mrml scene observations so that the GUI gets updated
    this->qvtkReconnect(this->mrmlScene(), vtkMRMLScene::NodeAddedEvent,
      this, SLOT(onNodeAddedEvent(vtkObject*, vtkObject*)));
    this->qvtkReconnect(this->mrmlScene(), vtkMRMLScene::NodeRemovedEvent,
      this, SLOT(onNodeRemovedEvent(vtkObject*, vtkObject*)));
    this->qvtkReconnect(this->mrmlScene(), vtkMRMLScene::EndImportEvent,
      this, SLOT(onMRMLSceneEndImportEvent()));
    this->qvtkReconnect(this->mrmlScene(), vtkMRMLScene::EndBatchProcessEvent,
      this, SLOT(onMRMLSceneEndBatchProcessEvent()));
    this->qvtkReconnect(this->mrmlScene(), vtkMRMLScene::EndCloseEvent,
      this, SLOT(onMRMLSceneEndCloseEvent()));
    this->qvtkReconnect(this->mrmlScene(), vtkMRMLScene::EndRestoreEvent,
      this, SLOT(onMRMLSceneEndRestoreEvent()));
  }
  else
  {
    qCritical() << "Entering the SequenceBrowser module failed, scene is invalid";
  }

  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::onNodeAddedEvent(vtkObject*, vtkObject* node)
{
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

  // we do not remove mrml scene observations, because replay may run while the
  // module GUI is not shown
  // this->qvtkDisconnectAll();
} 

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::activeBrowserNodeChanged(vtkMRMLNode* node)
{
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
  if (d->activeBrowserNode() == NULL)
  {
    return; // no active node, nothing to update
  }
  d->activeBrowserNode()->SetPlaybackItemSkippingEnabled(enabled);
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::recordMasterOnlyChanged(bool enabled)
{
  Q_D(qSlicerSequenceBrowserModuleWidget);
  if (d->activeBrowserNode() == NULL)
  {
    return; // no active node, nothing to update
  }
  d->activeBrowserNode()->SetRecordMasterOnly(enabled);
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::onActiveBrowserNodeModified(vtkObject* caller)
{
  this->updateWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::onMRMLInputSequenceInputNodeModified(vtkObject* inputNode)
{
  this->updateWidgetFromMRML();  
}

// --------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::setActiveBrowserNode(vtkMRMLSequenceBrowserNode* browserNode)
{
  Q_D(qSlicerSequenceBrowserModuleWidget);
  if (d->activeBrowserNode()==browserNode)
  {
    return; // no change
  }
  if (d->activeBrowserNode()!=browserNode // simple change
    || (browserNode==NULL && !d->ActiveBrowserNodeID.empty()) ) // when the scene is closing activeBrowserNode() would return NULL and therefore ignore browserNode setting to 0
  { 
    // Reconnect the input node's Modified() event observer
    this->qvtkReconnect(d->activeBrowserNode(), browserNode, vtkCommand::ModifiedEvent,
      this, SLOT(onActiveBrowserNodeModified(vtkObject*)));
    d->ActiveBrowserNodeID = (browserNode?browserNode->GetID():"");
  }
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
  if (d->activeBrowserNode()==NULL)
  {
    // this happens when entering the module (the node selector already finds a suitable sequence node so it selects it, but
    // no browser node is selected yet)
    this->updateWidgetFromMRML();
    return;
  }
  if (sequenceNode!=d->activeBrowserNode()->GetMasterSequenceNode())
  {
    bool oldModify=d->activeBrowserNode()->StartModify();

    // Reconnect the input node's Modified() event observer
    this->qvtkReconnect(d->activeBrowserNode()->GetMasterSequenceNode(), sequenceNode, vtkCommand::ModifiedEvent,
      this, SLOT(onMRMLInputSequenceInputNodeModified(vtkObject*)));

    char* sequenceNodeId = sequenceNode==NULL ? NULL : sequenceNode->GetID();

    d->activeBrowserNode()->SetAndObserveMasterSequenceNodeID(sequenceNodeId);

    // Update d->activeBrowserNode()->SetAndObserveSelectedSequenceNodeID
    if (sequenceNode!=NULL && sequenceNode->GetNumberOfDataNodes()>0)
    {
      d->activeBrowserNode()->SetSelectedItemNumber(0);
    }
    else
    {
      d->activeBrowserNode()->SetSelectedItemNumber(-1);
    }

    d->activeBrowserNode()->EndModify(oldModify);
  }   
}

// --------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::onAddSynchronizedNodeButtonClicked()
{
  Q_D(qSlicerSequenceBrowserModuleWidget);
  d->logic()->AddSynchronizedNode(d->MRMLNodeComboBox_SynchronizeSequenceNode->currentNode(), d->MRMLNodeComboBox_SynchronizeVirtualNode->currentNode(), d->MRMLNodeComboBox_ActiveBrowser->currentNode());
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::updateWidgetFromMRML()
{
  Q_D(qSlicerSequenceBrowserModuleWidget);
  
  if (d->activeBrowserNode()==NULL)
  {
    d->MRMLNodeComboBox_Sequence->setEnabled(false);
    this->refreshSynchronizedSequenceNodesTable();
    return;
  }

  vtkMRMLSequenceNode* sequenceNode = d->activeBrowserNode()->GetMasterSequenceNode();  
  d->MRMLNodeComboBox_Sequence->setEnabled(true);
  d->MRMLNodeComboBox_Sequence->setCurrentNode(sequenceNode);
  
  d->checkBox_PlaybackItemSkippingEnabled->setChecked(d->activeBrowserNode()->GetPlaybackItemSkippingEnabled());
  d->checkBox_RecordMasterOnly->setChecked(d->activeBrowserNode()->GetRecordMasterOnly());

  this->refreshSynchronizedSequenceNodesTable();
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::refreshSynchronizedSequenceNodesTable()
{
  Q_D(qSlicerSequenceBrowserModuleWidget);

  // Clear the table
  for (int row=0; row<d->tableWidget_SynchronizedSequenceNodes->rowCount(); row++)
  {
    QCheckBox* playbackCheckbox = dynamic_cast<QCheckBox*>(d->tableWidget_SynchronizedSequenceNodes->cellWidget(row, SYNCH_NODES_PLAYBACK_COLUMN));
    disconnect(playbackCheckbox, SIGNAL(stateChanged(int)), this, SLOT(synchronizedSequenceNodePlaybackStateChanged(int)));
    QCheckBox* recordingCheckbox = dynamic_cast<QCheckBox*>(d->tableWidget_SynchronizedSequenceNodes->cellWidget(row, SYNCH_NODES_RECORDING_COLUMN));
    disconnect(recordingCheckbox, SIGNAL(stateChanged(int)), this, SLOT(synchronizedSequenceNodeRecordingStateChanged(int)));
    QCheckBox* overwriteProxyNameCheckbox = dynamic_cast<QCheckBox*>(d->tableWidget_SynchronizedSequenceNodes->cellWidget(row, SYNCH_NODES_OVERWRITE_PROXY_NAME_COLUMN));
    disconnect(overwriteProxyNameCheckbox, SIGNAL(stateChanged(int)), this, SLOT(synchronizedSequenceNodeOverwriteProxyNameStateChanged(int)));
  }

  if (d->activeBrowserNode()==NULL)
  {
    d->tableWidget_SynchronizedSequenceNodes->setRowCount(0); // clear() would not actually remove the rows
    return;
  }
  // A valid active browser node is selected
  vtkMRMLSequenceNode* sequenceNode = d->activeBrowserNode()->GetMasterSequenceNode();  
  if (sequenceNode==NULL)
  {
    d->tableWidget_SynchronizedSequenceNodes->setRowCount(0); // clear() would not actually remove the rows
    return;
  }

  vtkNew<vtkCollection> syncedNodes;
  d->activeBrowserNode()->GetSynchronizedSequenceNodes(syncedNodes.GetPointer(), true);
  d->tableWidget_SynchronizedSequenceNodes->setRowCount(syncedNodes->GetNumberOfItems()); // +1 because we add the master as well

  // Create line for the compatible nodes
  for (int i=0; i<syncedNodes->GetNumberOfItems(); ++i)
  {
    vtkMRMLSequenceNode* syncedNode = vtkMRMLSequenceNode::SafeDownCast( syncedNodes->GetItemAsObject(i) );
    if (!syncedNode)
    {
      continue;
    }

    // If this is the master node, then add the master status string (but that is all the difference in the GUI)
    if (!strcmp(syncedNode->GetID(),d->activeBrowserNode()->GetMasterSequenceNode()->GetID()))
    {
      QTableWidgetItem* masterStatusItem = new QTableWidgetItem( MASTER_NODE_STATUS_STRING );
      masterStatusItem->setFlags(masterStatusItem->flags() ^ Qt::ItemIsEditable);
      d->tableWidget_SynchronizedSequenceNodes->setItem(i, SYNCH_NODES_STATUS_COLUMN, masterStatusItem);
    }

    // Create checkboxes
    QCheckBox* playbackCheckbox = new QCheckBox(d->tableWidget_SynchronizedSequenceNodes);
    playbackCheckbox->setToolTip(tr("Include this node in synchronized playback"));
    playbackCheckbox->setProperty("MRMLNodeID", QString(syncedNode->GetID()));

    QCheckBox* recordingCheckbox = new QCheckBox(d->tableWidget_SynchronizedSequenceNodes);
    recordingCheckbox->setToolTip(tr("Include this node in synchronized recording"));
    recordingCheckbox->setProperty("MRMLNodeID", QString(syncedNode->GetID()));

    QCheckBox* overwriteProxyNameCheckbox = new QCheckBox(d->tableWidget_SynchronizedSequenceNodes);
    overwriteProxyNameCheckbox->setToolTip(tr("Overwrite the associated node's name during playback"));
    overwriteProxyNameCheckbox->setProperty("MRMLNodeID", QString(syncedNode->GetID()));

    // Set previous checked state of the checkbox
    bool playbackChecked = d->activeBrowserNode()->GetPlayback(syncedNode);
    playbackCheckbox->setChecked(playbackChecked);

    bool recordingChecked = d->activeBrowserNode()->GetRecording(syncedNode);
    recordingCheckbox->setChecked(recordingChecked);

    bool overwriteProxyNameChecked = d->activeBrowserNode()->GetOverwriteProxyName(syncedNode);
    overwriteProxyNameCheckbox->setChecked(overwriteProxyNameChecked);

    connect(playbackCheckbox, SIGNAL(stateChanged(int)), this, SLOT(synchronizedSequenceNodePlaybackStateChanged(int)));
    connect(recordingCheckbox, SIGNAL(stateChanged(int)), this, SLOT(synchronizedSequenceNodeRecordingStateChanged(int)));
    connect(overwriteProxyNameCheckbox, SIGNAL(stateChanged(int)), this, SLOT(synchronizedSequenceNodeOverwriteProxyNameStateChanged(int)));

    d->tableWidget_SynchronizedSequenceNodes->setCellWidget(i, SYNCH_NODES_PLAYBACK_COLUMN, playbackCheckbox);
    d->tableWidget_SynchronizedSequenceNodes->setCellWidget(i, SYNCH_NODES_RECORDING_COLUMN, recordingCheckbox);
    d->tableWidget_SynchronizedSequenceNodes->setCellWidget(i, SYNCH_NODES_OVERWRITE_PROXY_NAME_COLUMN, overwriteProxyNameCheckbox);
    
    QTableWidgetItem* nameItem = new QTableWidgetItem( QString(syncedNode->GetName()) );
    nameItem->setFlags(nameItem->flags() ^ Qt::ItemIsEditable);
    d->tableWidget_SynchronizedSequenceNodes->setItem(i, SYNCH_NODES_NAME_COLUMN, nameItem);

    vtkMRMLNode* proxyNode = d->activeBrowserNode()->GetProxyNode(syncedNode);
    qMRMLNodeComboBox* proxyNodeComboBox = new qMRMLNodeComboBox();
    proxyNodeComboBox->setNodeTypes(QStringList() << syncedNode->GetDataNodeClassName().c_str());
    proxyNodeComboBox->setAddEnabled(true);
    proxyNodeComboBox->setNoneEnabled(true);
    proxyNodeComboBox->setRemoveEnabled(true);
    proxyNodeComboBox->setRenameEnabled(true);
    proxyNodeComboBox->setShowChildNodeTypes(true);
    proxyNodeComboBox->setMRMLScene(this->mrmlScene());
    proxyNodeComboBox->setCurrentNode(proxyNode);
    proxyNodeComboBox->setProperty("MRMLNodeID", QString(syncedNode->GetID()));
    d->tableWidget_SynchronizedSequenceNodes->setCellWidget(i, SYNCH_NODES_PROXY_COLUMN, proxyNodeComboBox);

    connect(proxyNodeComboBox, SIGNAL(currentNodeChanged(vtkMRMLNode*)), this, SLOT(onProxyNodeChanged(vtkMRMLNode*)));
  }

}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::synchronizedSequenceNodePlaybackStateChanged(int aState)
{
  Q_D(qSlicerSequenceBrowserModuleWidget);

  if (d->activeBrowserNode()==NULL)
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
  d->activeBrowserNode()->SetPlayback(synchronizedNode, aState);
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::synchronizedSequenceNodeRecordingStateChanged(int aState)
{
  Q_D(qSlicerSequenceBrowserModuleWidget);

  if (d->activeBrowserNode()==NULL)
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
  d->activeBrowserNode()->SetRecording(synchronizedNode, aState);
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::synchronizedSequenceNodeOverwriteProxyNameStateChanged(int aState)
{
  Q_D(qSlicerSequenceBrowserModuleWidget);

  if (d->activeBrowserNode()==NULL)
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
  d->activeBrowserNode()->SetOverwriteProxyName(synchronizedNode, aState);
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModuleWidget::onProxyNodeChanged(vtkMRMLNode* newProxyNode)
{
  Q_D(qSlicerSequenceBrowserModuleWidget);

  if (d->activeBrowserNode()==NULL)
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
  d->logic()->AddSynchronizedNode(synchronizedNode, newProxyNode, d->activeBrowserNode());
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
