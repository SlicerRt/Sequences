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

// SlicerQt includes
#include "qSlicerMetafileImporterIO.h"
#include "qSlicerMetafileImporterModule.h"

// Logic includes
#include "vtkSlicerMetafileImporterLogic.h"

// MRML includes

// VTK includes
#include <vtkSmartPointer.h>

//-----------------------------------------------------------------------------
class qSlicerMetafileImporterIOPrivate
{
public:
  vtkSmartPointer<vtkSlicerMetafileImporterLogic> MetafileImporterLogic;
};

//-----------------------------------------------------------------------------
qSlicerMetafileImporterIO::qSlicerMetafileImporterIO( vtkSlicerMetafileImporterLogic* newMetafileImporterLogic, QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerMetafileImporterIOPrivate)
{
  this->setMetafileImporterLogic( newMetafileImporterLogic );
}

//-----------------------------------------------------------------------------
qSlicerMetafileImporterIO::~qSlicerMetafileImporterIO()
{
}

//-----------------------------------------------------------------------------
void qSlicerMetafileImporterIO::setMetafileImporterLogic(vtkSlicerMetafileImporterLogic* newMetafileImporterLogic)
{
  Q_D(qSlicerMetafileImporterIO);
  d->MetafileImporterLogic = newMetafileImporterLogic;
}

//-----------------------------------------------------------------------------
vtkSlicerMetafileImporterLogic* qSlicerMetafileImporterIO::MetafileImporterLogic() const
{
  Q_D(const qSlicerMetafileImporterIO);
  return d->MetafileImporterLogic;
}

//-----------------------------------------------------------------------------
QString qSlicerMetafileImporterIO::description() const
{
  return "Sequence Metafile";
}

//-----------------------------------------------------------------------------
qSlicerIO::IOFileType qSlicerMetafileImporterIO::fileType() const
{
  return QString("Sequence Metafile");
}

//-----------------------------------------------------------------------------
QStringList qSlicerMetafileImporterIO::extensions() const
{
  return QStringList() << "Sequence Metafile (*.seq.mha *.seq.mhd *.mha *.mhd)";
}

//-----------------------------------------------------------------------------
bool qSlicerMetafileImporterIO::load(const IOProperties& properties)
{
  Q_D(qSlicerMetafileImporterIO);
  if (!properties.contains("fileName"))
  {
    qCritical() << "qSlicerMetafileImporterIO::load did not receive fileName property";
  }
  QString fileName = properties["fileName"].toString();
  
  vtkMRMLSequenceBrowserNode* browserNode = NULL;
  bool success = d->MetafileImporterLogic->ReadSequenceMetafile( fileName.toStdString(), &browserNode );
  if (success && browserNode!=NULL)
  {
    qSlicerMetafileImporterModule::showSequenceBrowser(browserNode);
  }

  return success;
}
