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
#include <QMainWindow>
#include <QMenu>
#include <QSettings>
#include <QTimer>
#include <QtPlugin>

#include "qSlicerApplication.h"
#include "qSlicerCoreApplication.h"

#include "vtkMRMLScene.h"

// SequenceBrowser Logic includes
#include <vtkSlicerSequenceBrowserLogic.h>

// SequenceBrowser includes
#include "qMRMLSequenceBrowserToolBar.h"
#include "qSlicerSequenceBrowserModule.h"
#include "qSlicerSequenceBrowserModuleWidget.h"

#include "vtkMRMLSequenceBrowserNode.h"

static const double UPDATE_VIRTUAL_OUTPUT_NODES_PERIOD_SEC = 0.020; // refresh output with a maximum of 50FPS

//-----------------------------------------------------------------------------
Q_EXPORT_PLUGIN2(qSlicerSequenceBrowserModule, qSlicerSequenceBrowserModule);

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_ExtensionTemplate
class qSlicerSequenceBrowserModulePrivate
{
public:
  /// Adds sequence browser toolbar to the application GUI (toolbar and menu)
  virtual void addToolBar();

  qSlicerSequenceBrowserModulePrivate();
  QTimer UpdateAllVirtualOutputNodesTimer;
  qMRMLSequenceBrowserToolBar* ToolBar;
};

//-----------------------------------------------------------------------------
// qSlicerSequenceBrowserModulePrivate methods

//-----------------------------------------------------------------------------
qSlicerSequenceBrowserModulePrivate::qSlicerSequenceBrowserModulePrivate()
{
  this->ToolBar = new qMRMLSequenceBrowserToolBar;
  this->ToolBar->setWindowTitle(QObject::tr("Sequence browser"));
  this->ToolBar->setObjectName("SequenceBrowser");
  this->ToolBar->setVisible(false);
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModulePrivate::addToolBar()
{
  QMainWindow* mainWindow = qSlicerApplication::application()->mainWindow();
  if (mainWindow==NULL)
  {
    qDebug("qSlicerSequenceBrowserModulePrivate::addToolBar: no main window is available, toolbar is not added");
    return;
  }

  this->ToolBar->setWindowTitle("Sequence browser");
  this->ToolBar->setObjectName("SequenceBrowserToolBar");
  mainWindow->addToolBar(this->ToolBar);
  foreach (QMenu* toolBarMenu,mainWindow->findChildren<QMenu*>())
  {
    if(toolBarMenu->objectName()==QString("WindowToolBarsMenu"))
    {
      toolBarMenu->addAction(this->ToolBar->toggleViewAction());
      break;
    }
  }

  // Main window takes care of saving and restoring toolbar geometry and state.
  // However, when state is restored the sequence browser toolbar was not created yet.
  // We need to restore the main window state again, now, that the Sequences toolbar is available.
  QSettings settings;
  settings.beginGroup("MainWindow");
  bool restore = settings.value("RestoreGeometry", false).toBool();
  if (restore)
  {
    mainWindow->restoreState(settings.value("windowState").toByteArray());
  }
  settings.endGroup();
}


//-----------------------------------------------------------------------------
// qSlicerSequenceBrowserModule methods

//-----------------------------------------------------------------------------
qSlicerSequenceBrowserModule::qSlicerSequenceBrowserModule(QObject* _parent)
: Superclass(_parent)
, d_ptr(new qSlicerSequenceBrowserModulePrivate)
{
  Q_D(qSlicerSequenceBrowserModule);
  connect(&d->UpdateAllVirtualOutputNodesTimer, SIGNAL(timeout()), this, SLOT(updateAllVirtualOutputNodes()));
  vtkMRMLScene * scene = qSlicerCoreApplication::application()->mrmlScene();
  if (scene)
    {
    // Need to listen for any new sequence browser nodes being added to start/stop timer
    this->qvtkConnect(scene, vtkMRMLScene::NodeAddedEvent, this, SLOT(onNodeAddedEvent(vtkObject*,vtkObject*)));
    this->qvtkConnect(scene, vtkMRMLScene::NodeRemovedEvent, this, SLOT(onNodeRemovedEvent(vtkObject*,vtkObject*)));
    }
}

//-----------------------------------------------------------------------------
qSlicerSequenceBrowserModule::~qSlicerSequenceBrowserModule()
{
}

//-----------------------------------------------------------------------------
QString qSlicerSequenceBrowserModule::helpText()const
{
  return "This is a module for browsing or replaying a node sequence. See more information in the  <a href=\"http://www.slicer.org/slicerWiki/index.php/Documentation/Nightly/Extensions/Sequences\">online documentation</a>.";
}

//-----------------------------------------------------------------------------
QString qSlicerSequenceBrowserModule::acknowledgementText()const
{
  return "This work was funded by CCO ACRU and OCAIRO grants.";
}

//-----------------------------------------------------------------------------
QStringList qSlicerSequenceBrowserModule::contributors()const
{
  QStringList moduleContributors;
  moduleContributors << QString("Andras Lasso (PerkLab, Queen's), Matthew Holden (PerkLab, Queen's), Kevin Wang (PMH)");
  return moduleContributors;
}

//-----------------------------------------------------------------------------
QIcon qSlicerSequenceBrowserModule::icon()const
{
  return QIcon(":/Icons/SequenceBrowser.png");
}

//-----------------------------------------------------------------------------
QStringList qSlicerSequenceBrowserModule::categories() const
{
  return QStringList() << "Sequences";
}

//-----------------------------------------------------------------------------
QStringList qSlicerSequenceBrowserModule::dependencies() const
{
  return QStringList();
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModule::setup()
{
  Q_D(qSlicerSequenceBrowserModule);
  this->Superclass::setup();
  d->addToolBar();
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModule::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qSlicerSequenceBrowserModule);

  vtkMRMLScene* oldScene = this->mrmlScene();
  this->Superclass::setMRMLScene(scene);

  if (scene == NULL)
    {
    return;
    }

  // Need to listen for any new sequence browser nodes being added to start/stop timer
  this->qvtkReconnect(oldScene, scene, vtkMRMLScene::NodeAddedEvent, this, SLOT(onNodeAddedEvent(vtkObject*,vtkObject*)));
  this->qvtkReconnect(oldScene, scene, vtkMRMLScene::NodeRemovedEvent, this, SLOT(onNodeRemovedEvent(vtkObject*,vtkObject*)));

  d->ToolBar->setMRMLScene(scene);
}

//-----------------------------------------------------------------------------
qSlicerAbstractModuleRepresentation * qSlicerSequenceBrowserModule
::createWidgetRepresentation()
{
  return new qSlicerSequenceBrowserModuleWidget;
}

//-----------------------------------------------------------------------------
vtkMRMLAbstractLogic* qSlicerSequenceBrowserModule::createLogic()
{
  return vtkSlicerSequenceBrowserLogic::New();
}

// --------------------------------------------------------------------------
void qSlicerSequenceBrowserModule::onNodeAddedEvent(vtkObject*, vtkObject* node)
{
  Q_D(qSlicerSequenceBrowserModule);

  vtkMRMLSequenceBrowserNode* sequenceBrowserNode = vtkMRMLSequenceBrowserNode::SafeDownCast(node);
  if (sequenceBrowserNode)
    {
    // If the timer is not active
    if (!d->UpdateAllVirtualOutputNodesTimer.isActive())
      {
      d->UpdateAllVirtualOutputNodesTimer.start(UPDATE_VIRTUAL_OUTPUT_NODES_PERIOD_SEC*1000.0);
      }
    }
}

// --------------------------------------------------------------------------
void qSlicerSequenceBrowserModule::onNodeRemovedEvent(vtkObject*, vtkObject* node)
{
  Q_D(qSlicerSequenceBrowserModule);

  vtkMRMLSequenceBrowserNode* sequenceBrowserNode = vtkMRMLSequenceBrowserNode::SafeDownCast(node);
  if (sequenceBrowserNode)
    {
    // If the timer is active
    if (d->UpdateAllVirtualOutputNodesTimer.isActive())
      {
      // Check if there is any other sequence browser node left in the Scene
      vtkMRMLScene * scene = qSlicerCoreApplication::application()->mrmlScene();
      if (scene)
        {
        std::vector<vtkMRMLNode *> nodes;
        this->mrmlScene()->GetNodesByClass("vtkMRMLSequenceBrowserNode", nodes);
        if (nodes.size() == 0)
          {
          // The last sequence browser was removed
          d->UpdateAllVirtualOutputNodesTimer.stop();
          }
        }
      }
    }
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModule::updateAllVirtualOutputNodes()
{
  vtkMRMLAbstractLogic* l = this->logic();
  vtkSlicerSequenceBrowserLogic * sequenceBrowserLogic = vtkSlicerSequenceBrowserLogic::SafeDownCast(l);
  if (sequenceBrowserLogic)
    {
    sequenceBrowserLogic->UpdateAllVirtualOutputNodes();
    }
}

//-----------------------------------------------------------------------------
qMRMLSequenceBrowserToolBar* qSlicerSequenceBrowserModule::toolBar()
{
  Q_D(qSlicerSequenceBrowserModule);
  return d->ToolBar;
}


//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModule::setToolBarVisible(bool visible)
{
  Q_D(qSlicerSequenceBrowserModule);
  d->ToolBar->setVisible(visible);
}

//-----------------------------------------------------------------------------
bool qSlicerSequenceBrowserModule::isToolBarVisible()
{
  Q_D(qSlicerSequenceBrowserModule);
  return d->ToolBar->isVisible();
}

//-----------------------------------------------------------------------------
void qSlicerSequenceBrowserModule::setToolBarActiveBrowserNode(vtkMRMLSequenceBrowserNode* browserNode)
{
  Q_D(qSlicerSequenceBrowserModule);
  return d->ToolBar->setActiveBrowserNode(browserNode);
}
