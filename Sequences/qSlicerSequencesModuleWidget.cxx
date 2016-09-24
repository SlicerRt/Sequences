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
#include <QDebug>
#include <QListWidgetItem>

// SlicerQt includes
#include "qSlicerSequencesModuleWidget.h"
#include "ui_qSlicerSequencesModuleWidget.h"
#include "qSlicerApplication.h"
#include "qSlicerLayoutManager.h"

// VTK includes
#include <vtkNew.h>
#include "vtkMRMLHierarchyNode.h"

// MRML includes
#include "vtkMRMLScene.h"

// Sequence includes
#include "vtkSlicerSequencesLogic.h"
#include "vtkMRMLSequenceNode.h"

#define FROM_STD_STRING_SAFE(unsafeString) QString::fromStdString( unsafeString==NULL?"":unsafeString )
#define FROM_ATTRIBUTE_SAFE(unsafeString) ( unsafeString==NULL?"":unsafeString )

enum
{
//  DATA_NODE_VIS_COLUMN=0,
  DATA_NODE_VALUE_COLUMN,
  DATA_NODE_NAME_COLUMN,
  DATA_NODE_NUMBER_OF_COLUMNS // this must be the last line in this enum
};

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerSequencesModuleWidgetPrivate: public Ui_qSlicerSequencesModuleWidget
{
  Q_DECLARE_PUBLIC( qSlicerSequencesModuleWidget ); 

protected:
  qSlicerSequencesModuleWidget* const q_ptr;
public:
  qSlicerSequencesModuleWidgetPrivate( qSlicerSequencesModuleWidget& object );
  ~qSlicerSequencesModuleWidgetPrivate();

  vtkSlicerSequencesLogic* logic() const;
  
  /// Get a list of MLRML nodes that are in the scene but not added to the sequences data node at the chosen index value
  void GetDataNodeCandidates(vtkCollection* foundNodes, vtkMRMLSequenceNode* sequenceNode);

  vtkWeakPointer<vtkMRMLSequenceNode> ActiveSequenceNode;
  QString DataNodeCandidatesClassName; // data node class name that was used for populating the candidate node list
};

//-----------------------------------------------------------------------------
// qSlicerSequencesModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerSequencesModuleWidgetPrivate::qSlicerSequencesModuleWidgetPrivate( qSlicerSequencesModuleWidget& object )
: q_ptr(&object)
, DataNodeCandidatesClassName("(uninitialized)") // this will force an initial update
{
}

//-----------------------------------------------------------------------------
qSlicerSequencesModuleWidgetPrivate::~qSlicerSequencesModuleWidgetPrivate()
{
}

//-----------------------------------------------------------------------------
vtkSlicerSequencesLogic* qSlicerSequencesModuleWidgetPrivate::logic() const
{
  Q_Q( const qSlicerSequencesModuleWidget );
  return vtkSlicerSequencesLogic::SafeDownCast( q->logic() );
}

//-----------------------------------------------------------------------------
void qSlicerSequencesModuleWidgetPrivate::GetDataNodeCandidates(vtkCollection* foundNodes, vtkMRMLSequenceNode* sequenceNode)
{
  Q_Q( const qSlicerSequencesModuleWidget );

  if (foundNodes==NULL)
  {
    qCritical() << "GetDataNodeCandidates failed, invalid output collection"; 
    return;
  }
  foundNodes->RemoveAllItems();
  if (sequenceNode==NULL)
  {
    qCritical() << "GetDataNodeCandidates failed, invalid sequence node"; 
    return;
  }

  std::string dataNodeClassName=sequenceNode->GetDataNodeClassName();

  for ( int i = 0; i < sequenceNode->GetScene()->GetNumberOfNodes(); i++ )
  {
    vtkMRMLNode* currentNode = vtkMRMLNode::SafeDownCast( sequenceNode->GetScene()->GetNthNode( i ) );    
    if (currentNode->GetHideFromEditors())
    {
      // don't show hidden nodes, they would clutter the view
      continue;
    }
    if (currentNode->GetSingletonTag()!=NULL)
    {
      // don't allow adding singletons (mainly because we can only store one singleton node in a scene, so we couldn't store it)
      continue;
    }
    if (currentNode==sequenceNode)
    {
      // don't allow adding itself as data node
      continue;
    }
    if (!dataNodeClassName.empty())
    {
      if (dataNodeClassName.compare(currentNode->GetClassName())!=0)
      {
        // class is not compatible with elements already in the sequence, don't show it
        continue;
      }
    }    
    foundNodes->AddItem( currentNode );    
  }
}

//-----------------------------------------------------------------------------
// qSlicerSequencesModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerSequencesModuleWidget::qSlicerSequencesModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerSequencesModuleWidgetPrivate( *this ) )
{
}

//-----------------------------------------------------------------------------
qSlicerSequencesModuleWidget::~qSlicerSequencesModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerSequencesModuleWidget::setup()
{
  Q_D(qSlicerSequencesModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  if (d->ComboBox_IndexType->count() == 0)
  {
    for (int indexType=0; indexType<vtkMRMLSequenceNode::NumberOfIndexTypes; indexType++)
    {
      d->ComboBox_IndexType->addItem(vtkMRMLSequenceNode::GetIndexTypeAsString(indexType).c_str());
    }
  }

  d->TableWidget_DataNodes->setColumnWidth( DATA_NODE_VALUE_COLUMN, 30 );
  d->TableWidget_DataNodes->setColumnWidth( DATA_NODE_NAME_COLUMN, 100 );

  connect( d->MRMLNodeComboBox_Sequence, SIGNAL( currentNodeChanged( vtkMRMLNode* ) ), this, SLOT( onSequenceNodeSelectionChanged() ) );

  connect( d->LineEdit_IndexName, SIGNAL( textEdited( const QString & ) ), this, SLOT( onIndexNameEdited() ) );
  connect( d->LineEdit_IndexUnit, SIGNAL( textEdited( const QString & ) ), this, SLOT( onIndexUnitEdited() ) );
  connect( d->ComboBox_IndexType, SIGNAL( currentIndexChanged( const QString & ) ), this, SLOT( onIndexTypeEdited(QString) ) );

  //connect( d->TableWidget_DataNodes, SIGNAL( currentCellChanged( int, int, int, int ) ), this, SLOT( onDataNodeChanged() ) );
  connect( d->TableWidget_DataNodes, SIGNAL( cellChanged( int, int ) ), this, SLOT( onDataNodeEdited( int, int ) ) );
  connect( d->TableWidget_DataNodes, SIGNAL( cellClicked( int, int ) ), this, SLOT( onHideDataNodeClicked( int, int ) ) );

  connect( d->PushButton_AddDataNode, SIGNAL( clicked() ), this, SLOT( onAddDataNodeButtonClicked() ) );
  d->PushButton_AddDataNode->setIcon( QApplication::style()->standardIcon( QStyle::SP_ArrowLeft ) );
  connect( d->PushButton_RemoveDataNode, SIGNAL( clicked() ), this, SLOT( onRemoveDataNodeButtonClicked() ) );
  d->PushButton_RemoveDataNode->setIcon( QIcon( ":/Icons/DataNodeDelete.png" ) );

  d->ExpandButton_DataNodes->setChecked( false );
}

//-----------------------------------------------------------------------------
void qSlicerSequencesModuleWidget::enter()
{
  Q_D(qSlicerSequencesModuleWidget);

  if (this->mrmlScene() != 0)
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
    this->setActiveSequenceNode(vtkMRMLSequenceNode::SafeDownCast(d->MRMLNodeComboBox_Sequence->currentNode()));
    this->updateCandidateNodesWidgetFromMRML(true);
  }
  else
  {
    qCritical() << "Entering the SequenceBrowser module failed, scene is invalid";
  }

  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerSequencesModuleWidget::exit()
{
  Q_D(qSlicerSequencesModuleWidget);
  this->Superclass::exit();

  // remove mrml scene observations, don't need to update the GUI while the
  // module is not showing
  this->qvtkDisconnectAll();
  d->ActiveSequenceNode = NULL;
} 

//-----------------------------------------------------------------------------
void qSlicerSequencesModuleWidget::onNodeAddedEvent(vtkObject*, vtkObject* node)
{
  Q_D(qSlicerSequencesModuleWidget);
  if (!this->mrmlScene() || this->mrmlScene()->IsBatchProcessing())
  {
    return;
  }
  this->updateCandidateNodesWidgetFromMRML(true);
}

//-----------------------------------------------------------------------------
void qSlicerSequencesModuleWidget::onNodeRemovedEvent(vtkObject* scene, vtkObject* node)
{
  Q_UNUSED(scene);
  Q_UNUSED(node);
  if (!this->mrmlScene() || this->mrmlScene()->IsBatchProcessing())
    {
    return;
    }
  this->updateCandidateNodesWidgetFromMRML(true);
}

//-----------------------------------------------------------------------------
void qSlicerSequencesModuleWidget::onMRMLSceneEndImportEvent()
{
  this->updateCandidateNodesWidgetFromMRML(true);
}

//-----------------------------------------------------------------------------
void qSlicerSequencesModuleWidget::onMRMLSceneEndRestoreEvent()
{
  this->updateCandidateNodesWidgetFromMRML(true);
}

//-----------------------------------------------------------------------------
void qSlicerSequencesModuleWidget::onMRMLSceneEndBatchProcessEvent()
{
  if (!this->mrmlScene())
    {
    return;
    }
  this->updateCandidateNodesWidgetFromMRML(true);
}

//-----------------------------------------------------------------------------
void qSlicerSequencesModuleWidget::onMRMLSceneEndCloseEvent()
{
  if (!this->mrmlScene() || this->mrmlScene()->IsBatchProcessing())
    {
    return;
    }
  this->updateCandidateNodesWidgetFromMRML(true);
} 

//-----------------------------------------------------------------------------
void qSlicerSequencesModuleWidget::onSequenceNodeSelectionChanged()
{
  Q_D(qSlicerSequencesModuleWidget);
  d->LineEdit_NewDataNodeIndexValue->setText("0");
  this->setActiveSequenceNode(vtkMRMLSequenceNode::SafeDownCast(d->MRMLNodeComboBox_Sequence->currentNode()));
}

//-----------------------------------------------------------------------------
void qSlicerSequencesModuleWidget::onSequenceNodeModified()
{
  Q_D(qSlicerSequencesModuleWidget);
  this->updateSequenceItemWidgetFromMRML();
  this->updateCandidateNodesWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSequencesModuleWidget::onIndexNameEdited()
{
  Q_D(qSlicerSequencesModuleWidget);

  vtkMRMLSequenceNode* currentSequence = vtkMRMLSequenceNode::SafeDownCast( d->MRMLNodeComboBox_Sequence->currentNode() );
  if ( currentSequence == NULL )
  {
    return;
  }

  currentSequence->SetIndexName(d->LineEdit_IndexName->text().toLatin1().constData());
}

//-----------------------------------------------------------------------------
void qSlicerSequencesModuleWidget::onIndexUnitEdited()
{
  Q_D(qSlicerSequencesModuleWidget);

  vtkMRMLSequenceNode* currentSequence = vtkMRMLSequenceNode::SafeDownCast( d->MRMLNodeComboBox_Sequence->currentNode() );
  if ( currentSequence == NULL )
  {
    return;
  }

  currentSequence->SetIndexUnit( d->LineEdit_IndexUnit->text().toLatin1().constData() );
}

//-----------------------------------------------------------------------------
void qSlicerSequencesModuleWidget::onIndexTypeEdited(QString indexTypeString)
{
  Q_D(qSlicerSequencesModuleWidget);

  vtkMRMLSequenceNode* currentSequence = vtkMRMLSequenceNode::SafeDownCast( d->MRMLNodeComboBox_Sequence->currentNode() );
  if ( currentSequence == NULL )
  {
    return;
  }

  currentSequence->SetIndexTypeFromString( indexTypeString.toLatin1().constData() );
}

//-----------------------------------------------------------------------------
void qSlicerSequencesModuleWidget::onDataNodeEdited( int row, int column )
{
  Q_D(qSlicerSequencesModuleWidget);

  // Ensure that the user is editing, not the index changed programmatically
  if ( d->TableWidget_DataNodes->currentRow() != row || d->TableWidget_DataNodes->currentColumn() != column )
  {
    return;
  }

  vtkMRMLSequenceNode* currentSequence = vtkMRMLSequenceNode::SafeDownCast( d->MRMLNodeComboBox_Sequence->currentNode() );
  if ( currentSequence == NULL )
  {
    return;
  }

  std::string currentIndexValue = currentSequence->GetNthIndexValue( d->TableWidget_DataNodes->currentRow() );
  if ( currentIndexValue.empty() )
  {
    return;
  }

  vtkMRMLNode* currentDataNode = currentSequence->GetDataNodeAtValue(currentIndexValue.c_str() );
  if ( currentDataNode == NULL )
  {
    return;
  }

  // Grab the tex from the modified item
  QTableWidgetItem* qItem = d->TableWidget_DataNodes->item( row, column );
  QString qText = qItem->text();

  if ( column == DATA_NODE_VALUE_COLUMN )
  {
    currentSequence->UpdateIndexValue( currentIndexValue.c_str(), qText.toLatin1().constData() );
  }

  if ( column == DATA_NODE_NAME_COLUMN )
  {
    currentDataNode->SetName( qText.toLatin1().constData() );
  }

  this->updateSequenceItemWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSequencesModuleWidget::onAddDataNodeButtonClicked()
{
  Q_D(qSlicerSequencesModuleWidget);

  vtkMRMLSequenceNode* currentSequence = vtkMRMLSequenceNode::SafeDownCast( d->MRMLNodeComboBox_Sequence->currentNode() );
  if ( currentSequence == NULL )
  {
    return;
  }

  std::string currentIndexValue = d->LineEdit_NewDataNodeIndexValue->text().toLatin1().constData();
  if ( currentIndexValue.empty() )
  {
    qCritical() << "Cannot add new data node, as Index value is not specified";
    return;
  }

  // Get the selected node
  QListWidgetItem* currentItem = d->ListWidget_CandidateDataNodes->currentItem();
  if (currentItem == 0)
  {
    qCritical() << "Cannot add new data node, as current data item selection is invalid";
    return;
  }

  QString currentCandidateNodeId = currentItem->data(Qt::UserRole).toString();
  vtkMRMLNode* currentCandidateNode = currentSequence->GetScene()->GetNodeByID(currentCandidateNodeId.toLatin1().constData());
  if (currentCandidateNode == 0)
  {
    qCritical() << "Cannot add new data node, as current data item is invalid";
    return;
  }
  int wasModified = currentSequence->StartModify();
  currentSequence->SetDataNodeAtValue(currentCandidateNode, currentIndexValue.c_str() );

  // Auto-increment the Index value in the new data textbox
  
  QString oldIndexValue=d->LineEdit_NewDataNodeIndexValue->text();
  bool isIndexValueNumeric=false;
  double oldIndexNumber = oldIndexValue.toDouble(&isIndexValueNumeric);
  if (isIndexValueNumeric)
  {
    double incrementValue=d->DoubleSpinBox_IndexValueAutoIncrement->value();
    QString newIndexValue=QString::number(oldIndexNumber+incrementValue);
    d->LineEdit_NewDataNodeIndexValue->setText(newIndexValue);
  }
  currentSequence->EndModify(wasModified);

  // Restore candidate node selection / auto-advance to the next node
  int selectionOffset=0; // can be 0 or 1, the selection in the data nodes list moves forward by this number of elements
  if (d->CheckBox_AutoAdvanceDataSelection->checkState()==Qt::Checked)
  {
    selectionOffset=1;
  }
  for ( int i = 0; i < d->ListWidget_CandidateDataNodes->count(); i++ )
  {
    QListWidgetItem * item = d->ListWidget_CandidateDataNodes->item(i);
    if (item->data(Qt::UserRole).toString() == currentCandidateNodeId)
    {      
      if (i+selectionOffset<d->ListWidget_CandidateDataNodes->count())
      {
        // not at the end of the list, so select the next item
        d->ListWidget_CandidateDataNodes->setCurrentRow(i+selectionOffset);
      }
      else
      {
        // we are at the end of the list (already added the last element),
        // so unselect the item to prevent duplicate adding of the last element
        d->ListWidget_CandidateDataNodes->setCurrentRow(-1);
      }      
      break;
    }
  }
}

//-----------------------------------------------------------------------------
void qSlicerSequencesModuleWidget::onRemoveDataNodeButtonClicked()
{
  Q_D(qSlicerSequencesModuleWidget);

  vtkMRMLSequenceNode* currentSequence = vtkMRMLSequenceNode::SafeDownCast( d->MRMLNodeComboBox_Sequence->currentNode() );
  if ( currentSequence == NULL )
  {
    return;
  }

  std::string currentIndexValue = currentSequence->GetNthIndexValue( d->TableWidget_DataNodes->currentRow() );
  if ( currentIndexValue.empty() )
  {
    return;
  }
  currentSequence->RemoveDataNodeAtValue( currentIndexValue.c_str() );  
}

// --------------------------------------------------------------------------
void qSlicerSequencesModuleWidget::setActiveSequenceNode(vtkMRMLSequenceNode* sequenceNode)
{
  Q_D(qSlicerSequencesModuleWidget);
  if (d->ActiveSequenceNode == sequenceNode)
  {
    return; // no change
  }
  if (d->ActiveSequenceNode != sequenceNode)
  {
    // Reconnect the input node's Modified() event observer
    this->qvtkReconnect(d->ActiveSequenceNode, sequenceNode, vtkCommand::ModifiedEvent,
      this, SLOT(onSequenceNodeModified()));
    d->ActiveSequenceNode = sequenceNode;
  }

  this->updateSequenceItemWidgetFromMRML();
  this->updateCandidateNodesWidgetFromMRML();
}

//-----------------------------------------------------------------------------
void qSlicerSequencesModuleWidget::updateSequenceItemWidgetFromMRML()
{
  Q_D(qSlicerSequencesModuleWidget);

  vtkMRMLSequenceNode* currentSequence = d->ActiveSequenceNode;

  if ( currentSequence == NULL )
  {
    d->Label_DataNodeTypeValue->setText( FROM_STD_STRING_SAFE( "undefined" ) );    
    d->LineEdit_IndexName->setText( FROM_STD_STRING_SAFE( "" ) );    
    d->LineEdit_IndexUnit->setText( FROM_STD_STRING_SAFE( "" ) );    
    d->ComboBox_IndexType->setCurrentIndex(-1);
    d->TableWidget_DataNodes->clear();
    d->TableWidget_DataNodes->setRowCount( 0 );
    d->TableWidget_DataNodes->setColumnCount( 0 );
    d->ListWidget_CandidateDataNodes->clear();
    setEnableWidgets(false);
    return;
  }

  setEnableWidgets(true);

  // Put the correct properties in the sequence node table
  QString typeName=currentSequence->GetDataNodeTagName().c_str();
  d->Label_DataNodeTypeValue->setText( typeName.toLatin1().constData() ); // TODO: maybe use the node->tag instead?

  d->LineEdit_IndexName->setText( currentSequence->GetIndexName().c_str() );
  d->LineEdit_IndexUnit->setText(currentSequence->GetIndexUnit().c_str());
  d->ComboBox_IndexType->setCurrentIndex( d->ComboBox_IndexType->findText(currentSequence->GetIndexTypeAsString().c_str()) );

  // Display all of the sequence nodes
  d->TableWidget_DataNodes->clear();
  d->TableWidget_DataNodes->setRowCount( currentSequence->GetNumberOfDataNodes() );
  d->TableWidget_DataNodes->setColumnCount( DATA_NODE_NUMBER_OF_COLUMNS );
  std::stringstream valueHeader;
  valueHeader << currentSequence->GetIndexName(); 
  valueHeader << " (" << currentSequence->GetIndexUnit() << ")";
  QStringList SequenceNodesTableHeader;
  //SequenceNodesTableHeader.insert( DATA_NODE_VIS_COLUMN, "Vis" );
  SequenceNodesTableHeader.insert( DATA_NODE_VALUE_COLUMN, valueHeader.str().c_str() );
  SequenceNodesTableHeader.insert( DATA_NODE_NAME_COLUMN, "Name" );
  d->TableWidget_DataNodes->setHorizontalHeaderLabels( SequenceNodesTableHeader );  

  int numberOfDataNodes=currentSequence->GetNumberOfDataNodes();
  for ( int dataNodeIndex = 0; dataNodeIndex < numberOfDataNodes; dataNodeIndex++ )
  {
    std::string currentValue = currentSequence->GetNthIndexValue( dataNodeIndex );
    vtkMRMLNode* currentDataNode = currentSequence->GetNthDataNode( dataNodeIndex );

    if (currentDataNode==NULL)
    {
      qCritical() << "qSlicerSequencesModuleWidget::updateSequenceItemWidgetFromMRML invalid data node";
      continue;
    }

    QTableWidgetItem* valueItem = new QTableWidgetItem( FROM_STD_STRING_SAFE( currentValue.c_str() ) );
    valueItem->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter);

    QTableWidgetItem* nameItem = new QTableWidgetItem( FROM_STD_STRING_SAFE( currentDataNode->GetName() ) );

    d->TableWidget_DataNodes->setItem( dataNodeIndex, DATA_NODE_VALUE_COLUMN, valueItem );
    d->TableWidget_DataNodes->setItem( dataNodeIndex, DATA_NODE_NAME_COLUMN, nameItem );
  }

  //d->TableWidget_DataNodes->resizeColumnsToContents();  
  d->TableWidget_DataNodes->resizeRowsToContents();  

  // Open the data node adding section if there are no data nodes yet
  // to make it easier to see how to add new nodes.
  if (numberOfDataNodes==0)
  {
    d->ExpandButton_DataNodes->setChecked(true);
  }
}

//-----------------------------------------------------------------------------
void qSlicerSequencesModuleWidget::updateCandidateNodesWidgetFromMRML(bool forceUpdate /* =false */)
{
  Q_D(qSlicerSequencesModuleWidget);

  vtkMRMLSequenceNode* currentSequence = vtkMRMLSequenceNode::SafeDownCast( d->MRMLNodeComboBox_Sequence->currentNode() );
  if ( currentSequence == NULL )
  {
    d->ListWidget_CandidateDataNodes->clear();
    return;
  }

  QString currentDataNodeCandidatesClassName(currentSequence->GetDataNodeClassName().c_str());
  if (!forceUpdate && d->DataNodeCandidatesClassName == currentDataNodeCandidatesClassName)
  {
    // already up-to-date
    return;
  }
  d->DataNodeCandidatesClassName = currentDataNodeCandidatesClassName;

  // Display the candidate data nodes
  vtkNew<vtkCollection> candidateNodes;
  d->GetDataNodeCandidates( candidateNodes.GetPointer(), currentSequence );
  
  d->ListWidget_CandidateDataNodes->clear();

  for ( int i = 0; i < candidateNodes->GetNumberOfItems(); i++ )
  {
    vtkMRMLNode* currentCandidateNode = vtkMRMLNode::SafeDownCast( candidateNodes->GetItemAsObject( i ));
    QListWidgetItem *qlwi = new QListWidgetItem();
    qlwi->setText(currentCandidateNode->GetName());
    qlwi->setData(Qt::UserRole, QString(currentCandidateNode->GetID()));
    d->ListWidget_CandidateDataNodes->addItem(qlwi);
  }
}

//-----------------------------------------------------------------------------
void qSlicerSequencesModuleWidget::setEnableWidgets(bool enable)
{
  Q_D(qSlicerSequencesModuleWidget);
  d->Label_DataNodeTypeValue->setEnabled(enable);
  d->LineEdit_IndexName->setEnabled(enable);
  d->LineEdit_IndexUnit->setEnabled(enable);
  d->ComboBox_IndexType->setEnabled(enable);
  d->TableWidget_DataNodes->setEnabled(enable);
  d->ListWidget_CandidateDataNodes->setEnabled(enable);
  d->LineEdit_NewDataNodeIndexValue->setEnabled(enable);
  d->PushButton_AddDataNode->setEnabled(enable);
  d->PushButton_RemoveDataNode->setEnabled(enable);
}
