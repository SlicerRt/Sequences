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

// #define ENABLE_PERFORMANCE_PROFILING

// MetafileImporter Logic includes
#include "vtkSlicerMetafileImporterLogic.h"
#include "vtkSlicerSequencesLogic.h"

// MRMLSequence includes
#include "vtkMRMLSequenceNode.h"
#include "vtkMRMLSequenceBrowserNode.h"
#include "vtkMRMLVolumeSequenceStorageNode.h"

// MRML includes
#include "vtkCacheManager.h"
#include "vtkMRMLLinearTransformNode.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLScalarVolumeDisplayNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLSelectionNode.h"
#include "vtkMRMLStorageNode.h"
#include "vtkMRMLVectorVolumeNode.h"

// VTK includes
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

#ifdef ENABLE_PERFORMANCE_PROFILING
#include "vtkTimerLog.h"
#endif

// STD includes
#include <sstream>
#include <algorithm>

static const char IMAGE_NODE_BASE_NAME[]="Image";
static const char NODE_BASE_NAME_SEPARATOR[]="-";

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerMetafileImporterLogic);

//----------------------------------------------------------------------------
vtkSlicerMetafileImporterLogic
::vtkSlicerMetafileImporterLogic() 
{
  this->SequencesLogic = NULL;
}

//----------------------------------------------------------------------------
vtkSlicerMetafileImporterLogic::~vtkSlicerMetafileImporterLogic()
{
}

//----------------------------------------------------------------------------
void vtkSlicerMetafileImporterLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//---------------------------------------------------------------------------
void vtkSlicerMetafileImporterLogic::SetSequencesLogic(vtkSlicerSequencesLogic* sequencesLogic)
{
  this->SequencesLogic=sequencesLogic;
}

//---------------------------------------------------------------------------
void vtkSlicerMetafileImporterLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerMetafileImporterLogic::RegisterNodes()
{
  if (this->GetMRMLScene()==NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
}

//---------------------------------------------------------------------------
void vtkSlicerMetafileImporterLogic::UpdateFromMRMLScene()
{
  if (this->GetMRMLScene()==NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
}

//---------------------------------------------------------------------------
void vtkSlicerMetafileImporterLogic
::OnMRMLSceneNodeAdded(vtkMRMLNode* vtkNotUsed(node))
{
}

//---------------------------------------------------------------------------
void vtkSlicerMetafileImporterLogic
::OnMRMLSceneNodeRemoved(vtkMRMLNode* vtkNotUsed(node))
{
}


// Add the helper functions

//-------------------------------------------------------
void Trim(std::string &str)
{
  str.erase(str.find_last_not_of(" \t\r\n")+1);
  str.erase(0,str.find_first_not_of(" \t\r\n"));
}

//----------------------------------------------------------------------------
/*! Quick and robust string to int conversion */
template<class T>
void StringToInt(const char* strPtr, T &result)
{
  if (strPtr==NULL || strlen(strPtr) == 0 )
  {
    return;
  }
  char * pEnd=NULL;
  result = static_cast<int>(strtol(strPtr, &pEnd, 10));
  if (pEnd != strPtr+strlen(strPtr) )
  {
    return;
  }
  return;
}


void UpdateTransformNodeFromString(vtkMRMLLinearTransformNode* transformNode, const std::string& str )
{
  std::stringstream ss( str );
  
  double e00; ss >> e00; double e01; ss >> e01; double e02; ss >> e02; double e03; ss >> e03;
  double e10; ss >> e10; double e11; ss >> e11; double e12; ss >> e12; double e13; ss >> e13;
  double e20; ss >> e20; double e21; ss >> e21; double e22; ss >> e22; double e23; ss >> e23;
  double e30; ss >> e30; double e31; ss >> e31; double e32; ss >> e32; double e33; ss >> e33;

  vtkSmartPointer<vtkMatrix4x4> matrix = vtkSmartPointer<vtkMatrix4x4>::New();

  matrix->SetElement( 0, 0, e00 );
  matrix->SetElement( 0, 1, e01 );
  matrix->SetElement( 0, 2, e02 );
  matrix->SetElement( 0, 3, e03 );

  matrix->SetElement( 1, 0, e10 );
  matrix->SetElement( 1, 1, e11 );
  matrix->SetElement( 1, 2, e12 );
  matrix->SetElement( 1, 3, e13 );

  matrix->SetElement( 2, 0, e20 );
  matrix->SetElement( 2, 1, e21 );
  matrix->SetElement( 2, 2, e22 );
  matrix->SetElement( 2, 3, e23 );

  matrix->SetElement( 3, 0, e30 );
  matrix->SetElement( 3, 1, e31 );
  matrix->SetElement( 3, 2, e32 );
  matrix->SetElement( 3, 3, e33 );

  transformNode->SetMatrixTransformToParent( matrix );
}


// Constants for reading transforms
static const int MAX_LINE_LENGTH = 1000;

static std::string SEQMETA_FIELD_FRAME_FIELD_PREFIX = "Seq_Frame";
static std::string SEQMETA_FIELD_IMG_STATUS = "ImageStatus";

//----------------------------------------------------------------------------
bool vtkSlicerMetafileImporterLogic::ReadSequenceMetafileTransforms(const std::string& fileName,
  const std::string &baseNodeName, std::deque< vtkMRMLNode* > &createdNodes,
  std::map< int, std::string >& frameNumberToIndexValueMap)
{
  // Open in binary mode because we determine the start of the image buffer also during this read
  const char* flags = "rb";
  FILE* stream = fopen( fileName.c_str(), flags ); // TODO: Removed error
  if (stream == NULL)
  {
    vtkErrorMacro("Failed to open file for transform reading: "<<fileName);
    return false;
  }

  char line[ MAX_LINE_LENGTH + 1 ] = { 0 };

  frameNumberToIndexValueMap.clear();

  // This structure contains all the transform nodes that are read from the file.
  // The nodes are not added immediately to the scene to allow them properly named, using the timestamp index value.
  // Maps the frame number to a vector of transform nodes that belong to that frame.
  std::map<int,std::vector<vtkMRMLLinearTransformNode*> > importedTransformNodes;

  // It contains the largest frame number. It will be used to iterate through all the frame numbers from 0 to lastFrameNumber
  int lastFrameNumber=-1;

  while ( fgets( line, MAX_LINE_LENGTH, stream ) )
  {
    std::string lineStr = line;

    // Split line into name and value
    size_t equalSignFound=0;
    equalSignFound = lineStr.find_first_of( "=" );
    if ( equalSignFound == std::string::npos )
    {
      vtkWarningMacro("Parsing line failed, equal sign is missing ("<<lineStr<<")");
      continue;
    }
    std::string name = lineStr.substr( 0, equalSignFound );
    std::string value = lineStr.substr( equalSignFound + 1 );

    // Trim spaces from the left and right
    Trim( name );
    Trim( value );

    if ( name.compare("ElementDataFile")==NULL )
    {
      // this is the last field of the header
      break;
    }
    

    // Only consider the Seq_Frame
    if ( name.compare( 0, SEQMETA_FIELD_FRAME_FIELD_PREFIX.size(), SEQMETA_FIELD_FRAME_FIELD_PREFIX ) != 0 )
    {
      // not a frame field, ignore it
      continue;
    }

    // frame field
    // name: Seq_Frame0000_CustomTransform
    name.erase( 0, SEQMETA_FIELD_FRAME_FIELD_PREFIX.size() ); // 0000_CustomTransform

    // Split line into name and value
    size_t underscoreFound;
    underscoreFound = name.find_first_of( "_" );
    if ( underscoreFound == std::string::npos )
    {
      vtkWarningMacro("Parsing line failed, underscore is missing from frame field name ("<<lineStr<<")");
      continue;
    }

    std::string frameNumberStr = name.substr( 0, underscoreFound ); // 0000
    std::string frameFieldName = name.substr( underscoreFound + 1 ); // CustomTransform

    int frameNumber = 0;
    StringToInt( frameNumberStr.c_str(), frameNumber ); // TODO: Removed warning
    if (frameNumber>lastFrameNumber)
    {
      lastFrameNumber=frameNumber;
    }
      
    // Convert the string to transform and add transform to hierarchy
    if ( frameFieldName.find( "Transform" ) != std::string::npos && frameFieldName.find( "Status" ) == std::string::npos )
    {
      vtkMRMLLinearTransformNode* currentTransform = vtkMRMLLinearTransformNode::New(); // will be deleted when added to the scene
      UpdateTransformNodeFromString(currentTransform, value);  
      // Generating a unique name is important because that will be used to generate the filename by default
      currentTransform->SetName( frameFieldName.c_str() );
      importedTransformNodes[frameNumber].push_back(currentTransform);
    }

    if ( frameFieldName.compare( "Timestamp" ) == 0 )
    {
      double timestampSec = atof(value.c_str());
      // round timestamp to 3 decimal digits, as timestamp is included in node names and having lots of decimal digits would
      // sometimes lead to extremely long node names
      std::ostringstream timestampSecStr;
      timestampSecStr << std::fixed << std::setprecision(3) << timestampSec << std::ends; 
      frameNumberToIndexValueMap[frameNumber] = timestampSecStr.str();
    }

    if ( ferror( stream ) )
    {
      vtkErrorMacro("Error reading the file "<<fileName.c_str());
      break;
    }
    if ( feof( stream ) )
    {
      break;
    }

  }
  fclose( stream );

  // Now add all the nodes to the scene

  std::map< std::string, vtkMRMLSequenceNode* > transformRootNodes;

  for (int currentFrameNumber=0; currentFrameNumber<=lastFrameNumber; currentFrameNumber++)
  {
    std::map<int,std::vector<vtkMRMLLinearTransformNode*> >::iterator transformsForCurrentFrame=importedTransformNodes.find(currentFrameNumber);
    if ( transformsForCurrentFrame == importedTransformNodes.end() )
    {
      // no transforms for this frame
      continue;
    }
    std::string paramValueString = frameNumberToIndexValueMap[currentFrameNumber];
    for (std::vector<vtkMRMLLinearTransformNode*>::iterator transformIt=transformsForCurrentFrame->second.begin(); transformIt!=transformsForCurrentFrame->second.end(); ++transformIt)
    {
      vtkMRMLLinearTransformNode* transform=(*transformIt);
      vtkMRMLSequenceNode* transformsRootNode = NULL;
      if (transformRootNodes.find(transform->GetName())==transformRootNodes.end())
      {
        // Setup hierarchy structure
        vtkSmartPointer<vtkMRMLSequenceNode> newTransformsRootNode = vtkSmartPointer<vtkMRMLSequenceNode>::New();
        transformsRootNode=newTransformsRootNode;
        this->GetMRMLScene()->AddNode(transformsRootNode);
        transformsRootNode->SetIndexName("time");
        transformsRootNode->SetIndexUnit("s");
        std::string transformsRootName=baseNodeName+NODE_BASE_NAME_SEPARATOR+transform->GetName();        
        transformsRootNode->SetName( transformsRootName.c_str() );

        // Create storage node
        vtkMRMLStorageNode *storageNode = transformsRootNode->CreateDefaultStorageNode();    
        if (storageNode)
        {
          this->GetMRMLScene()->AddNode(storageNode);
          storageNode->Delete(); // now the scene owns the storage node
          transformsRootNode->SetAndObserveStorageNodeID(storageNode->GetID());
          transformsRootNode->StorableModified(); // marks as modified, so the volume will be written to file on save
        }
        else
        {
          vtkErrorMacro("Failed to create storage node for the imported image sequence");
        }

        transformsRootNode->StartModify();
        transformRootNodes[transform->GetName()]=transformsRootNode;
      }
      else
      {
        transformsRootNode = transformRootNodes[transform->GetName()];
      }
      transform->SetHideFromEditors(false);
      // Generating a unique name is important because that will be used to generate the filename by default
      std::ostringstream nameStr;
      nameStr << transform->GetName() << std::setw(4) << std::setfill('0') << currentFrameNumber << std::ends; 
      transform->SetName( nameStr.str().c_str() );
      transformsRootNode->SetDataNodeAtValue(transform, paramValueString.c_str() );
      transform->Delete(); // ownership transferred to the root node
    }
  }

  for (std::map< std::string, vtkMRMLSequenceNode* > :: iterator it=transformRootNodes.begin(); it!=transformRootNodes.end(); ++it)
  {
    if (this->GetMRMLScene()->GetFirstNodeByName(it->second->GetName())!=it->second)
    {
      // node name is not unique, generate a unique name now
      it->second->SetName(this->GetMRMLScene()->GenerateUniqueName(it->second->GetName()).c_str());
    }
    it->second->EndModify(false);
    // Loading is completed indicate to modules that the hierarchy is changed
    it->second->Modified();
    createdNodes.push_back(it->second);
  }
  transformRootNodes.clear();
 
  return true;
}

//----------------------------------------------------------------------------
// Read the spacing and dimentions of the image.
vtkMRMLNode* vtkSlicerMetafileImporterLogic::ReadSequenceMetafileImages( const std::string& fileName,
  const std::string &baseNodeName, std::map< int, std::string >& frameNumberToIndexValueMap)
{
#ifdef ENABLE_PERFORMANCE_PROFILING
  vtkSmartPointer<vtkTimerLog> timer=vtkSmartPointer<vtkTimerLog>::New();
  timer->StartTimer();  
#endif
  vtkSmartPointer< vtkMetaImageReader > imageReader = vtkSmartPointer< vtkMetaImageReader >::New();
  imageReader->SetFileName( fileName.c_str() );
  imageReader->Update();
#ifdef ENABLE_PERFORMANCE_PROFILING
  timer->StopTimer();
  vtkWarningMacro("Image reading: " << timer->GetElapsedTime() << "sec\n");
#endif  

  // check for loading error
  // if there is a loading error then all the extents are set to 0
  // (although it corresponds to an 1x1x1 image size)
  if (imageReader->GetDataExtent()[0]==0 && imageReader->GetDataExtent()[1]==0
    && imageReader->GetDataExtent()[2]==0 && imageReader->GetDataExtent()[3]==0
    && imageReader->GetDataExtent()[4]==0 && imageReader->GetDataExtent()[5]==0)
  {       
    return NULL;
  }

  // Create sequence node
  vtkSmartPointer<vtkMRMLSequenceNode> imagesRootNode = vtkSmartPointer<vtkMRMLSequenceNode>::New();
  this->GetMRMLScene()->AddNode(imagesRootNode);
  imagesRootNode->SetIndexName("time");
  imagesRootNode->SetIndexUnit("s");
  std::string imagesRootName=baseNodeName+NODE_BASE_NAME_SEPARATOR+"Image";
  imagesRootNode->SetName( this->GetMRMLScene()->GenerateUniqueName(imagesRootName).c_str() );

  // Create storage node
  vtkMRMLStorageNode *storageNode = imagesRootNode->CreateDefaultStorageNode();    
  if (storageNode)
  {
    this->GetMRMLScene()->AddNode(storageNode);
    storageNode->Delete(); // now the scene owns the storage node
    imagesRootNode->SetAndObserveStorageNodeID(storageNode->GetID());
    imagesRootNode->StorableModified(); // marks as modified, so the volume will be written to file on save
  }
  else
  {
    vtkErrorMacro("Failed to create storage node for the imported image sequence");
  }

  int imagesRootNodeDisableModify = imagesRootNode->StartModify();

  // Grab the image data from the mha file  
  vtkImageData* imageData = imageReader->GetOutput();
  vtkSmartPointer<vtkImageData> emptySliceImageData=vtkSmartPointer<vtkImageData>::New();
  int* dimensions = imageData->GetDimensions();
  emptySliceImageData->SetDimensions(dimensions[0],dimensions[1],1);
  double* spacing = imageData->GetSpacing();
  emptySliceImageData->SetSpacing(spacing[0],spacing[1],1);
  emptySliceImageData->SetOrigin(0,0,0);  
  
  int sliceSize=imageData->GetIncrements()[2];
  for ( int frameNumber = 0; frameNumber < dimensions[2]; frameNumber++ )
  {     
    // Add the image slice to scene as a volume    

    vtkSmartPointer< vtkMRMLScalarVolumeNode > slice;
    if (imageData->GetNumberOfScalarComponents() > 1)
    {
      slice = vtkSmartPointer< vtkMRMLVectorVolumeNode >::New();
    }
    else
    {
      slice = vtkSmartPointer< vtkMRMLScalarVolumeNode >::New();
    }
    
    vtkSmartPointer<vtkImageData> sliceImageData=vtkSmartPointer<vtkImageData>::New();
    sliceImageData->DeepCopy(emptySliceImageData);

#if (VTK_MAJOR_VERSION <= 5)
    sliceImageData->SetScalarType(imageData->GetScalarType());
    sliceImageData->SetNumberOfScalarComponents(imageData->GetNumberOfScalarComponents());
    sliceImageData->AllocateScalars();
#else
    sliceImageData->AllocateScalars(imageData->GetScalarType(), imageData->GetNumberOfScalarComponents());
#endif

    unsigned char* startPtr=(unsigned char*)imageData->GetScalarPointer(0, 0, frameNumber);
    memcpy(sliceImageData->GetScalarPointer(), startPtr, sliceSize);

    // Generating a unique name is important because that will be used to generate the filename by default
    std::ostringstream nameStr;
    nameStr << IMAGE_NODE_BASE_NAME << std::setw(4) << std::setfill('0') << frameNumber << std::ends;
    slice->SetName(nameStr.str().c_str());
    slice->SetAndObserveImageData(sliceImageData);

    std::string paramValueString = frameNumberToIndexValueMap[frameNumber];
    slice->SetHideFromEditors(false);
    imagesRootNode->SetDataNodeAtValue(slice, paramValueString.c_str() );
  }

  imagesRootNode->EndModify(imagesRootNodeDisableModify);
  imagesRootNode->Modified();
  return imagesRootNode;
}

//----------------------------------------------------------------------------
bool vtkSlicerMetafileImporterLogic::ReadSequenceMetafile(const std::string& fileName)
{
  // Map the frame numbers to timestamps
  std::map< int, std::string > frameNumberToIndexValueMap;

  int dotFound = fileName.find_last_of( "." );
  int slashFound = fileName.find_last_of( "/" );
  std::string baseNodeName=fileName.substr( slashFound + 1, dotFound - slashFound - 1 );

#ifdef ENABLE_PERFORMANCE_PROFILING
  vtkSmartPointer<vtkTimerLog> timer=vtkSmartPointer<vtkTimerLog>::New();      
  timer->StartTimer();
#endif
  std::deque< vtkMRMLNode* > createdTransformNodes;
  if (!this->ReadSequenceMetafileTransforms(fileName, baseNodeName, createdTransformNodes, frameNumberToIndexValueMap))
  {
    // error is logged in ReadTransforms
    return false;
  }
#ifdef ENABLE_PERFORMANCE_PROFILING
  timer->StopTimer();
  vtkWarningMacro("ReadTransforms time: " << timer->GetElapsedTime() << "sec\n");
  timer->StartTimer();
#endif
  vtkMRMLNode* createdImageNode=this->ReadSequenceMetafileImages(fileName, baseNodeName, frameNumberToIndexValueMap);
#ifdef ENABLE_PERFORMANCE_PROFILING
  timer->StopTimer();
  vtkWarningMacro("ReadImages time: " << timer->GetElapsedTime() << "sec\n");
#endif

  // For the user's convenience, create a browser node that contains the image as master node
  // (the first transform node, if no image in the file) and the transforms as synchronized nodes
  vtkMRMLNode* masterNode=createdImageNode;
  if (masterNode==NULL)
  {
    if (!createdTransformNodes.empty())
    {
      masterNode=createdTransformNodes.front();
      createdTransformNodes.pop_front();
    }
  }

  // Add a browser node and show the volume in the slice viewer for user convenience
  if (masterNode!=NULL)
  { 
    vtkSmartPointer<vtkMRMLSequenceBrowserNode> sequenceBrowserNode=vtkSmartPointer<vtkMRMLSequenceBrowserNode>::New();
    sequenceBrowserNode->SetName(this->GetMRMLScene()->GenerateUniqueName(baseNodeName).c_str());
    this->GetMRMLScene()->AddNode(sequenceBrowserNode);
    sequenceBrowserNode->SetAndObserveRootNodeID(masterNode->GetID());
    for (std::deque< vtkMRMLNode* > :: iterator synchronizedNodesIt = createdTransformNodes.begin();
      synchronizedNodesIt != createdTransformNodes.end(); ++synchronizedNodesIt)
    {
      sequenceBrowserNode->AddSynchronizedRootNode((*synchronizedNodesIt)->GetID());
    }
    // Show output volume in the slice viewer
    vtkMRMLNode* masterOutputNode=sequenceBrowserNode->GetVirtualOutputDataNode(vtkMRMLSequenceNode::SafeDownCast(masterNode));
    if (masterOutputNode->IsA("vtkMRMLVolumeNode"))
    {
      vtkSlicerApplicationLogic* appLogic = this->GetApplicationLogic();
      vtkMRMLSelectionNode* selectionNode = appLogic ? appLogic->GetSelectionNode() : 0;
      if (selectionNode)
      {
        selectionNode->SetReferenceActiveVolumeID(masterOutputNode->GetID());
        if (appLogic)
        {
          appLogic->PropagateVolumeSelection();
          appLogic->FitSliceToAll();
        }
        sequenceBrowserNode->ScalarVolumeAutoWindowLevelOff(); // for performance optimization
      }
    }
  }

  return true;
}

//----------------------------------------------------------------------------
bool vtkSlicerMetafileImporterLogic::ReadVolumeSequence(const std::string& fileName)
{
  int dotFound = fileName.find_last_of( "." );
  int slashFound = fileName.find_last_of( "/" );
  std::string volumeName=fileName.substr( slashFound + 1, dotFound - slashFound - 1 );

  vtkMRMLScene* scene = this->GetMRMLScene();
  if (scene == NULL)
  {
    vtkErrorMacro("vtkSlicerMetafileImporterLogic::ReadVolumeSequence failed: scene is invalid");
    return false;
  }

  vtkNew<vtkMRMLSequenceNode> volumeSequenceNode;
  volumeSequenceNode->SetName(scene->GenerateUniqueName(volumeName).c_str());
  scene->AddNode(volumeSequenceNode.GetPointer());

  vtkNew<vtkMRMLVolumeSequenceStorageNode> storageNode;
  //storageNode->SetCenterImage(options & vtkSlicerVolumesLogic::CenterImage);
  scene->AddNode(storageNode.GetPointer());
  volumeSequenceNode->SetAndObserveStorageNodeID(storageNode->GetID());

  if (scene->GetCacheManager() && this->GetMRMLScene()->GetCacheManager()->IsRemoteReference(fileName.c_str()))
    {
    vtkDebugMacro("AddArchetypeVolume: input filename '" << fileName << "' is a URI");
    // need to set the scene on the storage node so that it can look for file handlers
    storageNode->SetURI(fileName.c_str());
    //storageNode->SetScene(this->GetMRMLScene());
    }
  else
    {
      storageNode->SetFileName(fileName.c_str());
    }

  bool success = storageNode->ReadData(volumeSequenceNode.GetPointer());

  if (!success)
  {
    scene->RemoveNode(storageNode.GetPointer());
    scene->RemoveNode(volumeSequenceNode.GetPointer());
    return false;
  }

  std::string browserNodeName = volumeName+" browser";
  vtkNew<vtkMRMLSequenceBrowserNode> sequenceBrowserNode;
  sequenceBrowserNode->SetName(scene->GenerateUniqueName(browserNodeName).c_str());
  scene->AddNode(sequenceBrowserNode.GetPointer());
  sequenceBrowserNode->SetAndObserveRootNodeID(volumeSequenceNode->GetID());

  // Show output volume in the slice viewer
  vtkMRMLNode* masterOutputNode=sequenceBrowserNode->GetVirtualOutputDataNode(volumeSequenceNode.GetPointer());
  if (masterOutputNode->IsA("vtkMRMLVolumeNode"))
  {
    vtkSlicerApplicationLogic* appLogic = this->GetApplicationLogic();
    vtkMRMLSelectionNode* selectionNode = appLogic ? appLogic->GetSelectionNode() : 0;
    if (selectionNode)
    {
      selectionNode->SetReferenceActiveVolumeID(masterOutputNode->GetID());
      if (appLogic)
      {
        appLogic->PropagateVolumeSelection();
        appLogic->FitSliceToAll();
      }
      sequenceBrowserNode->ScalarVolumeAutoWindowLevelOff(); // for performance optimization
    } 
  }

  return true;
}
