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

#ifndef __qSlicerMultidimDataBrowserModuleWidget_h
#define __qSlicerMultidimDataBrowserModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerMultidimDataBrowserModuleExport.h"

class qSlicerMultidimDataBrowserModuleWidgetPrivate;
class vtkMRMLMultidimDataNode;
class vtkMRMLMultidimDataBrowserNode;
class vtkMRMLNode;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class Q_SLICER_QTMODULES_MULTIDIMDATABROWSER_EXPORT qSlicerMultidimDataBrowserModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:

  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerMultidimDataBrowserModuleWidget(QWidget *parent=0);
  virtual ~qSlicerMultidimDataBrowserModuleWidget();

  void setActiveBrowserNode(vtkMRMLMultidimDataBrowserNode* browserNode);
  void setMultidimDataRootNode(vtkMRMLMultidimDataNode* multidimDataRootNode);

public slots:
  void setSelectedBundleIndex(int bundleIndex);
  void setPlaybackEnabled(bool play);
  void setPlaybackRateFps(double playbackRateFps);
  void setPlaybackLoopEnabled(bool loopEnabled);  

protected slots:
  void activeBrowserNodeChanged(vtkMRMLNode* node);
  void multidimDataRootNodeChanged(vtkMRMLNode*);
  void onMRMLInputMultidimDataInputNodeModified(vtkObject* caller);
  void onActiveBrowserNodeModified(vtkObject* caller);
  void onVcrFirst();
  void onVcrPrevious();
  void onVcrNext();
  void onVcrLast();
  void synchronizedRootNodeCheckStateChanged(int aState);

protected:
  void updateWidgetFromMRML();

  /// Refresh synchronized root nodes table from MRML
  void refreshSynchronizedRootNodesTable();

  QScopedPointer<qSlicerMultidimDataBrowserModuleWidgetPrivate> d_ptr;
  
  virtual void setup();  
  virtual void enter();

private:
  Q_DECLARE_PRIVATE(qSlicerMultidimDataBrowserModuleWidget);
  Q_DISABLE_COPY(qSlicerMultidimDataBrowserModuleWidget);
};

#endif
