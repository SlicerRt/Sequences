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

// SlicerQt includes
#include "qSlicerMultidimDataModuleWidget.h"
#include "ui_qSlicerMultidimDataModuleWidget.h"
#include "qSlicerApplication.h"
#include "qSlicerLayoutManager.h"

// VTK includes
#include "vtkMRMLHierarchyNode.h"

// MRML includes
#include "vtkMRMLScene.h"

// MultidimData includes
#include "vtkSlicerMultidimDataLogic.h"
#include "vtkMRMLMultidimDataNode.h"

#define FROM_STD_STRING_SAFE(unsafeString) QString::fromStdString( unsafeString==NULL?"":unsafeString )
#define FROM_ATTRIBUTE_SAFE(unsafeString) ( unsafeString==NULL?"":unsafeString )

enum
{
  DATA_NODE_VIS_COLUMN=0,
  DATA_NODE_VALUE_COLUMN,
  DATA_NODE_NAME_COLUMN,
  DATA_NODE_NUMBER_OF_COLUMNS // this must be the last line in this enum
};

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerMultidimDataModuleWidgetPrivate: public Ui_qSlicerMultidimDataModuleWidget
{
  Q_DECLARE_PUBLIC( qSlicerMultidimDataModuleWidget ); 

protected:
  qSlicerMultidimDataModuleWidget* const q_ptr;
public:
  qSlicerMultidimDataModuleWidgetPrivate( qSlicerMultidimDataModuleWidget& object );
  ~qSlicerMultidimDataModuleWidgetPrivate();

  vtkSlicerMultidimDataLogic* logic() const;
  
  /// Get a list of MLRML nodes that are in the scene but not added to the multidimensional data node at the chosen parameterValue
  void GetDataNodeCandidates(vtkCollection* foundNodes, vtkMRMLMultidimDataNode* rootNode);
};

//-----------------------------------------------------------------------------
// qSlicerMultidimDataModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerMultidimDataModuleWidgetPrivate::qSlicerMultidimDataModuleWidgetPrivate( qSlicerMultidimDataModuleWidget& object ) : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------

qSlicerMultidimDataModuleWidgetPrivate::~qSlicerMultidimDataModuleWidgetPrivate()
{
}


vtkSlicerMultidimDataLogic* qSlicerMultidimDataModuleWidgetPrivate::logic() const
{
  Q_Q( const qSlicerMultidimDataModuleWidget );
  return vtkSlicerMultidimDataLogic::SafeDownCast( q->logic() );
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidgetPrivate::GetDataNodeCandidates(vtkCollection* foundNodes, vtkMRMLMultidimDataNode* rootNode)
{
  Q_Q( const qSlicerMultidimDataModuleWidget );

  if (foundNodes==NULL)
  {
    qCritical() << "GetDataNodeCandidates failed, invalid output collection"; 
    return;
  }
  foundNodes->RemoveAllItems();
  if (rootNode==NULL)
  {
    qCritical() << "GetDataNodeCandidates failed, invalid root node"; 
    return;
  }

  std::string dataNodeClassName=rootNode->GetDataNodeClassName();

  for ( int i = 0; i < rootNode->GetScene()->GetNumberOfNodes(); i++ )
  {
    vtkMRMLNode* currentNode = vtkMRMLNode::SafeDownCast( rootNode->GetScene()->GetNthNode( i ) );    
    if (currentNode->GetHideFromEditors())
    {
      // don't show hidden nodes, they would clutter the view
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
// qSlicerMultidimDataModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerMultidimDataModuleWidget::qSlicerMultidimDataModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerMultidimDataModuleWidgetPrivate( *this ) )
{
}

//-----------------------------------------------------------------------------
qSlicerMultidimDataModuleWidget::~qSlicerMultidimDataModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::setup()
{
  Q_D(qSlicerMultidimDataModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  connect( d->MRMLNodeComboBox_MultidimDataRoot, SIGNAL( currentNodeChanged( vtkMRMLNode* ) ), this, SLOT( onRootNodeChanged() ) );

  connect( d->LineEdit_ParameterName, SIGNAL( textEdited( const QString & ) ), this, SLOT( onParameterNameEdited() ) );
  connect( d->LineEdit_ParameterUnit, SIGNAL( textEdited( const QString & ) ), this, SLOT( onParameterUnitEdited() ) );

  connect( d->TableWidget_DataNodes, SIGNAL( currentCellChanged( int, int, int, int ) ), this, SLOT( onDataNodeChanged() ) );
  connect( d->TableWidget_DataNodes, SIGNAL( cellChanged( int, int ) ), this, SLOT( onDataNodeEdited( int, int ) ) );
  connect( d->TableWidget_DataNodes, SIGNAL( cellClicked( int, int ) ), this, SLOT( onHideDataNodeClicked( int, int ) ) );

  connect( d->PushButton_AddDataNode, SIGNAL( clicked() ), this, SLOT( onAddDataNodeButtonClicked() ) );
  d->PushButton_AddDataNode->setIcon( QApplication::style()->standardIcon( QStyle::SP_ArrowLeft ) );
  connect( d->PushButton_RemoveDataNode, SIGNAL( clicked() ), this, SLOT( onRemoveDataNodeButtonClicked() ) );
  d->PushButton_RemoveDataNode->setIcon( QIcon( ":/Icons/SequenceNodeDelete.png" ) );

  d->ExpandButton_DataNodes->setChecked( false );
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::enter()
{
  Q_D(qSlicerMultidimDataModuleWidget);

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
  }
  else
  {
    qCritical() << "Entering the MultidimDataBrowser module failed, scene is invalid";
  }

  this->Superclass::enter();
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::exit()
{
  this->Superclass::exit();

  // remove mrml scene observations, don't need to update the GUI while the
  // module is not showing
  this->qvtkDisconnectAll();
} 

//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::onNodeAddedEvent(vtkObject*, vtkObject* node)
{
  Q_D(qSlicerMultidimDataModuleWidget);
  if (!this->mrmlScene() || this->mrmlScene()->IsBatchProcessing())
  {
    return;
  }
  UpdateCandidateNodes();
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::onNodeRemovedEvent(vtkObject* scene, vtkObject* node)
{
  Q_UNUSED(scene);
  Q_UNUSED(node);
  if (!this->mrmlScene() || this->mrmlScene()->IsBatchProcessing())
    {
    return;
    }
  UpdateCandidateNodes();
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::onMRMLSceneEndImportEvent()
{
  UpdateCandidateNodes();
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::onMRMLSceneEndRestoreEvent()
{
  UpdateCandidateNodes();
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::onMRMLSceneEndBatchProcessEvent()
{
  if (!this->mrmlScene())
    {
    return;
    }
  UpdateCandidateNodes();
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::onMRMLSceneEndCloseEvent()
{
  if (!this->mrmlScene() || this->mrmlScene()->IsBatchProcessing())
    {
    return;
    }
  UpdateCandidateNodes();
} 


//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::onRootNodeChanged()
{
  Q_D(qSlicerMultidimDataModuleWidget);

  this->UpdateRootNode();
  this->UpdateCandidateNodes();
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::onParameterNameEdited()
{
  Q_D(qSlicerMultidimDataModuleWidget);

  vtkMRMLMultidimDataNode* currentRoot = vtkMRMLMultidimDataNode::SafeDownCast( d->MRMLNodeComboBox_MultidimDataRoot->currentNode() );
  if ( currentRoot == NULL )
  {
    return;
  }

  currentRoot->SetDimensionName(d->LineEdit_ParameterName->text().toLatin1().constData());

  this->UpdateRootNode();
}



//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::onParameterUnitEdited()
{
  Q_D(qSlicerMultidimDataModuleWidget);

  vtkMRMLMultidimDataNode* currentRoot = vtkMRMLMultidimDataNode::SafeDownCast( d->MRMLNodeComboBox_MultidimDataRoot->currentNode() );
  if ( currentRoot == NULL )
  {
    return;
  }

  currentRoot->SetUnit( d->LineEdit_ParameterUnit->text().toLatin1().constData() );

  this->UpdateRootNode();
}


//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::onDataNodeEdited( int row, int column )
{
  Q_D(qSlicerMultidimDataModuleWidget);

  // Ensure that the user is editing, not the bundle changed programmatically
  if ( d->TableWidget_DataNodes->currentRow() != row || d->TableWidget_DataNodes->currentColumn() != column )
  {
    return;
  }

  vtkMRMLMultidimDataNode* currentRoot = vtkMRMLMultidimDataNode::SafeDownCast( d->MRMLNodeComboBox_MultidimDataRoot->currentNode() );
  if ( currentRoot == NULL )
  {
    return;
  }

  std::string currentParameterValue = currentRoot->GetNthParameterValue( d->TableWidget_DataNodes->currentRow() );
  if ( currentParameterValue.empty() )
  {
    return;
  }

  vtkMRMLNode* currentDataNode = currentRoot->GetDataNodeAtValue(currentParameterValue.c_str() );
  if ( currentDataNode == NULL )
  {
    return;
  }

  // Grab the tex from the modified item
  QTableWidgetItem* qItem = d->TableWidget_DataNodes->item( row, column );
  QString qText = qItem->text();

  if ( column == DATA_NODE_VALUE_COLUMN )
  {
    currentRoot->UpdateParameterValue( currentParameterValue.c_str(), qText.toLatin1().constData() );
    // TODO: Should this be propagated to data node names?
  }

  if ( column == DATA_NODE_NAME_COLUMN )
  {
    currentDataNode->SetName( qText.toLatin1().constData() );
  }

  this->UpdateRootNode();
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::onAddDataNodeButtonClicked()
{
  Q_D(qSlicerMultidimDataModuleWidget);

  vtkMRMLMultidimDataNode* currentRoot = vtkMRMLMultidimDataNode::SafeDownCast( d->MRMLNodeComboBox_MultidimDataRoot->currentNode() );
  if ( currentRoot == NULL )
  {
    return;
  }

  std::string currentParameterValue = d->LineEdit_NewDataNodeParameterValue->text().toLatin1().constData();
  if ( currentParameterValue.empty() )
  {
    qCritical() << "Cannot add new data node, as parameter value is not specified";
    return;
  }

  // Get the selected node
  vtkSmartPointer<vtkCollection> candidateNodes = vtkSmartPointer<vtkCollection>::New();
  d->GetDataNodeCandidates( candidateNodes, currentRoot );

  int row = d->ListWidget_CandidateDataNodes->currentRow();
  vtkMRMLNode* currentCandidateNode = vtkMRMLNode::SafeDownCast( candidateNodes->GetItemAsObject( row ) );
  currentRoot->SetDataNodeAtValue(currentCandidateNode, currentParameterValue.c_str() );

  // Auto-increment the parameter value in the new data textbox if it is an integer
  QString oldParameterValue=d->LineEdit_NewDataNodeParameterValue->text();
  bool isInteger=false;
  int oldParameterNumber = oldParameterValue.toInt(&isInteger);
  if (isInteger)
  {
    QString newParameterValue=QString::number(oldParameterNumber+1);
    d->LineEdit_NewDataNodeParameterValue->setText(newParameterValue);
  }

  this->UpdateRootNode();
  this->UpdateCandidateNodes();
}



//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::onRemoveDataNodeButtonClicked()
{
  Q_D(qSlicerMultidimDataModuleWidget);

  vtkMRMLMultidimDataNode* currentRoot = vtkMRMLMultidimDataNode::SafeDownCast( d->MRMLNodeComboBox_MultidimDataRoot->currentNode() );
  if ( currentRoot == NULL )
  {
    return;
  }

  std::string currentParameterValue = currentRoot->GetNthParameterValue( d->TableWidget_DataNodes->currentRow() );
  if ( currentParameterValue.empty() )
  {
    return;
  }
  currentRoot->RemoveDataNodeAtValue( currentParameterValue.c_str() );  

  this->UpdateRootNode();
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::onHideDataNodeClicked( int row, int column )
{
  Q_D(qSlicerMultidimDataModuleWidget);

  // TODO: find/create a browser node and show/hide the node
  
  /*

  vtkMRMLMultidimDataNode* currentRoot = vtkMRMLMultidimDataNode::SafeDownCast( d->MRMLNodeComboBox_MultidimDataRoot->currentNode() );
  if ( currentRoot == NULL )
  {
    return;
  }

  std::string currentParameterValue = currentRoot->GetNthParameterValue( d->TableWidget_DataNodes->currentRow() );
  if ( currentParameterValue.empty() )
  {
    return;
  }

  if ( column != DATA_NODE_VIS_COLUMN )
  {
    return;
  }

  // Get the selected nodes

  d->logic()->SetDataNodesHiddenAtValue( currentRoot, ! d->logic()->GetDataNodesHiddenAtValue( currentRoot, currentParameterValue.c_str() ), currentParameterValue.c_str() );

  // Change the eye for the current sequence node
  QTableWidgetItem* visItem = new QTableWidgetItem( QString( "" ) );
  this->CreateVisItem( visItem, d->logic()->GetDataNodesHiddenAtValue( currentRoot, currentParameterValue.c_str() ) );
  d->TableWidget_DataNodes->setItem( row, DATA_NODE_VIS_COLUMN, visItem ); // This changes current bundle to NULL
  d->TableWidget_DataNodes->setCurrentCell( row, column ); // Reset the current cell so updating the sequence node works

  this->UpdateRootNode();
  */
}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::UpdateRootNode()
{
  Q_D(qSlicerMultidimDataModuleWidget);

  vtkMRMLMultidimDataNode* currentRoot = vtkMRMLMultidimDataNode::SafeDownCast( d->MRMLNodeComboBox_MultidimDataRoot->currentNode() );

  if ( currentRoot == NULL )
  {
    d->Label_DataNodeTypeValue->setText( FROM_STD_STRING_SAFE( "any" ) );
    d->LineEdit_ParameterName->setText( FROM_STD_STRING_SAFE( "" ) );
    d->LineEdit_ParameterUnit->setText( FROM_STD_STRING_SAFE( "" ) );
    d->TableWidget_DataNodes->clear();
    d->TableWidget_DataNodes->setRowCount( 0 );
    d->TableWidget_DataNodes->setColumnCount( 0 );
    d->ListWidget_CandidateDataNodes->clear();
    return;
  }  

  // Put the correct properties in the root node table
  QString typeName=currentRoot->GetDataNodeClassName().c_str();
  if (typeName.startsWith("vtkMRML"))
  {
    typeName.remove(0,7);
  }
  if (typeName.endsWith("Node"))
  {
    typeName.remove(typeName.length()-4,4);
  }
  if (typeName.isEmpty())
  {
    typeName="any";
  }
  d->Label_DataNodeTypeValue->setText( typeName.toLatin1().constData() ); // TODO: maybe use the node->tag instead?

  d->LineEdit_ParameterName->setText( FROM_STD_STRING_SAFE( currentRoot->GetDimensionName() ) );
  d->LineEdit_ParameterUnit->setText( FROM_STD_STRING_SAFE( currentRoot->GetUnit() ) );

  // Display all of the sequence nodes
  d->TableWidget_DataNodes->clear();
  d->TableWidget_DataNodes->setRowCount( currentRoot->GetNumberOfDataNodes() );
  d->TableWidget_DataNodes->setColumnCount( DATA_NODE_NUMBER_OF_COLUMNS );
  std::stringstream valueHeader;
  valueHeader << FROM_ATTRIBUTE_SAFE( currentRoot->GetDimensionName() ); 
  valueHeader << " (" << FROM_ATTRIBUTE_SAFE( currentRoot->GetUnit() ) << ")";
  QStringList SequenceNodesTableHeader;
  SequenceNodesTableHeader.insert( DATA_NODE_VIS_COLUMN, "Vis" );
  SequenceNodesTableHeader.insert( DATA_NODE_VALUE_COLUMN, valueHeader.str().c_str() );
  SequenceNodesTableHeader.insert( DATA_NODE_NAME_COLUMN, "Name" );
  d->TableWidget_DataNodes->setHorizontalHeaderLabels( SequenceNodesTableHeader );  

  int numberOfDataNodes=currentRoot->GetNumberOfDataNodes();
  for ( int dataNodeIndex = 0; dataNodeIndex < numberOfDataNodes; dataNodeIndex++ )
  {
    std::string currentValue = currentRoot->GetNthParameterValue( dataNodeIndex );
    vtkMRMLNode* currentDataNode = currentRoot->GetNthDataNode( dataNodeIndex );

    if (currentDataNode==NULL)
    {
      qCritical() << "qSlicerMultidimDataModuleWidget::UpdateRootNode invalid data node";
      continue;
    }

    // Display the data node information
    QTableWidgetItem* visItem = new QTableWidgetItem( QString( "" ) );
    //this->CreateVisItem( visItem, d->logic()->GetDataNodesHiddenAtValue( currentRoot, currentValue.c_str() ) );

    QTableWidgetItem* valueItem = new QTableWidgetItem( FROM_STD_STRING_SAFE( currentValue.c_str() ) );

    QTableWidgetItem* nameItem = new QTableWidgetItem( FROM_STD_STRING_SAFE( currentDataNode->GetName() ) );

    d->TableWidget_DataNodes->setItem( dataNodeIndex, DATA_NODE_VIS_COLUMN, visItem );
    d->TableWidget_DataNodes->setItem( dataNodeIndex, DATA_NODE_VALUE_COLUMN, valueItem );
    d->TableWidget_DataNodes->setItem( dataNodeIndex, DATA_NODE_NAME_COLUMN, nameItem );
  }

  d->TableWidget_DataNodes->resizeColumnsToContents();  
  d->TableWidget_DataNodes->resizeRowsToContents();  
  
  /*
  if ( d->logic()->GetDataNodesHiddenAtValue( currentRoot, currentValue ) )
  {
    d->CheckBox_HideDataNodes->setCheckState( Qt::Checked );
  }
  else
  {
    d->CheckBox_HideDataNodes->setCheckState( Qt::Unchecked );
  }
  */

}


//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::UpdateCandidateNodes()
{
  Q_D(qSlicerMultidimDataModuleWidget);

  vtkMRMLMultidimDataNode* currentRoot = vtkMRMLMultidimDataNode::SafeDownCast( d->MRMLNodeComboBox_MultidimDataRoot->currentNode() );
  if ( currentRoot == NULL )
  {
    d->ListWidget_CandidateDataNodes->clear();
    return;
  }  

  // Display the candidate data nodes
  vtkSmartPointer<vtkCollection> candidateNodes = vtkSmartPointer<vtkCollection>::New();
  d->GetDataNodeCandidates( candidateNodes, currentRoot );

  int row = d->ListWidget_CandidateDataNodes->currentRow();
  vtkMRMLNode* selectedCandidateNode = NULL;
  if (row>=0)
  {
    selectedCandidateNode = vtkMRMLNode::SafeDownCast( candidateNodes->GetItemAsObject( row ) );
  }
  
  d->ListWidget_CandidateDataNodes->clear();

  int selectedCandidateNodeIndex = -1;
  for ( int i = 0; i < candidateNodes->GetNumberOfItems(); i++ )
  {
    vtkMRMLNode* currentCandidateNode = vtkMRMLNode::SafeDownCast( candidateNodes->GetItemAsObject( i ));
    if (selectedCandidateNode==currentCandidateNode)
    {
      selectedCandidateNodeIndex = i;
    }
    d->ListWidget_CandidateDataNodes->addItem( QString::fromStdString( currentCandidateNode->GetName() ) );
  }

  if (selectedCandidateNodeIndex>=0)
  {
    d->ListWidget_CandidateDataNodes->setCurrentRow(selectedCandidateNodeIndex);
  }

}

//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::CreateVisItem( QTableWidgetItem* visItem, bool hidden )
{
  Q_D(qSlicerMultidimDataModuleWidget);

  // Display the "hiddenness" of the data nodes
  visItem->setFlags( visItem->flags() & ~Qt::ItemIsEditable );
  if ( hidden )
  {
    visItem->setIcon( QIcon( ":/Icons/DataNodeHidden.png" ) );
  }
  else
  {
    visItem->setIcon( QIcon( ":/Icons/DataNodeUnhidden.png" ) );
  }

}
