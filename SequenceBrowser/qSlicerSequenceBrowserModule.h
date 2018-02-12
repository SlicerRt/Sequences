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

#ifndef __qSlicerSequenceBrowserModule_h
#define __qSlicerSequenceBrowserModule_h

// CTK includes
#include <ctkVTKObject.h>

// SlicerQt includes
#include "qSlicerLoadableModule.h"

#include "vtkSlicerConfigure.h" // For Slicer_HAVE_QT5

#include "qSlicerSequenceBrowserModuleExport.h"

class qSlicerSequenceBrowserModulePrivate;
class qMRMLSequenceBrowserToolBar;
class vtkMRMLScene;
class vtkMRMLSequenceBrowserNode;
class vtkObject;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class Q_SLICER_QTMODULES_SEQUENCEBROWSER_EXPORT
qSlicerSequenceBrowserModule
  : public qSlicerLoadableModule
{
  Q_OBJECT;
  QVTK_OBJECT;
#ifdef Slicer_HAVE_QT5	
  Q_PLUGIN_METADATA(IID "org.slicer.modules.loadable.qSlicerLoadableModule/1.0");	
#endif
  Q_INTERFACES(qSlicerLoadableModule);

  /// Visibility of the sequence browser toolbar
  Q_PROPERTY(bool toolBarVisible READ isToolBarVisible WRITE setToolBarVisible)
  Q_PROPERTY(bool autoShowToolBar READ autoShowToolBar WRITE setAutoShowToolBar)

public:

  typedef qSlicerLoadableModule Superclass;
  explicit qSlicerSequenceBrowserModule(QObject *parent=0);
  virtual ~qSlicerSequenceBrowserModule();

  qSlicerGetTitleMacro(QTMODULE_TITLE);

  virtual QString helpText()const;
  virtual QString acknowledgementText()const;
  virtual QStringList contributors()const;

  virtual QIcon icon()const;

  virtual QStringList categories()const;
  virtual QStringList dependencies() const;

  /// Enables automatic showing sequence browser toolbar when a new sequence is loaded
  Q_INVOKABLE bool autoShowToolBar();
  Q_INVOKABLE bool isToolBarVisible();
  Q_INVOKABLE qMRMLSequenceBrowserToolBar* toolBar();
  
protected:

  /// Initialize the module. Register the volumes reader/writer
  virtual void setup();

  /// Create and return the widget representation associated to this module
  virtual qSlicerAbstractModuleRepresentation * createWidgetRepresentation();

  /// Create and return the logic associated to this module
  virtual vtkMRMLAbstractLogic* createLogic();

public slots:
  virtual void setMRMLScene(vtkMRMLScene*);
  void setToolBarVisible(bool visible);
  /// Enables automatic showing sequence browser toolbar when a new sequence is loaded
  void setAutoShowToolBar(bool autoShow);
  void onNodeAddedEvent(vtkObject*, vtkObject*);
  void onNodeRemovedEvent(vtkObject*, vtkObject*);
  void updateAllVirtualOutputNodes();

  void setToolBarActiveBrowserNode(vtkMRMLSequenceBrowserNode* browserNode);

protected:
  QScopedPointer<qSlicerSequenceBrowserModulePrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(qSlicerSequenceBrowserModule);
  Q_DISABLE_COPY(qSlicerSequenceBrowserModule);

};

#endif
