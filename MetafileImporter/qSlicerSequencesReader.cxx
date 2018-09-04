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
#include <QFileInfo>

// SlicerQt includes
#include "qSlicerSequencesReader.h"
#include "qSlicerSequenceBrowserModule.h"

// Logic includes
#include "vtkSlicerSequencesLogic.h"

// MRML includes
#include "vtkMRMLSequenceNode.h"
#include "vtkMRMLSequenceBrowserNode.h"
#include "vtkMRMLSequenceStorageNode.h"
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkSmartPointer.h>

//-----------------------------------------------------------------------------
class qSlicerSequencesReaderPrivate
{
public:
  vtkSmartPointer<vtkSlicerSequencesLogic> SequencesLogic;
};

//-----------------------------------------------------------------------------
/// \ingroup Slicer_QtModules_Sequences
qSlicerSequencesReader::qSlicerSequencesReader(vtkSlicerSequencesLogic* sequencesLogic, QObject* _parent)
  : Superclass(_parent)
  , d_ptr(new qSlicerSequencesReaderPrivate)
{
  this->setSequencesLogic(sequencesLogic);
}

//-----------------------------------------------------------------------------
qSlicerSequencesReader::~qSlicerSequencesReader()
{
}

//-----------------------------------------------------------------------------
void qSlicerSequencesReader::setSequencesLogic(vtkSlicerSequencesLogic* newSequencesLogic)
{
  Q_D(qSlicerSequencesReader);
  d->SequencesLogic = newSequencesLogic;
}

//-----------------------------------------------------------------------------
vtkSlicerSequencesLogic* qSlicerSequencesReader::sequencesLogic()const
{
  Q_D(const qSlicerSequencesReader);
  return d->SequencesLogic;
}

//-----------------------------------------------------------------------------
QString qSlicerSequencesReader::description()const
{
  return "Sequence";
}

//-----------------------------------------------------------------------------
qSlicerIO::IOFileType qSlicerSequencesReader::fileType()const
{
  return QString("SequenceFile");
}

//-----------------------------------------------------------------------------
QStringList qSlicerSequencesReader::extensions()const
{
  return QStringList()
    << "Sequence (*.seq.mrb *.mrb)";
}

//-----------------------------------------------------------------------------
bool qSlicerSequencesReader::load(const IOProperties& properties)
{
  Q_D(qSlicerSequencesReader);
  Q_ASSERT(properties.contains("fileName"));
  QString fileName = properties["fileName"].toString();

  this->setLoadedNodes(QStringList());
  if (d->SequencesLogic.GetPointer() == 0)
    {
    return false;
    }
  vtkMRMLSequenceNode* node = d->SequencesLogic->AddSequence(fileName.toLatin1());
  if (!node)
    {
    return false;
    }

  if (properties.contains("name"))
  {
    std::string customName = this->mrmlScene()->GetUniqueNameByString(
      properties["name"].toString().toLatin1());
    node->SetName(customName.c_str());
  }

  QStringList loadedNodeIDs;
  loadedNodeIDs << QString(node->GetID());

  const bool createDefaultBrowserNode = true;
  
  vtkMRMLSequenceBrowserNode* browserNode = NULL;
  if (createDefaultBrowserNode)
  {
    std::string browserCustomName = std::string(node->GetName()) + " browser";
    browserNode = vtkMRMLSequenceBrowserNode::SafeDownCast(
      this->mrmlScene()->AddNewNodeByClass("vtkMRMLSequenceBrowserNode", browserCustomName));
  }

  if (browserNode)
  {
    loadedNodeIDs << QString(browserNode->GetID());
    browserNode->SetAndObserveMasterSequenceNodeID(node->GetID());
    qSlicerSequenceBrowserModule::showSequenceBrowser(browserNode);
  }

  this->setLoadedNodes(loadedNodeIDs);
  return true;
}
