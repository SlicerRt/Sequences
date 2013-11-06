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

// MultidimData includes
#include "vtkSlicerMultidimDataLogic.h"
#include "vtkMRMLMultidimDataNode.h"

#define FROM_STD_STRING_SAFE(unsafeString) QString::fromStdString( unsafeString==NULL?"":unsafeString )
#define FROM_ATTRIBUTE_SAFE(unsafeString) ( unsafeString==NULL?"":unsafeString )

enum
{
  SEQUENCE_NODE_VIS_COLUMN=0,
  SEQUENCE_NODE_VALUE_COLUMN,
  SEQUENCE_NODE_NUMBER_OF_COLUMNS // this must be the last line in this enum
};

enum
{
  DATA_NODE_VIS_COLUMN = 0,
  DATA_NODE_NAME_COLUMN,
  DATA_NODE_ROLE_COLUMN,
  DATA_NODE_TYPE_COLUMN,
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
  void GetNonDataNodesAtValue(vtkCollection* foundNodes, vtkMRMLMultidimDataNode* rootNode, const char* parameterValue);
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
void qSlicerMultidimDataModuleWidgetPrivate::GetNonDataNodesAtValue(vtkCollection* foundNodes, vtkMRMLMultidimDataNode* rootNode, const char* parameterValue)
{
  Q_Q( const qSlicerMultidimDataModuleWidget );

  if (foundNodes==NULL)
  {
    qCritical() << "GetNonDataNodesAtValue failed, invalid output collection"; 
    return;
  }
  foundNodes->RemoveAllItems();
  if (rootNode==NULL)
  {
    qCritical() << "GetNonDataNodesAtValue failed, invalid root node"; 
    return;
  }
  if (parameterValue==NULL)
  {
    qCritical() << "GetNonDataNodesAtValue failed, invalid parameter value"; 
    return;
  }

  // Return all of the nodes in the 
  vtkSmartPointer<vtkCollection> dataNodeCollection = vtkSmartPointer<vtkCollection>::New();
  rootNode->GetDataNodesAtValue( dataNodeCollection, parameterValue );    

  for ( int i = 0; i < rootNode->GetScene()->GetNumberOfNodes(); i++ )
  {
    vtkMRMLNode* currentNode = vtkMRMLNode::SafeDownCast( rootNode->GetScene()->GetNthNode( i ) );
    
    if (currentNode->GetHideFromEditors())
    {
      // don't show hidden nodes, they would clutter the view
      continue;
    }
    
    bool alreadyInMultidimData=false;
    for ( int j = 0; j < dataNodeCollection->GetNumberOfItems(); j++ )
    {
      vtkMRMLNode* testDataNode = vtkMRMLNode::SafeDownCast( dataNodeCollection->GetItemAsObject( j ) );
      if ( currentNode==testDataNode)
      {
        // already in the multidimensional data bundle
        alreadyInMultidimData=true;
        break;
      }
    }

    if (!alreadyInMultidimData)
    {
      foundNodes->AddItem( currentNode );
    }
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
  connect( d->TableWidget_SequenceNodes, SIGNAL( currentBundleChanged( int, int, int, int ) ), this, SLOT( onSequenceNodeChanged() ) );

  connect( d->LineEdit_ParameterName, SIGNAL( textEdited( const QString & ) ), this, SLOT( onParameterNameEdited() ) );
  connect( d->LineEdit_ParameterUnit, SIGNAL( textEdited( const QString & ) ), this, SLOT( onParameterUnitEdited() ) );

  connect( d->TableWidget_SequenceNodes, SIGNAL( bundleChanged( int, int ) ), this, SLOT( onSequenceNodeEdited( int, int ) ) );
  connect( d->TableWidget_SequenceNodes, SIGNAL( bundleClicked( int, int ) ), this, SLOT( onHideSequenceNodeClicked( int, int ) ) );
  connect( d->TableWidget_DataNodes, SIGNAL( bundleChanged( int, int ) ), this, SLOT( onDataNodeEdited( int, int ) ) );
  connect( d->TableWidget_DataNodes, SIGNAL( bundleClicked( int, int ) ), this, SLOT( onHideDataNodeClicked( int, int ) ) );

  connect( d->PushButton_AddDataNode, SIGNAL( clicked() ), this, SLOT( onAddDataNodeButtonClicked() ) );
  d->PushButton_AddDataNode->setIcon( QApplication::style()->standardIcon( QStyle::SP_ArrowLeft ) );
  connect( d->PushButton_RemoveDataNode, SIGNAL( clicked() ), this, SLOT( onRemoveDataNodeButtonClicked() ) );
  d->PushButton_RemoveDataNode->setIcon( QApplication::style()->standardIcon( QStyle::SP_ArrowRight ) );

  connect( d->PushButton_AddSequenceNode, SIGNAL( clicked() ), this, SLOT( onAddSequenceNodeButtonClicked() ) );
  d->PushButton_AddSequenceNode->setIcon( QIcon( ":/Icons/SequenceNodeAdd.png" ) );
  connect( d->PushButton_RemoveSequenceNode, SIGNAL( clicked() ), this, SLOT( onRemoveSequenceNodeButtonClicked() ) );
  d->PushButton_RemoveSequenceNode->setIcon( QIcon( ":/Icons/SequenceNodeDelete.png" ) );

  d->ExpandButton_DataNodes->setChecked( false );
}


//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::onRootNodeChanged()
{
  Q_D(qSlicerMultidimDataModuleWidget);

  this->UpdateRootNode();
}



//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::onSequenceNodeChanged()
{
  Q_D(qSlicerMultidimDataModuleWidget);

  this->UpdateSequenceNode();
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

  currentRoot->SetDimensionName(d->LineEdit_ParameterName->text().toLatin1().constData() );

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
void qSlicerMultidimDataModuleWidget::onSequenceNodeEdited( int row, int column )
{
  Q_D(qSlicerMultidimDataModuleWidget);

  // Ensure that the user is editing, not the bundle changed programmatically
  if ( d->TableWidget_SequenceNodes->currentRow() != row || d->TableWidget_SequenceNodes->currentColumn() != column )
  {
    return;
  }

  vtkMRMLMultidimDataNode* currentRoot = vtkMRMLMultidimDataNode::SafeDownCast( d->MRMLNodeComboBox_MultidimDataRoot->currentNode() );
  if ( currentRoot == NULL )
  {
    return;
  }

  std::string currentParameterValue = currentRoot->GetNthParameterValue( d->TableWidget_SequenceNodes->currentRow() );
  if ( currentParameterValue.empty() )
  {
    return;
  }

  // Grab the tex from the modified item
  QTableWidgetItem* qItem = d->TableWidget_SequenceNodes->item( row, column );
  QString qText = qItem->text();

  if ( column == SEQUENCE_NODE_VALUE_COLUMN )
  {
    currentRoot->UpdateParameterValue( currentParameterValue.c_str(), qText.toLatin1().constData() );
    // TODO: Should this be propagated to data node names?
  }

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

  std::string currentParameterValue = currentRoot->GetNthParameterValue( d->TableWidget_SequenceNodes->currentRow() );
  if ( currentParameterValue.empty() )
  {
    return;
  }

  // Get the data node
  vtkSmartPointer<vtkCollection> currentDataNodes = vtkSmartPointer<vtkCollection>::New();
  currentRoot->GetDataNodesAtValue( currentDataNodes, currentParameterValue.c_str() );
  vtkMRMLNode* currentDataNode = vtkMRMLNode::SafeDownCast( currentDataNodes->GetItemAsObject( row ) );

  // Grab the text from the modified item
  QTableWidgetItem* qItem = d->TableWidget_DataNodes->item( row, column );
  QString qText = qItem->text();

  if ( column == DATA_NODE_NAME_COLUMN )
  {
    currentDataNode->SetName( qText.toLatin1().constData() );
  }

  if ( column == DATA_NODE_ROLE_COLUMN )
  {
    currentRoot->SetDataNodeAtValue(currentDataNode, qText.toLatin1().constData(), currentParameterValue.c_str());
  }

  this->UpdateSequenceNode();
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

  std::string currentParameterValue = currentRoot->GetNthParameterValue( d->TableWidget_SequenceNodes->currentRow() );
  if ( currentParameterValue.empty() )
  {
    return;
  }

  // Get the selected nodes
  vtkSmartPointer<vtkCollection> currentNonDataNodes = vtkSmartPointer<vtkCollection>::New();
  d->GetNonDataNodesAtValue( currentNonDataNodes, currentRoot, currentParameterValue.c_str() );

  int row = d->ListWidget_NonDataNodes->currentRow();
  vtkMRMLNode* currentNonDataNode = vtkMRMLNode::SafeDownCast( currentNonDataNodes->GetItemAsObject( row ) );
  currentRoot->SetDataNodeAtValue(currentNonDataNode, currentNonDataNode->GetName(), currentParameterValue.c_str() );

  this->UpdateSequenceNode();
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

  std::string currentParameterValue = currentRoot->GetNthParameterValue( d->TableWidget_SequenceNodes->currentRow() );
  if ( currentParameterValue.empty() )
  {
    return;
  }

  // Get the selected nodes
  vtkSmartPointer<vtkCollection> currentDataNodes = vtkSmartPointer<vtkCollection>::New();
  currentRoot->GetDataNodesAtValue( currentDataNodes, currentParameterValue.c_str() );

  int row = d->TableWidget_DataNodes->currentRow();
  vtkMRMLNode* currentDataNode = vtkMRMLNode::SafeDownCast( currentDataNodes->GetItemAsObject( row ) );
  currentRoot->RemoveDataNodeAtValue(currentDataNode, currentParameterValue.c_str() );

  this->UpdateSequenceNode();
}



//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::onRemoveSequenceNodeButtonClicked()
{
  Q_D(qSlicerMultidimDataModuleWidget);

  vtkMRMLMultidimDataNode* currentRoot = vtkMRMLMultidimDataNode::SafeDownCast( d->MRMLNodeComboBox_MultidimDataRoot->currentNode() );
  if ( currentRoot == NULL )
  {
    return;
  }

  std::string currentParameterValue = currentRoot->GetNthParameterValue( d->TableWidget_SequenceNodes->currentRow() );
  if ( currentParameterValue.empty() )
  {
    return;
  }

  // Get the selected nodes


  currentRoot->RemoveAllDataNodesAtValue( currentParameterValue.c_str() );

  this->UpdateRootNode();
}


//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::onHideSequenceNodeClicked( int row, int column )
{
  Q_D(qSlicerMultidimDataModuleWidget);

  vtkMRMLMultidimDataNode* currentRoot = vtkMRMLMultidimDataNode::SafeDownCast( d->MRMLNodeComboBox_MultidimDataRoot->currentNode() );
  if ( currentRoot == NULL )
  {
    return;
  }

  std::string currentParameterValue = currentRoot->GetNthParameterValue( d->TableWidget_SequenceNodes->currentRow() );
  if ( currentParameterValue.empty() )
  {
    return;
  }

  if ( column != SEQUENCE_NODE_VIS_COLUMN )
  {
    return;
  }

  // Get the selected nodes

  d->logic()->SetDataNodesHiddenAtValue( currentRoot, ! d->logic()->GetDataNodesHiddenAtValue( currentRoot, currentParameterValue.c_str() ), currentParameterValue.c_str() );

  // Change the eye for the current sequence node
  QTableWidgetItem* visItem = new QTableWidgetItem( QString( "" ) );
  this->CreateVisItem( visItem, d->logic()->GetDataNodesHiddenAtValue( currentRoot, currentParameterValue.c_str() ) );
  d->TableWidget_SequenceNodes->setItem( row, SEQUENCE_NODE_VIS_COLUMN, visItem ); // This changes current bundle to NULL
  d->TableWidget_SequenceNodes->setCurrentCell( row, column ); // Reset the current cell so updating the sequence node works

  this->UpdateSequenceNode();
}



//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::onHideDataNodeClicked( int row, int column )
{
  Q_D(qSlicerMultidimDataModuleWidget);

  vtkMRMLMultidimDataNode* currentRoot = vtkMRMLMultidimDataNode::SafeDownCast( d->MRMLNodeComboBox_MultidimDataRoot->currentNode() );
  if ( currentRoot == NULL )
  {
    return;
  }

  std::string currentParameterValue = currentRoot->GetNthParameterValue( d->TableWidget_SequenceNodes->currentRow() );
  if ( currentParameterValue.empty() )
  {
    return;
  }

  if ( column != DATA_NODE_VIS_COLUMN )
  {
    return;
  }
  
  // Get the selected nodes
  vtkSmartPointer<vtkCollection> currentDataNodes = vtkSmartPointer<vtkCollection>::New();
  currentRoot->GetDataNodesAtValue( currentDataNodes, currentParameterValue.c_str() );

  vtkMRMLNode* currentDataNode = vtkMRMLNode::SafeDownCast( currentDataNodes->GetItemAsObject( row ) );
  currentDataNode->SetHideFromEditors( ! currentDataNode->GetHideFromEditors() );
  qSlicerApplication::application()->layoutManager()->setMRMLScene( this->mrmlScene() );

  // Change the eye for the current sequence node
  int sequenceRow = d->TableWidget_SequenceNodes->currentRow();
  int sequenceColumn = d->TableWidget_SequenceNodes->currentColumn();
  QTableWidgetItem* visItem = new QTableWidgetItem( QString( "" ) );
  this->CreateVisItem( visItem, d->logic()->GetDataNodesHiddenAtValue( currentRoot, currentParameterValue.c_str() ) );
  d->TableWidget_SequenceNodes->setItem( d->TableWidget_SequenceNodes->currentRow(), SEQUENCE_NODE_VIS_COLUMN, visItem ); // This changes current bundle to NULL
  d->TableWidget_SequenceNodes->setCurrentCell( sequenceRow, sequenceColumn ); // Reset the current cell so updating the sequence node works

  this->UpdateSequenceNode();
}



//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::onAddSequenceNodeButtonClicked()
{
  Q_D(qSlicerMultidimDataModuleWidget);

  vtkMRMLMultidimDataNode* currentRoot = vtkMRMLMultidimDataNode::SafeDownCast( d->MRMLNodeComboBox_MultidimDataRoot->currentNode() );
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

  currentRoot->AddBundle(value.toStdString().c_str() );

  this->UpdateRootNode();
}


//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::UpdateRootNode()
{
  Q_D(qSlicerMultidimDataModuleWidget);

  vtkMRMLMultidimDataNode* currentRoot = vtkMRMLMultidimDataNode::SafeDownCast( d->MRMLNodeComboBox_MultidimDataRoot->currentNode() );

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
  d->LineEdit_ParameterName->setText( FROM_STD_STRING_SAFE( currentRoot->GetDimensionName() ) );
  d->LineEdit_ParameterUnit->setText( FROM_STD_STRING_SAFE( currentRoot->GetUnit() ) );

  // Display all of the sequence nodes
  d->TableWidget_SequenceNodes->clear();
  d->TableWidget_SequenceNodes->setRowCount( currentRoot->GetNumberOfBundles() );
  d->TableWidget_SequenceNodes->setColumnCount( SEQUENCE_NODE_NUMBER_OF_COLUMNS );
  std::stringstream valueHeader;
  valueHeader << FROM_ATTRIBUTE_SAFE( currentRoot->GetDimensionName() ); 
  valueHeader << " (" << FROM_ATTRIBUTE_SAFE( currentRoot->GetUnit() ) << ")";
  QStringList SequenceNodesTableHeader;
  SequenceNodesTableHeader.insert( SEQUENCE_NODE_VIS_COLUMN, "Vis" );
  SequenceNodesTableHeader.insert( SEQUENCE_NODE_VALUE_COLUMN, valueHeader.str().c_str() );
  d->TableWidget_SequenceNodes->setHorizontalHeaderLabels( SequenceNodesTableHeader );

  for ( int i = 0; i < currentRoot->GetNumberOfBundles(); i++ )
  {
    std::string currentValue = currentRoot->GetNthParameterValue( i );

    // Display the data node information
    QTableWidgetItem* visItem = new QTableWidgetItem( QString( "" ) );
    this->CreateVisItem( visItem, d->logic()->GetDataNodesHiddenAtValue( currentRoot, currentValue.c_str() ) );

    QTableWidgetItem* valueItem = new QTableWidgetItem( FROM_STD_STRING_SAFE( currentValue.c_str() ) );

    d->TableWidget_SequenceNodes->setItem( i, SEQUENCE_NODE_VIS_COLUMN, visItem );
    d->TableWidget_SequenceNodes->setItem( i, SEQUENCE_NODE_VALUE_COLUMN, valueItem );
  }

  d->TableWidget_SequenceNodes->resizeColumnsToContents();  
  d->TableWidget_SequenceNodes->resizeRowsToContents();  

  this->UpdateSequenceNode();
}


//-----------------------------------------------------------------------------
void qSlicerMultidimDataModuleWidget::UpdateSequenceNode()
{
  Q_D(qSlicerMultidimDataModuleWidget);

  vtkMRMLMultidimDataNode* currentRoot = vtkMRMLMultidimDataNode::SafeDownCast( d->MRMLNodeComboBox_MultidimDataRoot->currentNode() );
  if ( currentRoot == NULL )
  {
    return;
  }

  std::string currentParameterValue = currentRoot->GetNthParameterValue( d->TableWidget_SequenceNodes->currentRow() );
  if ( currentParameterValue.empty() )  
  {
    d->TableWidget_DataNodes->clear();
    d->TableWidget_DataNodes->setRowCount( 0 );
    d->TableWidget_DataNodes->setColumnCount( 0 );
    d->ListWidget_NonDataNodes->clear();
    return;
  }

  vtkSmartPointer<vtkCollection> currentDataNodes = vtkSmartPointer<vtkCollection>::New();
  currentRoot->GetDataNodesAtValue( currentDataNodes, currentParameterValue.c_str() );

  // Display all of the data nodes
  d->TableWidget_DataNodes->clear();
  d->TableWidget_DataNodes->setRowCount( currentDataNodes->GetNumberOfItems() );
  d->TableWidget_DataNodes->setColumnCount( DATA_NODE_NUMBER_OF_COLUMNS );
  QStringList DataNodesTableHeader;
  DataNodesTableHeader.insert( DATA_NODE_VIS_COLUMN, "Vis" );
  DataNodesTableHeader.insert( DATA_NODE_NAME_COLUMN, "Name" );
  DataNodesTableHeader.insert( DATA_NODE_ROLE_COLUMN, "Role" );
  DataNodesTableHeader.insert( DATA_NODE_TYPE_COLUMN, "Type" );
  d->TableWidget_DataNodes->setHorizontalHeaderLabels( DataNodesTableHeader );

  for ( int i = 0; i < currentDataNodes->GetNumberOfItems(); i++ )
  {
    vtkMRMLNode* currentDataNode = vtkMRMLNode::SafeDownCast( currentDataNodes->GetItemAsObject( i ) );

    // Display the data node information
    QTableWidgetItem* visItem = new QTableWidgetItem( QString( "" ) );
    this->CreateVisItem( visItem, currentDataNode->GetHideFromEditors() );

    QTableWidgetItem* nameItem = new QTableWidgetItem( FROM_STD_STRING_SAFE( currentDataNode->GetName() ) );

    std::string role = currentRoot->GetDataNodeRoleAtValue( currentDataNode, currentParameterValue.c_str() );
    QTableWidgetItem* roleItem = new QTableWidgetItem( role.c_str() );

    QTableWidgetItem* typeItem = new QTableWidgetItem( FROM_STD_STRING_SAFE( currentDataNode->GetClassName() ) );
    typeItem->setFlags( typeItem->flags() & ~Qt::ItemIsEditable );

    d->TableWidget_DataNodes->setItem( i, DATA_NODE_VIS_COLUMN, visItem );
    d->TableWidget_DataNodes->setItem( i, DATA_NODE_NAME_COLUMN, nameItem );
    d->TableWidget_DataNodes->setItem( i, DATA_NODE_ROLE_COLUMN, roleItem );
    d->TableWidget_DataNodes->setItem( i, DATA_NODE_TYPE_COLUMN, typeItem );
  }

  d->TableWidget_DataNodes->resizeColumnsToContents();
  d->TableWidget_DataNodes->resizeRowsToContents();


  // Display the non-data nodes
  vtkSmartPointer<vtkCollection> currentNonDataNodes = vtkSmartPointer<vtkCollection>::New();
  d->GetNonDataNodesAtValue( currentNonDataNodes, currentRoot, currentParameterValue.c_str() );
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
