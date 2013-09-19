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

#define FROM_STD_STRING_SAFE(unsafeString) QString::fromStdString( unsafeString==NULL?"":unsafeString )

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


/*
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
*/


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
  d->LineEdit_ParameterName->setText( FROM_STD_STRING_SAFE( currentRoot->GetAttribute( "MultiDimension.Name" ) ) );
  d->LineEdit_ParameterUnit->setText( FROM_STD_STRING_SAFE( currentRoot->GetAttribute( "MultiDimension.Unit" ) ) );

  // Display all of the sequence nodes
  d->TableWidget_SequenceNodes->clear();
  d->TableWidget_SequenceNodes->setRowCount( currentRoot->GetNumberOfChildrenNodes() );
  d->TableWidget_SequenceNodes->setColumnCount( 2 );
  std::stringstream valueHeader;
  valueHeader << currentRoot->GetAttribute( "MultiDimension.Name" ) << " (" << currentRoot->GetAttribute( "MultiDimension.Unit" ) << ")";
  QStringList SequenceNodesTableHeader;
  SequenceNodesTableHeader << "Name" << valueHeader.str().c_str();
  d->TableWidget_SequenceNodes->setHorizontalHeaderLabels( SequenceNodesTableHeader );
  d->TableWidget_SequenceNodes->horizontalHeader()->setResizeMode( QHeaderView::Stretch );

  for ( int i = 0; i < currentRoot->GetNumberOfChildrenNodes(); i++ )
  {
    vtkMRMLHierarchyNode* currentSequenceNode = currentRoot->GetNthChildNode( i );
    QTableWidgetItem* nameItem = new QTableWidgetItem( FROM_STD_STRING_SAFE( currentSequenceNode->GetName() ) );
    QTableWidgetItem* valueItem = new QTableWidgetItem( FROM_STD_STRING_SAFE( currentSequenceNode->GetAttribute( "MultiDimension.Value" ) ) );
    d->TableWidget_SequenceNodes->setItem( i, 0, nameItem );
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
  vtkSmartPointer<vtkCollection> currentDataNodes = vtkSmartPointer<vtkCollection>::New();
  d->logic()->GetDataNodesAtValue( currentDataNodes, currentRoot, currentValue );

  // Display all of the data nodes
  d->TableWidget_DataNodes->clear();
  d->TableWidget_DataNodes->setRowCount( currentDataNodes->GetNumberOfItems() );
  d->TableWidget_DataNodes->setColumnCount( 2 );
  QStringList SequenceNodesTableHeader;
  SequenceNodesTableHeader << "Name" << "Type";
  d->TableWidget_DataNodes->setHorizontalHeaderLabels( SequenceNodesTableHeader );
  d->TableWidget_DataNodes->horizontalHeader()->setResizeMode( QHeaderView::Stretch );

  for ( int i = 0; i < currentDataNodes->GetNumberOfItems(); i++ )
  {
    vtkMRMLNode* currentDataNode = vtkMRMLNode::SafeDownCast( currentDataNodes->GetItemAsObject( i ) );
    QTableWidgetItem* nameItem = new QTableWidgetItem( QString::fromStdString( currentDataNode->GetName() ) );
    QTableWidgetItem* typeItem = new QTableWidgetItem( QString::fromStdString( currentDataNode->GetClassName() ) );
    d->TableWidget_DataNodes->setItem( i, 0, nameItem );
    d->TableWidget_DataNodes->setItem( i, 1, typeItem );
  }

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