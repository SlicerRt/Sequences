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
#include <QDebug>
#include <QtGui>

// SlicerQt includes
#include "qSlicerMultiDimensionModuleWidget.h"
#include "ui_qSlicerMultiDimensionModuleWidget.h"

// VTK includes
#include "vtkMRMLHierarchyNode.h"

// MultiDimension includes
#include "vtkSlicerMultiDimensionLogic.h"

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerMultiDimensionModuleWidgetPrivate: public Ui_qSlicerMultiDimensionModuleWidget
{
  Q_DECLARE_PUBLIC( qSlicerMultiDimensionModuleWidget ); 

protected:
  qSlicerMultiDimensionModuleWidget* const q_ptr;
public:
  qSlicerMultiDimensionModuleWidgetPrivate( qSlicerMultiDimensionModuleWidget& object );
  ~qSlicerMultiDimensionModuleWidgetPrivate();

  vtkSlicerMultiDimensionLogic* logic() const;
};

//-----------------------------------------------------------------------------
// qSlicerMultiDimensionModuleWidgetPrivate methods

//-----------------------------------------------------------------------------
qSlicerMultiDimensionModuleWidgetPrivate::qSlicerMultiDimensionModuleWidgetPrivate( qSlicerMultiDimensionModuleWidget& object ) : q_ptr(&object)
{
}

//-----------------------------------------------------------------------------

qSlicerMultiDimensionModuleWidgetPrivate::~qSlicerMultiDimensionModuleWidgetPrivate()
{
}


vtkSlicerMultiDimensionLogic* qSlicerMultiDimensionModuleWidgetPrivate::logic() const
{
  Q_Q( const qSlicerMultiDimensionModuleWidget );
  return vtkSlicerMultiDimensionLogic::SafeDownCast( q->logic() );
}

//-----------------------------------------------------------------------------
// qSlicerMultiDimensionModuleWidget methods

//-----------------------------------------------------------------------------
qSlicerMultiDimensionModuleWidget::qSlicerMultiDimensionModuleWidget(QWidget* _parent)
  : Superclass( _parent )
  , d_ptr( new qSlicerMultiDimensionModuleWidgetPrivate( *this ) )
{
}

//-----------------------------------------------------------------------------
qSlicerMultiDimensionModuleWidget::~qSlicerMultiDimensionModuleWidget()
{
}

//-----------------------------------------------------------------------------
void qSlicerMultiDimensionModuleWidget::setup()
{
  Q_D(qSlicerMultiDimensionModuleWidget);
  d->setupUi(this);
  this->Superclass::setup();

  connect( d->MRMLNodeComboBox_MultiDimensionRoot, SIGNAL( currentNodeChanged( vtkMRMLNode* ) ), this, SLOT( onRootNodeChanged() ) );
  connect( d->TableWidget_SequenceNodes, SIGNAL( currentCellChanged( int, int, int, int ) ), this, SLOT( onSequenceNodeChanged() ) );

  connect( d->PushButton_Sequence, SIGNAL( clicked() ), this, SLOT( onSequenceButtonClicked() ) );
  connect( d->PushButton_Unsequence, SIGNAL( clicked() ), this, SLOT( onUnsequenceButtonClicked() ) );

  connect( d->PushButton_RemoveSequenceNode, SIGNAL( clicked() ), this, SLOT( onRemoveSequenceNodeButtonClicked() ) );
  connect( d->CheckBox_HideDataNodes, SIGNAL( stateChanged( int ) ), this, SLOT( onHideDataNodesChecked() ) );
  connect( d->PushButton_AddSequenceNode, SIGNAL( clicked() ), this, SLOT( onAddSequenceNodeButtonClicked() ) );
}


//-----------------------------------------------------------------------------
void qSlicerMultiDimensionModuleWidget::onRootNodeChanged()
{
  Q_D(qSlicerMultiDimensionModuleWidget);

  this->UpdateRootNode();
}



//-----------------------------------------------------------------------------
void qSlicerMultiDimensionModuleWidget::onSequenceNodeChanged()
{
  Q_D(qSlicerMultiDimensionModuleWidget);

  this->UpdateSequenceNode();
}



//-----------------------------------------------------------------------------
void qSlicerMultiDimensionModuleWidget::onSequenceButtonClicked()
{
  Q_D(qSlicerMultiDimensionModuleWidget);

  vtkMRMLHierarchyNode* currentRoot = vtkMRMLHierarchyNode::SafeDownCast( d->MRMLNodeComboBox_MultiDimensionRoot->currentNode() );

  if ( currentRoot == NULL )
  {
    return;
  }

  vtkMRMLHierarchyNode* currentSequenceNode = currentRoot->GetNthChildNode( d->TableWidget_SequenceNodes->currentRow() );

  if ( currentSequenceNode == NULL )
  {
    return;
  }

  // Get the selected nodes
  const char* currentValue = currentSequenceNode->GetAttribute( "MultiDimension.Value" );
  vtkCollection* currentUnsequencedNodes = d->logic()->GetNondataNodesAtValue( currentRoot, currentValue );

  int row = d->ListWidget_UnsequencedNodes->currentRow();
  vtkMRMLNode* currentUnsequencedNode = vtkMRMLNode::SafeDownCast( currentUnsequencedNodes->GetItemAsObject( row ) );
  d->logic()->AddDataNodeAtValue( currentRoot, currentUnsequencedNode, currentValue );

  this->UpdateSequenceNode();
}



//-----------------------------------------------------------------------------
void qSlicerMultiDimensionModuleWidget::onUnsequenceButtonClicked()
{
  Q_D(qSlicerMultiDimensionModuleWidget);

  vtkMRMLHierarchyNode* currentRoot = vtkMRMLHierarchyNode::SafeDownCast( d->MRMLNodeComboBox_MultiDimensionRoot->currentNode() );

  if ( currentRoot == NULL )
  {
    return;
  }

  vtkMRMLHierarchyNode* currentSequenceNode = currentRoot->GetNthChildNode( d->TableWidget_SequenceNodes->currentRow() );

  if ( currentSequenceNode == NULL )
  {
    return;
  }

  // Get the selected nodes
  const char* currentValue = currentSequenceNode->GetAttribute( "MultiDimension.Value" );
  vtkCollection* currentSequencedNodes = d->logic()->GetDataNodesAtValue( currentRoot, currentValue );

  int row = d->ListWidget_SequencedNodes->currentRow();
  vtkMRMLNode* currentSequencedNode = vtkMRMLNode::SafeDownCast( currentSequencedNodes->GetItemAsObject( row ) );
  d->logic()->RemoveDataNodeAtValue( currentRoot, currentSequencedNode, currentValue );

  this->UpdateSequenceNode();
}



//-----------------------------------------------------------------------------
void qSlicerMultiDimensionModuleWidget::onRemoveSequenceNodeButtonClicked()
{
  Q_D(qSlicerMultiDimensionModuleWidget);

  vtkMRMLHierarchyNode* currentRoot = vtkMRMLHierarchyNode::SafeDownCast( d->MRMLNodeComboBox_MultiDimensionRoot->currentNode() );

  if ( currentRoot == NULL )
  {
    return;
  }

  vtkMRMLHierarchyNode* currentSequenceNode = currentRoot->GetNthChildNode( d->TableWidget_SequenceNodes->currentRow() );

  if ( currentSequenceNode == NULL )
  {
    return;
  }

  // Get the selected nodes
  const char* currentValue = currentSequenceNode->GetAttribute( "MultiDimension.Value" );

  d->logic()->RemoveSequenceNodeAtValue( currentRoot, currentValue );

  this->UpdateRootNode();
}



//-----------------------------------------------------------------------------
void qSlicerMultiDimensionModuleWidget::onHideDataNodesChecked()
{
  Q_D(qSlicerMultiDimensionModuleWidget);

  vtkMRMLHierarchyNode* currentRoot = vtkMRMLHierarchyNode::SafeDownCast( d->MRMLNodeComboBox_MultiDimensionRoot->currentNode() );

  if ( currentRoot == NULL )
  {
    return;
  }

  vtkMRMLHierarchyNode* currentSequenceNode = currentRoot->GetNthChildNode( d->TableWidget_SequenceNodes->currentRow() );

  if ( currentSequenceNode == NULL )
  {
    return;
  }

  // Get the selected nodes
  const char* currentValue = currentSequenceNode->GetAttribute( "MultiDimension.Value" );

  d->logic()->SetDataNodesHiddenAtValue( currentRoot, d->CheckBox_HideDataNodes->checkState() == Qt::Checked, currentValue );
}



//-----------------------------------------------------------------------------
void qSlicerMultiDimensionModuleWidget::onAddSequenceNodeButtonClicked()
{
  Q_D(qSlicerMultiDimensionModuleWidget);

  vtkMRMLHierarchyNode* currentRoot = vtkMRMLHierarchyNode::SafeDownCast( d->MRMLNodeComboBox_MultiDimensionRoot->currentNode() );

  if ( currentRoot == NULL )
  {
    return;
  }

  // Get the new value
  QString value = QInputDialog::getText( this, tr("Sequence Value"), tr("Input value for new sequence node:") );
  if ( value.isNull() )
  {
    return;
  }

  d->logic()->CreateSequenceNodeAtValue( currentRoot, value.toStdString().c_str() );

  this->UpdateRootNode();
}




//-----------------------------------------------------------------------------
void qSlicerMultiDimensionModuleWidget::UpdateRootNode()
{
  Q_D(qSlicerMultiDimensionModuleWidget);

  vtkMRMLHierarchyNode* currentRoot = vtkMRMLHierarchyNode::SafeDownCast( d->MRMLNodeComboBox_MultiDimensionRoot->currentNode() );

  if ( currentRoot == NULL )
  {
    return;
  }

  // Put the correct properties in the root node table
  d->TableWidget_Root->clear();
  d->TableWidget_Root->setRowCount( 1 );
  d->TableWidget_Root->setColumnCount( 3 );
  QStringList RootTableHeader;
  RootTableHeader << "Node ID" << "Name" << "Unit";
  d->TableWidget_Root->setHorizontalHeaderLabels( RootTableHeader );
  d->TableWidget_Root->horizontalHeader()->setResizeMode( QHeaderView::Stretch );

  QTableWidgetItem* idItem = new QTableWidgetItem( QString::fromStdString( currentRoot->GetID() ) );
  QTableWidgetItem* nameItem = new QTableWidgetItem( QString::fromStdString( currentRoot->GetAttribute( "MultiDimension.Name" ) ) );
  QTableWidgetItem* unitItem = new QTableWidgetItem( QString::fromStdString( currentRoot->GetAttribute( "MultiDimension.Unit" ) ) );
  d->TableWidget_Root->setItem( 0, 0, idItem );
  d->TableWidget_Root->setItem( 0, 1, nameItem );
  d->TableWidget_Root->setItem( 0, 2, unitItem );  

  d->TableWidget_Root->resizeRowsToContents();  

  // Display all of the sequence nodes
  d->TableWidget_SequenceNodes->clear();
  d->TableWidget_SequenceNodes->setRowCount( currentRoot->GetNumberOfChildrenNodes() );
  d->TableWidget_SequenceNodes->setColumnCount( 2 );
  QStringList SequenceNodesTableHeader;
  SequenceNodesTableHeader << "Node ID" << "Value";
  d->TableWidget_SequenceNodes->setHorizontalHeaderLabels( SequenceNodesTableHeader );
  d->TableWidget_SequenceNodes->horizontalHeader()->setResizeMode( QHeaderView::Stretch );

  for ( int i = 0; i < currentRoot->GetNumberOfChildrenNodes(); i++ )
  {
    vtkMRMLHierarchyNode* currentSequenceNode = currentRoot->GetNthChildNode( i );
    QTableWidgetItem* idItem = new QTableWidgetItem( QString::fromStdString( currentSequenceNode->GetID() ) );
    QTableWidgetItem* valueItem = new QTableWidgetItem( QString::fromStdString( currentSequenceNode->GetAttribute( "MultiDimension.Value" ) ) );
    d->TableWidget_SequenceNodes->setItem( i, 0, idItem );
    d->TableWidget_SequenceNodes->setItem( i, 1, valueItem );
  }

  d->TableWidget_SequenceNodes->resizeRowsToContents();  

  this->UpdateSequenceNode();
}


//-----------------------------------------------------------------------------
void qSlicerMultiDimensionModuleWidget::UpdateSequenceNode()
{
  Q_D(qSlicerMultiDimensionModuleWidget);

  vtkMRMLHierarchyNode* currentRoot = vtkMRMLHierarchyNode::SafeDownCast( d->MRMLNodeComboBox_MultiDimensionRoot->currentNode() );

  if ( currentRoot == NULL )
  {
    return;
  }

  vtkMRMLHierarchyNode* currentSequenceNode = currentRoot->GetNthChildNode( d->TableWidget_SequenceNodes->currentRow() );

  if ( currentSequenceNode == NULL )
  {
    return;
  }

  const char* currentValue = currentSequenceNode->GetAttribute( "MultiDimension.Value" );
  vtkCollection* currentDataNodes = d->logic()->GetDataNodesAtValue( currentRoot, currentValue );

  // Display all of the data nodes
  d->TableWidget_DataNodes->clear();
  d->TableWidget_DataNodes->setRowCount( currentDataNodes->GetNumberOfItems() );
  d->TableWidget_DataNodes->setColumnCount( 3 );
  QStringList SequenceNodesTableHeader;
  SequenceNodesTableHeader << "Node ID" << "Node Name" << "Node Tag";
  d->TableWidget_DataNodes->setHorizontalHeaderLabels( SequenceNodesTableHeader );
  d->TableWidget_DataNodes->horizontalHeader()->setResizeMode( QHeaderView::Stretch );

  for ( int i = 0; i < currentDataNodes->GetNumberOfItems(); i++ )
  {
    vtkMRMLNode* currentDataNode = vtkMRMLNode::SafeDownCast( currentDataNodes->GetItemAsObject( i ) );
    QTableWidgetItem* idItem = new QTableWidgetItem( QString::fromStdString( currentDataNode->GetID() ) );
    QTableWidgetItem* nameItem = new QTableWidgetItem( QString::fromStdString( currentDataNode->GetName() ) );
    QTableWidgetItem* tagItem = new QTableWidgetItem( QString::fromStdString( currentDataNode->GetNodeTagName() ) );
    d->TableWidget_DataNodes->setItem( i, 0, idItem );
    d->TableWidget_DataNodes->setItem( i, 1, nameItem );
    d->TableWidget_DataNodes->setItem( i, 2, tagItem );
  }

  d->TableWidget_DataNodes->resizeRowsToContents();


  // Display the sequenced and unsequenced nodes
  d->ListWidget_SequencedNodes->clear();

  for ( int i = 0; i < currentDataNodes->GetNumberOfItems(); i++ )
  {
    vtkMRMLNode* currentDataNode = vtkMRMLNode::SafeDownCast( currentDataNodes->GetItemAsObject( i ) );
    d->ListWidget_SequencedNodes->addItem( QString::fromStdString( currentDataNode->GetName() ) );
  }

  vtkCollection* currentNondataNodes = d->logic()->GetNondataNodesAtValue( currentRoot, currentValue );
  d->ListWidget_UnsequencedNodes->clear();

  for ( int i = 0; i < currentNondataNodes->GetNumberOfItems(); i++ )
  {
    vtkMRMLNode* currentNode = vtkMRMLNode::SafeDownCast( currentNondataNodes->GetItemAsObject( i ));
    d->ListWidget_UnsequencedNodes->addItem( QString::fromStdString( currentNode->GetName() ) );
  }

  if ( d->logic()->GetDataNodesHiddenAtValue( currentRoot, currentValue ) )
  {
    d->CheckBox_HideDataNodes->setCheckState( Qt::Checked );
  }
  else
  {
    d->CheckBox_HideDataNodes->setCheckState( Qt::Unchecked );
  }

}