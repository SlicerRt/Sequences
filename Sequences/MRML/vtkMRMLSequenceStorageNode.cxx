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

#include "vtkMRMLSequenceNode.h"
#include "vtkMRMLSequenceStorageNode.h"
#include "vtkMRMLScene.h"

#include "vtkSlicerApplicationLogic.h"

// VTK includes
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkStringArray.h>
#include <vtksys/SystemTools.hxx>

// QT includes
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QPixmap> 

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLSequenceStorageNode);

//----------------------------------------------------------------------------
vtkMRMLSequenceStorageNode::vtkMRMLSequenceStorageNode()
{
}

//----------------------------------------------------------------------------
vtkMRMLSequenceStorageNode::~vtkMRMLSequenceStorageNode()
{
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceStorageNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLStorageNode::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
bool vtkMRMLSequenceStorageNode::CanReadInReferenceNode(vtkMRMLNode *refNode)
{
  return refNode->IsA("vtkMRMLSequenceNode");
}

//----------------------------------------------------------------------------
int vtkMRMLSequenceStorageNode::ReadDataInternal(vtkMRMLNode *refNode)
{
  vtkMRMLSequenceNode *sequenceNode = dynamic_cast <vtkMRMLSequenceNode *> (refNode);

  std::string fullName = this->GetFullNameFromFileName();
  if (fullName == std::string(""))
  {
    vtkErrorMacro("ReadDataInternal: File name not specified");
    return 0;
  }

  // check that the file exists
  if (vtksys::SystemTools::FileExists(fullName.c_str()) == false)
  {
    vtkErrorMacro("ReadDataInternal: model file '" << fullName.c_str() << "' not found.");
    return 0;
  }

  // compute file prefix
  std::string extension = vtkMRMLStorageNode::GetLowercaseExtensionFromFileName(fullName);
  if( extension.empty() )
  {
    vtkErrorMacro("ReadData: no file extension specified: " << fullName.c_str());
    return 0;
  }

  vtkDebugMacro("ReadDataInternal: extension = " << extension.c_str());

  // Custom nodes (such as vtkMRMLSceneView node) must be registered in the sequence scene,
  // otherwise the XML parser cannot instantiate them
  if (this->GetScene() && sequenceNode->GetSequenceScene())
  {
    this->GetScene()->CopyRegisteredNodesToScene(sequenceNode->GetSequenceScene());
  }
  else
  {
    vtkErrorMacro("Cannot register nodes in the sequence node");
  }

  int success = false;
  if (extension == std::string(".mrb"))
  {    
    success = vtkMRMLSequenceStorageNode::ReadFromMRB(fullName.c_str(), sequenceNode->GetSequenceScene());
  }
  else
  {
    vtkErrorMacro("Cannot read sequence file '" << fullName.c_str() << "' (extension = " << extension.c_str() << ")");
  }

  return success ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkMRMLSequenceStorageNode::WriteDataInternal(vtkMRMLNode *refNode)
{
  vtkMRMLSequenceNode *sequenceNode = vtkMRMLSequenceNode::SafeDownCast(refNode);

  std::string fullName = this->GetFullNameFromFileName();
  if (fullName == std::string(""))
  {
    vtkErrorMacro("vtkMRMLSequenceNode: File name not specified");
    return 0;
  }

  std::string extension = vtkMRMLStorageNode::GetLowercaseExtensionFromFileName(fullName);

  bool success = false;
  if (extension == ".mrb")
  {
    vtkMRMLScene *sequenceScene=sequenceNode->GetSequenceScene();
    success = WriteToMRB(fullName.c_str(), sequenceScene);
  }
  else
  {
    vtkErrorMacro( << "No file extension recognized: " << fullName.c_str() );
  }

  return success ? 1 : 0;
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceStorageNode::InitializeSupportedReadFileTypes()
{
  this->SupportedReadFileTypes->InsertNextValue("Medical Reality Bundle (.mrb)");
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceStorageNode::InitializeSupportedWriteFileTypes()
{
  this->SupportedWriteFileTypes->InsertNextValue("Medical Reality Bundle (.mrb)");
}

//----------------------------------------------------------------------------
const char* vtkMRMLSequenceStorageNode::GetDefaultWriteFileExtension()
{
  return "mrb";
}

//-----------------------------------------------------------------------------
bool RemoveDirRecursively(const QString & dirName)
{
  bool result = false;
  QDir dir(dirName);

  if (dir.exists(dirName))
  {
    foreach (QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst))
    {
      if (info.isDir())
      {
        result = RemoveDirRecursively(info.absoluteFilePath());
      }
      else
      {
        result = QFile::remove(info.absoluteFilePath());
      }

      if (!result)
      {
        return result;
      }
    }
    result = dir.rmdir(dirName);
  }

  return result;
} 

// Adopted from Modules/Loadable/Data/qSlicerSceneWriter.cxx
//----------------------------------------------------------------------------
bool vtkMRMLSequenceStorageNode::WriteToMRB(const char* fullName, vtkMRMLScene *scene)
{
  //
  // make a temp directory to save the scene into - this will
  // be a uniquely named directory that contains a directory
  // named based on the user's selection.
  //

  QFileInfo fileInfo(fullName);
  QString basePath = fileInfo.absolutePath();
  if (!QFileInfo(basePath).isWritable())
  {
    qWarning() << "Failed to save" << fileInfo.absoluteFilePath() << ":"
      << "Path" << basePath << "is not writable";
    return false;
  }

  // TODO: switch to QTemporaryDir in Qt5.
  // For now, create a named directory and use Qt calls to remove it
  // use the output directory as temporary directory. This may not be ideal if the output directory has limited storage space (e.g., USB stick).
  QString tempDir = basePath;
  QFileInfo pack(QDir(tempDir), //QDir::tempPath(),
    QString("__BundleSaveTemp-") + 
    QDateTime::currentDateTime().toString("yyyy-MM-dd_hh+mm+ss.zzz"));
  qDebug() << "packing to " << pack.absoluteFilePath();

  // make a subdirectory with the name the user has chosen
  QFileInfo bundle = QFileInfo(QDir(pack.absoluteFilePath()),
    fileInfo.baseName());
  QString bundlePath = bundle.absoluteFilePath();
  if ( bundle.exists() )
  {
    if ( !RemoveDirRecursively(bundlePath) )
    {
      QMessageBox::critical(0, QObject::tr("Save Scene as MRB"), QObject::tr("Could not remove temp directory"));
      return false;
    }
  }

  if ( !QDir().mkpath(bundlePath) )
  {
    QMessageBox::critical(0, QObject::tr("Save scene as MRB"), QObject::tr("Could not make temp directory"));
    return false;
  }

  vtkSmartPointer<vtkImageData> imageData;
  /* TODO: might be useful to generate a thumbnail and save it with the scene
  if (properties.contains("screenShot"))
  {
  QPixmap screenShot = properties["screenShot"].value<QPixmap>();
  // convert to vtkImageData
  imageData = vtkSmartPointer<vtkImageData>::New();
  qMRMLUtils::qImageToVtkImageData(screenShot.toImage(), imageData);
  }
  */

  //
  // Now save the scene into the bundle directory and then make a zip (mrb) file
  // in the user's selected file location
  //
  vtkSmartPointer<vtkSlicerApplicationLogic> applicationLogic =  vtkSmartPointer<vtkSlicerApplicationLogic>::New();
  applicationLogic->SetAndObserveMRMLScene(scene);
  std::string bundlePathStd = bundlePath.toLatin1().constData();
  bool retval = applicationLogic->SaveSceneToSlicerDataBundleDirectory(bundlePathStd.c_str(), imageData);
  if (!retval)
  {
    QMessageBox::critical(0, QObject::tr("Save scene as MRB"), QObject::tr("Failed to create bundle"));
    return false;
  }

  qDebug() << "zipping to " << fileInfo.absoluteFilePath();
  if ( !applicationLogic->Zip(fileInfo.absoluteFilePath().toLatin1().constData(), bundlePathStd.c_str()) )
  {
    QMessageBox::critical(0, QObject::tr("Save scene as MRB"), QObject::tr("Could not compress bundle"));
    return false;
  }

  //
  // Now clean up the temp directory
  //
  QString tmpPath = pack.absoluteFilePath();
  if ( !RemoveDirRecursively(tmpPath) )
  {
    QMessageBox::critical(0, QObject::tr("Save scene as MRB"), QObject::tr("Could not remove temp directory"));
    return false;
  }

  qDebug() << "saved " << fileInfo.absoluteFilePath();
  return true;
}

// Adopted from qSlicerSceneBundleReader::load in qSlicerSceneBundleReader.cxx 
//-----------------------------------------------------------------------------
bool vtkMRMLSequenceStorageNode::ReadFromMRB(const char* fullName, vtkMRMLScene *scene)
{
  // TODO: switch to QTemporaryDir in Qt5.
  // For now, create a named directory and use
  // kwsys calls to remove it
  QString unpackPath( QDir::tempPath() + 
    QString("/__BundleLoadTemp") + 
    QDateTime::currentDateTime().toString("yyyy-MM-dd_hh+mm+ss.zzz") );

  qDebug() << "Unpacking bundle to " << unpackPath;
  std::string unpackPathStd = unpackPath.toLatin1().constData();
  if (vtksys::SystemTools::FileIsDirectory(unpackPathStd.c_str()))
  {
    if ( !vtksys::SystemTools::RemoveADirectory(unpackPathStd.c_str()) )
    {
      return false;
    }
  }

  if ( !vtksys::SystemTools::MakeDirectory(unpackPathStd.c_str()) )
  {
    return false;
  }

  vtkNew<vtkMRMLApplicationLogic> appLogic;
  appLogic->SetMRMLScene( scene );
  std::string mrmlFile = appLogic->UnpackSlicerDataBundle( fullName, unpackPathStd.c_str() );

  scene->SetURL(mrmlFile.c_str());

  int res = scene->Connect();

  if ( !vtksys::SystemTools::RemoveADirectory(unpackPathStd.c_str()) )
  {
    return false;
  }

  qDebug() << "Loaded bundle from " << unpackPath;
  return res;
}
