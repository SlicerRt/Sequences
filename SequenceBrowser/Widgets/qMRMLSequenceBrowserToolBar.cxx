/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Julien Finet, Kitware Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Qt includes
#include <QDebug>
#include <QMenu>
#include <QInputDialog>
#include <QTimer>
#include <QToolButton>

// CTK includes
#include <ctkMessageBox.h>

// qMRML includes
#include "qMRMLSequenceBrowserToolBar.h"
#include "qMRMLSceneViewMenu.h"
#include "qMRMLNodeFactory.h"

// MRML includes
#include <vtkMRMLViewNode.h>
#include <vtkMRMLSceneViewNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

//-----------------------------------------------------------------------------
class qMRMLSequenceBrowserToolBarPrivate
{
  QVTK_OBJECT
  Q_DECLARE_PUBLIC(qMRMLSequenceBrowserToolBar);
protected:
  qMRMLSequenceBrowserToolBar* const q_ptr;
  bool timeOutFlag;
public:
  qMRMLSequenceBrowserToolBarPrivate(qMRMLSequenceBrowserToolBar& object);
  void init();
  void setMRMLScene(vtkMRMLScene* newScene);
  QAction*                         ScreenshotAction;
  QAction*                         SceneViewAction;
  qMRMLSceneViewMenu*              SceneViewMenu;

  // TODO In LayoutManager, use GetActive/IsActive flag ...
  vtkWeakPointer<vtkMRMLViewNode>  ActiveMRMLThreeDViewNode;
  vtkSmartPointer<vtkMRMLScene>    MRMLScene;

public slots:
  void OnMRMLSceneStartBatchProcessing();
  void OnMRMLSceneEndBatchProcessing();
  void updateWidgetFromMRML();
  void createSceneView();
};

//--------------------------------------------------------------------------
// qMRMLSequenceBrowserToolBarPrivate methods

//---------------------------------------------------------------------------
qMRMLSequenceBrowserToolBarPrivate::qMRMLSequenceBrowserToolBarPrivate(qMRMLSequenceBrowserToolBar& object)
  : q_ptr(&object)
{
  this->ScreenshotAction = 0;
  this->SceneViewAction = 0;
  this->SceneViewMenu = 0;
  this->timeOutFlag = false;
}

// --------------------------------------------------------------------------
void qMRMLSequenceBrowserToolBarPrivate::updateWidgetFromMRML()
{
  Q_Q(qMRMLSequenceBrowserToolBar);
  // Enable buttons
  q->setEnabled(this->MRMLScene != 0);
  this->ScreenshotAction->setEnabled(this->ActiveMRMLThreeDViewNode != 0);
}

//---------------------------------------------------------------------------
void qMRMLSequenceBrowserToolBarPrivate::init()
{
  Q_Q(qMRMLSequenceBrowserToolBar);

  // Screenshot button
  this->ScreenshotAction = new QAction(q);
  this->ScreenshotAction->setIcon(QIcon(":/Icons/ViewCapture.png"));
  this->ScreenshotAction->setText(q->tr("Screenshot"));
  this->ScreenshotAction->setToolTip(q->tr(
    "Capture a screenshot of the full layout, 3D view or slice views. Use File, Save to save the image. Edit in the Annotations module."));
  QObject::connect(this->ScreenshotAction, SIGNAL(triggered()),
                   q, SIGNAL(screenshotButtonClicked()));
  q->addAction(this->ScreenshotAction);

  // Scene View buttons
  this->SceneViewAction = new QAction(q);
  this->SceneViewAction->setIcon(QIcon(":/Icons/ViewCamera.png"));
  this->SceneViewAction->setText(q->tr("Scene view"));
  this->SceneViewAction->setToolTip(q->tr("Capture and name a scene view."));
  QObject::connect(this->SceneViewAction, SIGNAL(triggered()),
                   q, SIGNAL(sceneViewButtonClicked()));
  q->addAction(this->SceneViewAction);

  // Scene view menu
  QToolButton* sceneViewMenuButton = new QToolButton(q);
  sceneViewMenuButton->setText(q->tr("Restore view"));
  sceneViewMenuButton->setIcon(QIcon(":/Icons/ViewCameraSelect.png"));
  sceneViewMenuButton->setToolTip(QObject::tr("Restore or delete saved scene views."));
  this->SceneViewMenu = new qMRMLSceneViewMenu(sceneViewMenuButton);
  sceneViewMenuButton->setMenu(this->SceneViewMenu);
  sceneViewMenuButton->setPopupMode(QToolButton::InstantPopup);
  //QObject::connect(q, SIGNAL(mrmlSceneChanged(vtkMRMLScene*)),
  //                 this->SceneViewMenu, SLOT(setMRMLScene(vtkMRMLScene*)));
  q->addWidget(sceneViewMenuButton);
  QObject::connect(q, SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)),
                  sceneViewMenuButton,
                  SLOT(setToolButtonStyle(Qt::ToolButtonStyle)));
}
// --------------------------------------------------------------------------
void qMRMLSequenceBrowserToolBarPrivate::setMRMLScene(vtkMRMLScene* newScene)
{
  Q_Q(qMRMLSequenceBrowserToolBar);

  if (newScene == this->MRMLScene)
    {
    return;
    }
/*
  this->qvtkReconnect(this->MRMLScene, newScene, vtkMRMLScene::StartBatchProcessEvent,
                      this, SLOT(OnMRMLSceneStartBatchProcessing()));

  this->qvtkReconnect(this->MRMLScene, newScene, vtkMRMLScene::EndBatchProcessEvent,
                      this, SLOT(OnMRMLSceneEndBatchProcessing()));

*/

  this->MRMLScene = newScene;

  this->SceneViewMenu->setMRMLScene(newScene);

  // Update UI
  this->updateWidgetFromMRML();
}

// --------------------------------------------------------------------------
void qMRMLSequenceBrowserToolBarPrivate::OnMRMLSceneStartBatchProcessing()
{
  Q_Q(qMRMLSequenceBrowserToolBar);
  q->setEnabled(false);
}

// --------------------------------------------------------------------------
void qMRMLSequenceBrowserToolBarPrivate::OnMRMLSceneEndBatchProcessing()
{
  this->updateWidgetFromMRML();
}

// --------------------------------------------------------------------------
void qMRMLSequenceBrowserToolBarPrivate::createSceneView()
{
  Q_Q(qMRMLSequenceBrowserToolBar);

  // Ask user for a name
  bool ok = false;
  QString sceneViewName = QInputDialog::getText(q, QObject::tr("SceneView Name"),
                                                QObject::tr("SceneView Name:"), QLineEdit::Normal,
                                                "View", &ok);
  if (!ok || sceneViewName.isEmpty())
    {
    return;
    }

  // Create scene view
  qMRMLNodeFactory nodeFactory;
  nodeFactory.setMRMLScene(this->MRMLScene);
  nodeFactory.setBaseName("vtkMRMLSceneViewNode", sceneViewName);
  vtkMRMLNode * newNode = nodeFactory.createNode("vtkMRMLSceneViewNode");
  vtkMRMLSceneViewNode * newSceneViewNode = vtkMRMLSceneViewNode::SafeDownCast(newNode);
  newSceneViewNode->StoreScene();
}

// --------------------------------------------------------------------------
// qMRMLSequenceBrowserToolBar methods

// --------------------------------------------------------------------------
qMRMLSequenceBrowserToolBar::qMRMLSequenceBrowserToolBar(const QString& title, QWidget* parentWidget)
  :Superclass(title, parentWidget)
   , d_ptr(new qMRMLSequenceBrowserToolBarPrivate(*this))
{
  Q_D(qMRMLSequenceBrowserToolBar);
  d->init();
}

// --------------------------------------------------------------------------
qMRMLSequenceBrowserToolBar::qMRMLSequenceBrowserToolBar(QWidget* _parent)
  : Superclass(_parent)
  , d_ptr(new qMRMLSequenceBrowserToolBarPrivate(*this))
{
  Q_D(qMRMLSequenceBrowserToolBar);
  d->init();
}

//---------------------------------------------------------------------------
qMRMLSequenceBrowserToolBar::~qMRMLSequenceBrowserToolBar()
{
}

// --------------------------------------------------------------------------
void qMRMLSequenceBrowserToolBar::setMRMLScene(vtkMRMLScene* scene)
{
  Q_D(qMRMLSequenceBrowserToolBar);
  d->setMRMLScene(scene);
}

// --------------------------------------------------------------------------
void qMRMLSequenceBrowserToolBar::setActiveMRMLThreeDViewNode(
  vtkMRMLViewNode * newActiveMRMLThreeDViewNode)
{
  Q_D(qMRMLSequenceBrowserToolBar);
  d->ActiveMRMLThreeDViewNode = newActiveMRMLThreeDViewNode;
  d->updateWidgetFromMRML();
}

// --------------------------------------------------------------------------
bool qMRMLSequenceBrowserToolBar::popupsTimeOut() const
{
  Q_D(const qMRMLSequenceBrowserToolBar);

  return d->timeOutFlag;
}

// --------------------------------------------------------------------------
void qMRMLSequenceBrowserToolBar::setPopupsTimeOut(bool flag)
{
  Q_D(qMRMLSequenceBrowserToolBar);

  d->timeOutFlag = flag;
}
