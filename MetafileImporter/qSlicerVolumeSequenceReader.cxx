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
#include "qSlicerMetafileImporterModule.h"
#include "qSlicerVolumeSequenceReader.h"

// Logic includes
#include "vtkSlicerMetafileImporterLogic.h"

// MRML includes
#include "vtkMRMLSequenceBrowserNode.h"

// VTK includes
#include <vtkSmartPointer.h>

//-----------------------------------------------------------------------------
class qSlicerVolumeSequenceReaderPrivate
{
public:
  vtkSmartPointer<vtkSlicerMetafileImporterLogic> MetafileImporterLogic;
};

//-----------------------------------------------------------------------------
qSlicerVolumeSequenceReader::qSlicerVolumeSequenceReader( vtkSlicerMetafileImporterLogic* newMetafileImporterLogic, QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerVolumeSequenceReaderPrivate)
{
  this->setMetafileImporterLogic( newMetafileImporterLogic );
}

//-----------------------------------------------------------------------------
qSlicerVolumeSequenceReader::~qSlicerVolumeSequenceReader()
{
}

//-----------------------------------------------------------------------------
void qSlicerVolumeSequenceReader::setMetafileImporterLogic(vtkSlicerMetafileImporterLogic* newMetafileImporterLogic)
{
  Q_D(qSlicerVolumeSequenceReader);
  d->MetafileImporterLogic = newMetafileImporterLogic;
}

//-----------------------------------------------------------------------------
vtkSlicerMetafileImporterLogic* qSlicerVolumeSequenceReader::metafileImporterLogic() const
{
  Q_D(const qSlicerVolumeSequenceReader);
  return d->MetafileImporterLogic;
}

//-----------------------------------------------------------------------------
QString qSlicerVolumeSequenceReader::description() const
{
  return "Volume Sequence";
}

//-----------------------------------------------------------------------------
qSlicerIO::IOFileType qSlicerVolumeSequenceReader::fileType() const
{
  return QString("Volume Sequence");
}

//-----------------------------------------------------------------------------
QStringList qSlicerVolumeSequenceReader::extensions() const
{
  return QStringList() << "Volume Sequence (*.seq.nrrd *.seq.nhdr)" << "Volume Sequence (*.nrrd *.nhdr)";
}

//-----------------------------------------------------------------------------
bool qSlicerVolumeSequenceReader::load(const IOProperties& properties)
{
  Q_D(qSlicerVolumeSequenceReader);
  if (!properties.contains("fileName"))
  {
    qCritical() << "qSlicerVolumeSequenceReader::load did not receive fileName property";
  }
  QString fileName = properties["fileName"].toString();

  vtkNew<vtkCollection> loadedSequenceNodes;  
  vtkMRMLSequenceBrowserNode* browserNode = d->MetafileImporterLogic->ReadVolumeSequence(fileName.toStdString(), loadedSequenceNodes.GetPointer());
  if (browserNode == NULL)
  {
    return false;
  }

  QStringList loadedNodes;
  loadedNodes << QString(browserNode->GetID());
  for (int i = 0; i < loadedSequenceNodes->GetNumberOfItems(); i++)
  {
    vtkMRMLNode* loadedNode = vtkMRMLNode::SafeDownCast(loadedSequenceNodes->GetItemAsObject(i));
    if (loadedNode == NULL)
    {
      continue;
    }
    loadedNodes << QString(loadedNode->GetID());
  } 
  this->setLoadedNodes(loadedNodes);

  qSlicerMetafileImporterModule::showSequenceBrowser(browserNode);
  return true;
}
