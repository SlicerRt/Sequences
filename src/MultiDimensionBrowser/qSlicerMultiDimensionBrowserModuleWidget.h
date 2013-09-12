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

#ifndef __qSlicerMultiDimensionBrowserModuleWidget_h
#define __qSlicerMultiDimensionBrowserModuleWidget_h

// SlicerQt includes
#include "qSlicerAbstractModuleWidget.h"

#include "qSlicerMultiDimensionBrowserModuleExport.h"

class qSlicerMultiDimensionBrowserModuleWidgetPrivate;
class vtkMRMLHierarchyNode;
class vtkMRMLMultiDimensionBrowserNode;
class vtkMRMLNode;

/// \ingroup Slicer_QtModules_ExtensionTemplate
class Q_SLICER_QTMODULES_MULTIDIMENSIONBROWSER_EXPORT qSlicerMultiDimensionBrowserModuleWidget :
  public qSlicerAbstractModuleWidget
{
  Q_OBJECT

public:

  typedef qSlicerAbstractModuleWidget Superclass;
  qSlicerMultiDimensionBrowserModuleWidget(QWidget *parent=0);
  virtual ~qSlicerMultiDimensionBrowserModuleWidget();

  void setActiveBrowserNode(vtkMRMLMultiDimensionBrowserNode* browserNode);
  void setMultiDimensionRootNode(vtkMRMLHierarchyNode* multiDimensionRootNode);
  void setVirtualOutputNode(vtkMRMLHierarchyNode* virtualOutputNode);  
  void setParameterValueIndex(int paramValue);

public slots:

protected slots:
  void activeBrowserNodeChanged(vtkMRMLNode* node);
  void multiDimensionRootNodeChanged(vtkMRMLNode*);
  void virtualOutputNodeChanged(vtkMRMLNode*);
  void parameterValueIndexChanged(int);
  void onMRMLInputMultiDimensionInputNodeModified(vtkObject* caller);
  void onActiveBrowserNodeModified(vtkObject* caller);

protected:
  void updateWidgetFromMRML();

  QScopedPointer<qSlicerMultiDimensionBrowserModuleWidgetPrivate> d_ptr;
  
  virtual void setup();

private:
  Q_DECLARE_PRIVATE(qSlicerMultiDimensionBrowserModuleWidget);
  Q_DISABLE_COPY(qSlicerMultiDimensionBrowserModuleWidget);
};

#endif
