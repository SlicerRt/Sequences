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
#include "qSlicerMultiDimensionModuleWidget.h"
#include "ui_qSlicerMultiDimensionModuleWidget.h"
#include "qSlicerApplication.h"
#include "qSlicerLayoutManager.h"

// VTK includes
#include "vtkMRMLHierarchyNode.h"

// MultiDimension includes
#include "vtkSlicerMultiDimensionLogic.h"

#define FROM_STD_STRING_SAFE(unsafeString) QString::fromStdString( unsafeString==NULL?"":unsafeString )

static const int SEQUENCE_NODE_COLUMNS = 3;
static const int SEQUENCE_NODE_VIS_COLUMN = 0;
static const int SEQUENCE_NODE_NAME_COLUMN = 1;
static const int SEQUENCE_NODE_VALUE_COLUMN = 2;

static const int DATA_NODE_COLUMNS = 3;
static const int DATA_NODE_VIS_COLUMN = 0;
static const int DATA_NODE_NAME_COLUMN = 1;
static const int DATA_NODE_TYPE_COLUMN = 2;

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

  connect( d->LineEdit_ParameterName, SIGNAL( textEdited( const QString & ) ), this, SLOT( onParameterNameEdited() ) );
  connect( d->LineEdit_ParameterUnit, SIGNAL( textEdited( const QString & ) ), this, SLOT( onParameterUnitEdited() ) );

  connect( d->TableWidget_SequenceNodes, SIGNAL( cellChanged( int, int ) ), this, SLOT( onSequenceNodeEdited( int, int ) ) );
  connect( d->TableWidget_SequenceNodes, SIGNAL( cellClicked( int, int ) ), this, SLOT( onHideSequenceNodeClicked( int, int ) ) );
  connect( d->TableWidget_DataNodes, SIGNAL( cellChanged( int, int ) ), this, SLOT( onDataNodeEdited( int, int ) ) );
  connect( d->TableWidget_DataNodes, SIGNAL( cellClicked( int, int ) ), this, SLOT( onHideDataNodeClicked( int, int ) ) );

  connect( d->PushButton_AddDataNode, SIGNAL( clicked() ), this, SLOT( onAddDataNodeButtonClicked() ) );
  connect( d->PushButton_RemoveDataNode, SIGNAL( clicked() ), this, SLOT( onRemoveDataNodeButtonClicked() ) );

  connect( d->PushButton_RemoveSequenceNode, SIGNAL( clicked() ), this, SLOT( onRemoveSequenceNodeButtonClicked() ) );
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
void qSlicerMultiDimensionModuleWidget::onParameterNameEdited()
{
  Q_D(qSlicerMultiDimensionModuleWidget);

  vtkMRMLHierarchyNode* currentRoot = vtkMRMLHierarchyNode::SafeDownCast( d->MRMLNodeComboBox_MultiDimensionRoot->currentNode() );

  if ( currentRoot == NULL )
  {
    return;
  }

  QString qName = d->LineEdit_ParameterName->text();
  std::string sName = qName.toStdString();
  const char* cName = sName.c_str();
  currentRoot->SetAttribute( "MultiDimension.Name", cName );

  this->UpdateRootNode();
}



//-----------------------------------------------------------------------------
void qSlicerMultiDimensionModuleWidget::onParameterUnitEdited()
{
  Q_D(qSlicerMultiDimensionModuleWidget);

  vtkMRMLHierarchyNode* currentRoot = vtkMRMLHierarchyNode::SafeDownCast( d->MRMLNodeComboBox_MultiDimensionRoot->currentNode() );

  if ( currentRoot == NULL )
  {
    return;
  }

  QString qUnit = d->LineEdit_ParameterUnit->text();
  std::string sUnit = qUnit.toStdString();
  const char* cUnit = sUnit.c_str();
  currentRoot->SetAttribute( "MultiDimension.Unit", cUnit );

  this->UpdateRootNode();
}


//-----------------------------------------------------------------------------
void qSlicerMultiDimensionModuleWidget::onSequenceNodeEdited( int row, int column )
{
  Q_D(qSlicerMultiDimensionModuleWidget);

  // Ensure that the user is editing, not the cell changed programmatically
  if ( d->TableWidget_SequenceNodes->currentRow() != row || d->TableWidget_SequenceNodes->currentColumn() != column )
  {
    return;
  }

  vtkMRMLHierarchyNode* currentRoot = vtkMRMLHierarchyNode::SafeDownCast( d->MRMLNodeComboBox_MultiDimensionRoot->currentNode() );

  if ( currentRoot == NULL )
  {
    return;
  }

  vtkMRMLHierarchyNode* currentSequenceNode = currentRoot->GetNthChildNode( row );

  if ( currentSequenceNode == NULL )
  {
    return;
  }

  // Grab the tex from the modified item
  QTableWidgetItem* qItem = d->TableWidget_SequenceNodes->item( row, column );
  QString qText = qItem->text();
  std::string sText = qText.toStdString();
  const char* cText = sText.c_str();

  if ( column == SEQUENCE_NODE_NAME_COLUMN )
  {
    currentSequenceNode->SetName( cText );
  }

  if ( column == SEQUENCE_NODE_VALUE_COLUMN )
  {
    currentSequenceNode->SetAttribute( "MultiDimension.Value", cText ); // TODO: Should this be propagated to data node names?
  }

  this->UpdateRootNode();
}



//-----------------------------------------------------------------------------
void qSlicerMultiDimensionModuleWidget::onDataNodeEdited( int row, int column )
{
  Q_D(qSlicerMultiDimensionModuleWidget);

  // Ensure that the user is editing, not the cell changed programmatically
  if ( d->TableWidget_DataNodes->currentRow() != row || d->TableWidget_DataNodes->currentColumn() != column )
  {
    return;
  }

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

  // Get the data node
  const char* currentValue = currentSequenceNode->GetAttribute( "MultiDimension.Value" );
  vtkSmartPointer<vtkCollection> currentDataNodes = vtkSmartPointer<vtkCollection>::New();
  d->logic()->GetDataNodesAtValue( currentDataNodes, currentRoot, currentValue );
  vtkMRMLNode* currentDataNode = vtkMRMLNode::SafeDownCast( currentDataNodes->GetItemAsObject( row ) );

  // Grab the text from the modified item
  QTableWidgetItem* qItem = d->TableWidget_DataNodes->item( row, column );
  QString qText = qItem->text();
  std::string sText = qText.toStdString();
  const char* cText = sText.c_str();

  if ( column == DATA_NODE_NAME_COLUMN )
  {
    currentDataNode->SetName( cText );
  }

  this->UpdateSequenceNode();
}



//-----------------------------------------------------------------------------
void qSlicerMultiDimensionModuleWidget::onAddDataNodeButtonClicked()
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
  vtkSmartPointer<vtkCollection> currentNonDataNodes = vtkSmartPointer<vtkCollection>::New();
  d->logic()->GetNonDataNodesAtValue( currentNonDataNodes, currentRoot, currentValue );

  int row = d->ListWidget_NonDataNodes->currentRow();
  vtkMRMLNode* currentNonDataNode = vtkMRMLNode::SafeDownCast( currentNonDataNodes->GetItemAsObject( row ) );
  d->logic()->AddDataNodeAtValue( currentRoot, currentNonDataNode, currentValue );

  this->UpdateSequenceNode();
}



//-----------------------------------------------------------------------------
void qSlicerMultiDimensionModuleWidget::onRemoveDataNodeButtonClicked()
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
  vtkSmartPointer<vtkCollection> currentDataNodes = vtkSmartPointer<vtkCollection>::New();
  d->logic()->GetDataNodesAtValue( currentDataNodes, currentRoot, currentValue );

  int row = d->TableWidget_DataNodes->currentRow();
  vtkMRMLNode* currentDataNode = vtkMRMLNode::SafeDownCast( currentDataNodes->GetItemAsObject( row ) );
  d->logic()->RemoveDataNodeAtValue( currentRoot, currentDataNode, currentValue );

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
void qSlicerMultiDimensionModuleWidget::onHideSequenceNodeClicked( int row, int column )
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

  if ( column != SEQUENCE_NODE_VIS_COLUMN )
  {
    return;
  }

  // Get the selected nodes
  const char* currentValue = currentSequenceNode->GetAttribute( "MultiDimension.Value" );

  d->logic()->SetDataNodesHiddenAtValue( currentRoot, ! d->logic()->GetDataNodesHiddenAtValue( currentRoot, currentValue ), currentValue );

  // Change the eye for the current sequence node
  QTableWidgetItem* visItem = new QTableWidgetItem( QString( "" ) );
  this->CreateVisItem( visItem, d->logic()->GetDataNodesHiddenAtValue( currentRoot, currentValue ) );
  d->TableWidget_SequenceNodes->setItem( row, SEQUENCE_NODE_VIS_COLUMN, visItem ); // This changes current cell to NULL
  d->TableWidget_SequenceNodes->setCurrentCell( row, column ); // Reset the current cell so updating the sequence node works

  this->UpdateSequenceNode();
}



//-----------------------------------------------------------------------------
void qSlicerMultiDimensionModuleWidget::onHideDataNodeClicked( int row, int column )
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

  if ( column != DATA_NODE_VIS_COLUMN )
  {
    return;
  }
  
  // Get the selected nodes
  const char* currentValue = currentSequenceNode->GetAttribute( "MultiDimension.Value" );
  vtkSmartPointer<vtkCollection> currentDataNodes = vtkSmartPointer<vtkCollection>::New();
  d->logic()->GetDataNodesAtValue( currentDataNodes, currentRoot, currentValue );

  vtkMRMLNode* currentDataNode = vtkMRMLNode::SafeDownCast( currentDataNodes->GetItemAsObject( row ) );
  currentDataNode->SetHideFromEditors( ! currentDataNode->GetHideFromEditors() );
  qSlicerApplication::application()->layoutManager()->setMRMLScene( this->mrmlScene() );

  // Change the eye for the current sequence node
  int sequenceRow = d->TableWidget_SequenceNodes->currentRow();
  int sequenceColumn = d->TableWidget_SequenceNodes->currentColumn();
  QTableWidgetItem* visItem = new QTableWidgetItem( QString( "" ) );
  this->CreateVisItem( visItem, d->logic()->GetDataNodesHiddenAtValue( currentRoot, currentValue ) );
  d->TableWidget_SequenceNodes->setItem( d->TableWidget_SequenceNodes->currentRow(), SEQUENCE_NODE_VIS_COLUMN, visItem ); // This changes current cell to NULL
  d->TableWidget_SequenceNodes->setCurrentCell( sequenceRow, sequenceColumn ); // Reset the current cell so updating the sequence node works


  this->UpdateSequenceNode();
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
    d->LineEdit_ParameterName->setText( FROM_STD_STRING_SAFE( "" ) );
    d->LineEdit_ParameterUnit->setText( FROM_STD_STRING_SAFE( "" ) );
    d->TableWidget_SequenceNodes->clear();
    d->TableWidget_SequenceNodes->setRowCount( 0 );
    d->TableWidget_SequenceNodes->setColumnCount( 0 );
    return;
  }

  // Put the correct properties in the root node table
  d->LineEdit_ParameterName->setText( FROM_STD_STRING_SAFE( currentRoot->GetAttribute( "MultiDimension.Name" ) ) );
  d->LineEdit_ParameterUnit->setText( FROM_STD_STRING_SAFE( currentRoot->GetAttribute( "MultiDimension.Unit" ) ) );

  // Display all of the sequence nodes
  d->TableWidget_SequenceNodes->clear();
  d->TableWidget_SequenceNodes->setRowCount( currentRoot->GetNumberOfChildrenNodes() );
  d->TableWidget_SequenceNodes->setColumnCount( SEQUENCE_NODE_COLUMNS );
  std::stringstream valueHeader;
  valueHeader << currentRoot->GetAttribute( "MultiDimension.Name" ) << " (" << currentRoot->GetAttribute( "MultiDimension.Unit" ) << ")";
  QStringList SequenceNodesTableHeader;
  SequenceNodesTableHeader.insert( SEQUENCE_NODE_VIS_COLUMN, "Vis" );
  SequenceNodesTableHeader.insert( SEQUENCE_NODE_NAME_COLUMN, "Name" );
  SequenceNodesTableHeader.insert( SEQUENCE_NODE_VALUE_COLUMN, valueHeader.str().c_str() );
  d->TableWidget_SequenceNodes->setHorizontalHeaderLabels( SequenceNodesTableHeader );

  for ( int i = 0; i < currentRoot->GetNumberOfChildrenNodes(); i++ )
  {
    vtkMRMLHierarchyNode* currentSequenceNode = currentRoot->GetNthChildNode( i );
    const char* currentValue = currentSequenceNode->GetAttribute( "MultiDimension.Value" );

    // Display the data node information
    QTableWidgetItem* visItem = new QTableWidgetItem( QString( "" ) );
    this->CreateVisItem( visItem, d->logic()->GetDataNodesHiddenAtValue( currentRoot, currentValue ) );

    QTableWidgetItem* nameItem = new QTableWidgetItem( FROM_STD_STRING_SAFE( currentSequenceNode->GetName() ) );
    QTableWidgetItem* valueItem = new QTableWidgetItem( FROM_STD_STRING_SAFE( currentValue ) );

    d->TableWidget_SequenceNodes->setItem( i, SEQUENCE_NODE_VIS_COLUMN, visItem );
    d->TableWidget_SequenceNodes->setItem( i, SEQUENCE_NODE_NAME_COLUMN, nameItem );
    d->TableWidget_SequenceNodes->setItem( i, SEQUENCE_NODE_VALUE_COLUMN, valueItem );
  }

  d->TableWidget_SequenceNodes->resizeColumnsToContents();  
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
    d->TableWidget_DataNodes->clear();
    d->TableWidget_DataNodes->setRowCount( 0 );
    d->TableWidget_DataNodes->setColumnCount( 0 );
    d->ListWidget_NonDataNodes->clear();
    return;
  }

  const char* currentValue = currentSequenceNode->GetAttribute( "MultiDimension.Value" );
  vtkSmartPointer<vtkCollection> currentDataNodes = vtkSmartPointer<vtkCollection>::New();
  d->logic()->GetDataNodesAtValue( currentDataNodes, currentRoot, currentValue );

  // Display all of the data nodes
  d->TableWidget_DataNodes->clear();
  d->TableWidget_DataNodes->setRowCount( currentDataNodes->GetNumberOfItems() );
  d->TableWidget_DataNodes->setColumnCount( DATA_NODE_COLUMNS );
  QStringList DataNodesTableHeader;
  DataNodesTableHeader.insert( DATA_NODE_VIS_COLUMN, "Vis" );
  DataNodesTableHeader.insert( DATA_NODE_NAME_COLUMN, "Name" );
  DataNodesTableHeader.insert( DATA_NODE_TYPE_COLUMN, "Type" );
  d->TableWidget_DataNodes->setHorizontalHeaderLabels( DataNodesTableHeader );

  for ( int i = 0; i < currentDataNodes->GetNumberOfItems(); i++ )
  {
    vtkMRMLNode* currentDataNode = vtkMRMLNode::SafeDownCast( currentDataNodes->GetItemAsObject( i ) );

    // Display the data node information
    QTableWidgetItem* visItem = new QTableWidgetItem( QString( "" ) );
    this->CreateVisItem( visItem, currentDataNode->GetHideFromEditors() );

    QTableWidgetItem* nameItem = new QTableWidgetItem( QString::fromStdString( currentDataNode->GetName() ) );

    QTableWidgetItem* typeItem = new QTableWidgetItem( QString::fromStdString( currentDataNode->GetClassName() ) );
    typeItem->setFlags( typeItem->flags() & ~Qt::ItemIsEditable );

    d->TableWidget_DataNodes->setItem( i, DATA_NODE_VIS_COLUMN, visItem );
    d->TableWidget_DataNodes->setItem( i, DATA_NODE_NAME_COLUMN, nameItem );
    d->TableWidget_DataNodes->setItem( i, DATA_NODE_TYPE_COLUMN, typeItem );
  }

  d->TableWidget_DataNodes->resizeColumnsToContents();
  d->TableWidget_DataNodes->resizeRowsToContents();


  // Display the non-data nodes
  vtkSmartPointer<vtkCollection> currentNonDataNodes = vtkSmartPointer<vtkCollection>::New();
  d->logic()->GetNonDataNodesAtValue( currentNonDataNodes, currentRoot, currentValue );
  d->ListWidget_NonDataNodes->clear();

  for ( int i = 0; i < currentNonDataNodes->GetNumberOfItems(); i++ )
  {
    vtkMRMLNode* currentNonDataNode = vtkMRMLNode::SafeDownCast( currentNonDataNodes->GetItemAsObject( i ));
    d->ListWidget_NonDataNodes->addItem( QString::fromStdString( currentNonDataNode->GetName() ) );
  }

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
void qSlicerMultiDimensionModuleWidget::CreateVisItem( QTableWidgetItem* visItem, bool hidden )
{
  Q_D(qSlicerMultiDimensionModuleWidget);

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
