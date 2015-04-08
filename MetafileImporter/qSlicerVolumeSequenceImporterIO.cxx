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

==============================================================================*/

// Qt includes
#include <QDebug>

// SlicerQt includes
#include "qSlicerVolumeSequenceImporterIO.h"

// Logic includes
#include "vtkSlicerMetafileImporterLogic.h"

// MRML includes

// VTK includes
#include <vtkSmartPointer.h>

//-----------------------------------------------------------------------------
class qSlicerVolumeSequenceImporterIOPrivate
{
public:
  vtkSmartPointer<vtkSlicerMetafileImporterLogic> MetafileImporterLogic;
};

//-----------------------------------------------------------------------------
qSlicerVolumeSequenceImporterIO::qSlicerVolumeSequenceImporterIO( vtkSlicerMetafileImporterLogic* newMetafileImporterLogic, QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerVolumeSequenceImporterIOPrivate)
{
  this->setMetafileImporterLogic( newMetafileImporterLogic );
}

//-----------------------------------------------------------------------------
qSlicerVolumeSequenceImporterIO::~qSlicerVolumeSequenceImporterIO()
{
}

//-----------------------------------------------------------------------------
void qSlicerVolumeSequenceImporterIO::setMetafileImporterLogic(vtkSlicerMetafileImporterLogic* newMetafileImporterLogic)
{
  Q_D(qSlicerVolumeSequenceImporterIO);
  d->MetafileImporterLogic = newMetafileImporterLogic;
}

//-----------------------------------------------------------------------------
vtkSlicerMetafileImporterLogic* qSlicerVolumeSequenceImporterIO::metafileImporterLogic() const
{
  Q_D(const qSlicerVolumeSequenceImporterIO);
  return d->MetafileImporterLogic;
}

//-----------------------------------------------------------------------------
QString qSlicerVolumeSequenceImporterIO::description() const
{
  return "Volume Sequence";
}

//-----------------------------------------------------------------------------
qSlicerIO::IOFileType qSlicerVolumeSequenceImporterIO::fileType() const
{
  return QString("Volume Sequence");
}

//-----------------------------------------------------------------------------
QStringList qSlicerVolumeSequenceImporterIO::extensions() const
{
  return QStringList() << "Volume Sequence (*.nrrd *.nhdr)";
}

//-----------------------------------------------------------------------------
bool qSlicerVolumeSequenceImporterIO::load(const IOProperties& properties)
{
  Q_D(qSlicerVolumeSequenceImporterIO);
  if (!properties.contains("fileName"))
  {
    qCritical() << "qSlicerVolumeSequenceImporterIO::load did not receive fileName property";
  }
  QString fileName = properties["fileName"].toString();
  
  return d->MetafileImporterLogic->ReadVolumeSequence( fileName.toStdString() );
}
