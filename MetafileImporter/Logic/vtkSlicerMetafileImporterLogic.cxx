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
#include <vtkAddonMathUtilities.h>
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


// Constants for reading transforms
static const int MAX_LINE_LENGTH = 1000;

static std::string SEQMETA_FIELD_FRAME_FIELD_PREFIX = "Seq_Frame";
static std::string SEQMETA_FIELD_IMG_STATUS = "ImageStatus";

//----------------------------------------------------------------------------
bool vtkSlicerMetafileImporterLogic::ReadSequenceMetafileTransforms(const std::string& fileName,
  const std::string &baseNodeName, std::deque< vtkMRMLSequenceNode* > &createdNodes,
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
      vtkNew<vtkMatrix4x4> matrix;
      bool success = vtkAddonMathUtilities::FromString(matrix.GetPointer(), value);
      if (!success)
      {
        continue;
      }
      vtkMRMLLinearTransformNode* currentTransform = vtkMRMLLinearTransformNode::New(); // will be deleted when added to the scene
      currentTransform->SetMatrixTransformToParent(matrix.GetPointer());  
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

  std::map< std::string, vtkMRMLSequenceNode* > transformSequenceNodes;

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
      vtkMRMLSequenceNode* transformsSequenceNode = NULL;
      if (transformSequenceNodes.find(transform->GetName())==transformSequenceNodes.end())
      {
        // Setup hierarchy structure
        vtkSmartPointer<vtkMRMLSequenceNode> newTransformsSequenceNode = vtkSmartPointer<vtkMRMLSequenceNode>::New();
        transformsSequenceNode=newTransformsSequenceNode;
        this->GetMRMLScene()->AddNode(transformsSequenceNode);
        transformsSequenceNode->SetIndexName("time");
        transformsSequenceNode->SetIndexUnit("s");
        std::string transformsSequenceName=baseNodeName+NODE_BASE_NAME_SEPARATOR+transform->GetName();        
        transformsSequenceNode->SetName( transformsSequenceName.c_str() );

        // Create storage node
        vtkMRMLStorageNode *storageNode = transformsSequenceNode->CreateDefaultStorageNode();    
        if (storageNode)
        {
          this->GetMRMLScene()->AddNode(storageNode);
          storageNode->Delete(); // now the scene owns the storage node
          transformsSequenceNode->SetAndObserveStorageNodeID(storageNode->GetID());
          transformsSequenceNode->StorableModified(); // marks as modified, so the volume will be written to file on save
        }
        else
        {
          vtkErrorMacro("Failed to create storage node for the imported image sequence");
        }

        transformsSequenceNode->StartModify();
        transformSequenceNodes[transform->GetName()]=transformsSequenceNode;
      }
      else
      {
        transformsSequenceNode = transformSequenceNodes[transform->GetName()];
      }
      transform->SetHideFromEditors(false);
      // Generating a unique name is important because that will be used to generate the filename by default
      std::ostringstream nameStr;
      nameStr << transform->GetName() << "_" << std::setw(4) << std::setfill('0') << currentFrameNumber << std::ends; 
      transform->SetName( nameStr.str().c_str() );
      transformsSequenceNode->SetDataNodeAtValue(transform, paramValueString.c_str() );
      transform->Delete(); // ownership transferred to the sequence node
    }
  }

  for (std::map< std::string, vtkMRMLSequenceNode* > :: iterator it=transformSequenceNodes.begin(); it!=transformSequenceNodes.end(); ++it)
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
  transformSequenceNodes.clear();
 
  return true;
}

//----------------------------------------------------------------------------
// Read the spacing and dimentions of the image.
vtkMRMLSequenceNode* vtkSlicerMetafileImporterLogic::ReadSequenceMetafileImages(const std::string& fileName,
  const std::string &baseNodeName, std::map< int, std::string >& frameNumberToIndexValueMap)
{
#ifdef ENABLE_PERFORMANCE_PROFILING
  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();  
#endif
  vtkNew< vtkMetaImageReader > imageReader;
  imageReader->SetFileName( fileName.c_str() );
  imageReader->Update();
#ifdef ENABLE_PERFORMANCE_PROFILING
  timer->StopTimer();
  vtkInfoMacro("Image reading: " << timer->GetElapsedTime() << "sec\n");
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
  vtkSmartPointer<vtkMRMLSequenceNode> imagesSequenceNode = vtkSmartPointer<vtkMRMLSequenceNode>::New();
  this->GetMRMLScene()->AddNode(imagesSequenceNode);
  imagesSequenceNode->SetIndexName("time");
  imagesSequenceNode->SetIndexUnit("s");
  std::string imagesSequenceName=baseNodeName+NODE_BASE_NAME_SEPARATOR+"Image";
  imagesSequenceNode->SetName( this->GetMRMLScene()->GenerateUniqueName(imagesSequenceName).c_str() );

  // Create storage node
  vtkMRMLStorageNode *storageNode = imagesSequenceNode->CreateDefaultStorageNode();    
  if (storageNode)
  {
    this->GetMRMLScene()->AddNode(storageNode);
    storageNode->Delete(); // now the scene owns the storage node
    imagesSequenceNode->SetAndObserveStorageNodeID(storageNode->GetID());
    imagesSequenceNode->StorableModified(); // marks as modified, so the volume will be written to file on save
  }
  else
  {
    vtkErrorMacro("Failed to create storage node for the imported image sequence");
  }

  int imagesSequenceNodeDisableModify = imagesSequenceNode->StartModify();

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

    sliceImageData->AllocateScalars(imageData->GetScalarType(), imageData->GetNumberOfScalarComponents());

    unsigned char* startPtr=(unsigned char*)imageData->GetScalarPointer(0, 0, frameNumber);
    memcpy(sliceImageData->GetScalarPointer(), startPtr, sliceSize);

    // Generating a unique name is important because that will be used to generate the filename by default
    std::ostringstream nameStr;
    nameStr << IMAGE_NODE_BASE_NAME << std::setw(4) << std::setfill('0') << frameNumber << std::ends;
    slice->SetName(nameStr.str().c_str());
    slice->SetAndObserveImageData(sliceImageData);

    std::string paramValueString = frameNumberToIndexValueMap[frameNumber];
    slice->SetHideFromEditors(false);
    imagesSequenceNode->SetDataNodeAtValue(slice, paramValueString.c_str() );
  }

  imagesSequenceNode->EndModify(imagesSequenceNodeDisableModify);
  imagesSequenceNode->Modified();
  return imagesSequenceNode;
}


//----------------------------------------------------------------------------
void vtkSlicerMetafileImporterLogic::WriteSequenceMetafileTransforms(const std::string& fileName, std::deque< vtkMRMLSequenceNode* > &transformSequenceNodes, vtkMRMLSequenceNode* masterNode, vtkMRMLSequenceNode* imageNode)
{
  vtkMRMLSequenceNode* masterSequenceNode = vtkMRMLSequenceNode::SafeDownCast(masterNode);
  if ( masterSequenceNode == NULL )
  {
    return;
  }
  vtkMRMLSequenceNode* imageSequenceNode = vtkMRMLSequenceNode::SafeDownCast(imageNode);

  // Read the file back in
  std::ifstream headerInStream( fileName.c_str(), std::ios_base::binary );

  std::string line;
  std::string elementDataFileLine;
  std::stringstream defaultHeaderOutStream;
  while ( std::getline( headerInStream, line ) )
  {
    if ( line.find( "ElementDataFile" ) == std::string::npos ) // Ignore this line (since this must be last)
    {
      defaultHeaderOutStream << line << std::endl;
    }
    else
    {
      elementDataFileLine = line;
    }
  }
  headerInStream.close();

  // Append the transform information to the end of the file
  std::ofstream headerOutStream( fileName.c_str(), std::ios_base::binary );
  headerOutStream << defaultHeaderOutStream.str();

  // Add the necessary header header stuff
  headerOutStream << "UltrasoundImageOrientation = ??" << std::endl;
  headerOutStream << "UltrasoundImageType = ??" << std::endl;
  // Other stuff is already taken care of by the vtkMetaImageWriter

  headerOutStream << std::setfill( '0' );
  // Iterate over everything in the master sequence node
  for ( int frameNumber = 0; frameNumber < masterSequenceNode->GetNumberOfDataNodes(); frameNumber++ )
  {
    std::string indexValue = masterSequenceNode->GetNthIndexValue( frameNumber );
    // Put all the transforms in the header
    for (std::deque< vtkMRMLSequenceNode* >::iterator itr = transformSequenceNodes.begin(); itr != transformSequenceNodes.end(); itr++)
    {
      vtkMRMLSequenceNode* currSequenceNode = vtkMRMLSequenceNode::SafeDownCast( *itr );

      std::string transformValue = "1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1"; // Identity
      std::string transformStatus = "INVALID";
      vtkMRMLLinearTransformNode* transformNode = vtkMRMLLinearTransformNode::SafeDownCast( currSequenceNode->GetDataNodeAtValue( indexValue.c_str() ) );
      if (transformNode!=NULL)
      {
        vtkNew<vtkMatrix4x4> matrix;
        transformNode->GetMatrixTransformToParent(matrix.GetPointer());
        transformValue = vtkAddonMathUtilities::ToString(matrix.GetPointer());
        transformStatus = "OK";
      }

      headerOutStream << SEQMETA_FIELD_FRAME_FIELD_PREFIX << std::setw( 4 ) << frameNumber << std::setw( 0 );
      headerOutStream << "_" << currSequenceNode->GetName() << "Transform =" << transformValue << std::endl; // TODO: Find a better way to grab the transform name
      headerOutStream << SEQMETA_FIELD_FRAME_FIELD_PREFIX << std::setw( 4 ) << frameNumber << std::setw( 0 );
      headerOutStream << "_" << currSequenceNode->GetName() << "TransformStatus = " << transformStatus << std::endl;
    }

    // The timestamp information
    headerOutStream << SEQMETA_FIELD_FRAME_FIELD_PREFIX << std::setw( 4 ) << frameNumber << std::setw( 0 );
    headerOutStream << "_Timestamp = " << masterSequenceNode->GetNthIndexValue( frameNumber ) << std::endl;
    
    // Put the image information
    std::string imageStatus = "INVALID";
    if (imageSequenceNode!=NULL && imageSequenceNode->GetDataNodeAtValue( indexValue.c_str() )!=NULL)
    {
      imageStatus = "OK";
    }
    headerOutStream << SEQMETA_FIELD_FRAME_FIELD_PREFIX << std::setw( 4 ) << frameNumber << std::setw( 0 );
    headerOutStream << "_ImageStatus = " << imageStatus << std::endl; // TODO: Find the image status in a better way
  }

  // Finally, append the element data file line at the end
  headerOutStream << elementDataFileLine;

  headerOutStream.close();
}



//----------------------------------------------------------------------------
// Write the spacing and dimentions of the image.
void vtkSlicerMetafileImporterLogic::WriteSequenceMetafileImages(const std::string& fileName, vtkMRMLSequenceNode* imageSequenceNode, vtkMRMLSequenceNode* masterSequenceNode)
{
  if ( masterSequenceNode == NULL )
  {
    return;
  }
  if ( imageSequenceNode == NULL || imageSequenceNode->GetNumberOfDataNodes() == 0 )
  {
    return; // Nothing to do if there are zero slices
  }

  // Otherwise, we can grab the slice extents from the first frame
  vtkMRMLVolumeNode* sliceNode = vtkMRMLVolumeNode::SafeDownCast( imageSequenceNode->GetNthDataNode( 0 ) ); // Must have at least one data node
  if ( sliceNode == NULL || sliceNode->GetImageData() == NULL )
  {
    return;
  }
  vtkImageData* sliceImageData = sliceNode->GetImageData();
  if ( sliceImageData->GetDimensions()[ 0 ] == 0 || sliceImageData->GetDimensions()[ 1 ] == 0 || sliceImageData->GetDimensions()[ 2 ] != 1 )
  {
    return;
  }

  // Allocate the output image data from the first slice
  vtkNew< vtkImageData > imageData;
  int* sliceDimensions = sliceImageData->GetDimensions();
  double* sliceSpacing = sliceImageData->GetSpacing();
  imageData->SetDimensions( sliceDimensions[ 0 ], sliceDimensions[ 1 ], imageSequenceNode->GetNumberOfDataNodes() );
  imageData->SetSpacing( sliceSpacing[ 0 ], sliceSpacing[ 1 ], sliceSpacing[ 2 ] );
  imageData->SetOrigin( 0, 0, 0 );

  imageData->AllocateScalars(sliceImageData->GetScalarType(), sliceImageData->GetNumberOfScalarComponents());

  int sliceSize=sliceImageData->GetIncrements()[2];
  for ( int frameNumber = 0; frameNumber < imageSequenceNode->GetNumberOfDataNodes(); frameNumber++ ) // Iterate including the first frame
  {
    std::string indexValue = masterSequenceNode->GetNthIndexValue( frameNumber );

    // Check the image slice is OK
    sliceNode = vtkMRMLVolumeNode::SafeDownCast( imageSequenceNode->GetDataNodeAtValue( indexValue.c_str() ) );
    if ( sliceNode == NULL || sliceNode->GetImageData() == NULL )
    {
      return;
    }
    sliceImageData = sliceNode->GetImageData();
    if ( sliceImageData->GetDimensions()[ 0 ] != sliceDimensions[ 0 ] || sliceImageData->GetDimensions()[ 1 ] != sliceDimensions[ 1 ] || sliceImageData->GetDimensions()[ 2 ] != 1 
      || sliceImageData->GetSpacing()[ 0 ] != sliceSpacing[ 0 ] || sliceImageData->GetSpacing()[ 1 ] != sliceSpacing[ 1 ] || sliceImageData->GetSpacing()[ 2 ] != sliceSpacing[ 2 ] )
    {
      return;
    }

    unsigned char* startPtr=(unsigned char*)imageData->GetScalarPointer(0, 0, frameNumber);
    memcpy(startPtr, sliceImageData->GetScalarPointer(), sliceImageData->GetScalarSize() * sliceSize);
  }

#ifdef ENABLE_PERFORMANCE_PROFILING
  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();  
#endif
  vtkNew< vtkMetaImageWriter > imageWriter;
  imageWriter->SetInputData( imageData.GetPointer() );
  imageWriter->SetFileName( fileName.c_str() ); // This automatically takes care of the file extensions. Note: Saving to .mhd is way faster than saving to .mha
  imageWriter->SetCompression( false );
  imageWriter->Write();
#ifdef ENABLE_PERFORMANCE_PROFILING
  timer->StopTimer();
  vtkInfoMacro("Image writing: " << timer->GetElapsedTime() << "sec\n");
#endif  
}


//----------------------------------------------------------------------------
bool vtkSlicerMetafileImporterLogic::ReadSequenceMetafile(const std::string& fileName, vtkMRMLSequenceBrowserNode** createdBrowserNodePtr /* = NULL */)
{
  if (createdBrowserNodePtr != NULL)
    {
      *createdBrowserNodePtr = NULL;
    }

  // Map the frame numbers to timestamps
  std::map< int, std::string > frameNumberToIndexValueMap;

  int dotFound = fileName.find_last_of( "." );
  int slashFound = fileName.find_last_of( "/" );
  std::string baseNodeName=fileName.substr( slashFound + 1, dotFound - slashFound - 1 );

#ifdef ENABLE_PERFORMANCE_PROFILING
  vtkNew<vtkTimerLog> timer;
  timer->StartTimer();
#endif
  std::deque< vtkMRMLSequenceNode* > createdTransformNodes;
  if (!this->ReadSequenceMetafileTransforms(fileName, baseNodeName, createdTransformNodes, frameNumberToIndexValueMap))
  {
    // error is logged in ReadTransforms
    return false;
  }
#ifdef ENABLE_PERFORMANCE_PROFILING
  timer->StopTimer();
  vtkInfoMacro("ReadTransforms time: " << timer->GetElapsedTime() << "sec\n");
  timer->StartTimer();
#endif
  vtkMRMLSequenceNode* createdImageNode = this->ReadSequenceMetafileImages(fileName, baseNodeName, frameNumberToIndexValueMap);
#ifdef ENABLE_PERFORMANCE_PROFILING
  timer->StopTimer();
  vtkInfoMacro("ReadImages time: " << timer->GetElapsedTime() << "sec\n");
#endif

  // For the user's convenience, create a browser node that contains the image as master node
  // (the first transform node, if no image in the file) and the transforms as synchronized nodes
  vtkMRMLSequenceNode* masterNode=createdImageNode;
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
    if (createdBrowserNodePtr != NULL)
    {
      *createdBrowserNodePtr = sequenceBrowserNode.GetPointer();
    }
    sequenceBrowserNode->SetName(this->GetMRMLScene()->GenerateUniqueName(baseNodeName).c_str());
    this->GetMRMLScene()->AddNode(sequenceBrowserNode);
    sequenceBrowserNode->SetAndObserveMasterSequenceNodeID(masterNode->GetID());
    for (std::deque< vtkMRMLSequenceNode* > ::iterator synchronizedNodesIt = createdTransformNodes.begin();
      synchronizedNodesIt != createdTransformNodes.end(); ++synchronizedNodesIt)
    {
      sequenceBrowserNode->AddSynchronizedSequenceNode((*synchronizedNodesIt)->GetID());
      // Prevent accidental overwriting of transforms
      sequenceBrowserNode->SetSaveChanges(*synchronizedNodesIt, false);
    }
    // Show output volume in the slice viewer
    vtkMRMLVolumeNode* masterProxyVolumeNode = vtkMRMLVolumeNode::SafeDownCast(sequenceBrowserNode->GetProxyNode(masterNode));
    if (masterProxyVolumeNode)
    {
      // Master node is an image.
      // If save changes are allowed then proxy nodes are updated using shallow copy, which is much faster for images.
      sequenceBrowserNode->SetSaveChanges(masterNode, true);
      vtkSlicerApplicationLogic* appLogic = this->GetApplicationLogic();
      vtkMRMLSelectionNode* selectionNode = appLogic ? appLogic->GetSelectionNode() : 0;
      if (selectionNode)
      {
        selectionNode->SetReferenceActiveVolumeID(masterProxyVolumeNode->GetID());
        if (appLogic)
        {
          appLogic->PropagateVolumeSelection();
          appLogic->FitSliceToAll();
        }
      }
    }
  }

  return true;
}


//----------------------------------------------------------------------------
bool vtkSlicerMetafileImporterLogic::WriteSequenceMetafile(const std::string& fileName, vtkMRMLSequenceBrowserNode* createdBrowserNodePtr /* = NULL */)
{
  return this->WriteSequenceMetafile( fileName, &createdBrowserNodePtr );
}

//----------------------------------------------------------------------------
bool vtkSlicerMetafileImporterLogic::WriteSequenceMetafile(const std::string& fileName, vtkMRMLSequenceBrowserNode** createdBrowserNodePtr /* = NULL */)
{
  if (*createdBrowserNodePtr == NULL)
  {
    vtkWarningMacro( "vtkSlicerMetafileImporterLogic::WriteSequenceMetafile: Could not write to file, browser node not specified.");
    return false;
  }
  vtkMRMLSequenceNode* masterSequenceNode = (*createdBrowserNodePtr)->GetMasterSequenceNode();
  if (masterSequenceNode == NULL)
  {
    vtkWarningMacro( "vtkSlicerMetafileImporterLogic::WriteSequenceMetafile: Could not write to file, browser node has no master node.");
    return false;
  }

  vtkNew< vtkCollection > sequenceNodes;
  (*createdBrowserNodePtr)->GetSynchronizedSequenceNodes(sequenceNodes.GetPointer(), true); // Include the master node (since it is probably the image sequence)

  // Find the image sequence node
  vtkMRMLSequenceNode* imageNode = NULL;
  for ( int i = 0; i < sequenceNodes->GetNumberOfItems(); i++ )
  {
    vtkMRMLSequenceNode* currSequenceNode = vtkMRMLSequenceNode::SafeDownCast( sequenceNodes->GetItemAsObject( i ) );
    if ( currSequenceNode == NULL )
    {
      continue;
    }
    vtkMRMLVolumeNode* volumeNode = vtkMRMLVolumeNode::SafeDownCast( currSequenceNode->GetNthDataNode( 0 ) );
    if ( volumeNode != NULL )
    {
      imageNode = currSequenceNode;
      break;
    }
  }

  // Find all of the transform nodes
  std::deque< vtkMRMLSequenceNode* > transformNodes;
  for ( int i = 0; i < sequenceNodes->GetNumberOfItems(); i++ )
  {
    vtkMRMLSequenceNode* currSequenceNode = vtkMRMLSequenceNode::SafeDownCast( sequenceNodes->GetItemAsObject( i ) );
    if ( currSequenceNode == NULL )
    {
      continue;
    }
    vtkMRMLTransformNode* transformNode = vtkMRMLTransformNode::SafeDownCast( currSequenceNode->GetNthDataNode( 0 ) );
    if ( transformNode == NULL ) // Check if the data nodes are a subclass of vtkMRMLTransformNode. This is better than looking at just the class name, because we can't tell inheritance that way.
    {
      continue;
    }
    transformNodes.push_back( currSequenceNode );
  }

  // Need to write the images first so the header file is generated by the vtkMetaImageWriter
  // Then, we can append the transforms to the header
  this->WriteSequenceMetafileImages(fileName, imageNode, masterSequenceNode);
  this->WriteSequenceMetafileTransforms(fileName, transformNodes, masterSequenceNode, imageNode);

  return true;
}


//----------------------------------------------------------------------------
bool vtkSlicerMetafileImporterLogic::ReadVolumeSequence(const std::string& fileName, vtkMRMLSequenceBrowserNode** createdBrowserNodePtr /* = NULL */)
{
  if (createdBrowserNodePtr != NULL)
  {
    *createdBrowserNodePtr = NULL;
  }

  vtkMRMLScene* scene = this->GetMRMLScene();
  if (scene == NULL)
  {
    vtkErrorMacro("vtkSlicerMetafileImporterLogic::ReadVolumeSequence failed: scene is invalid");
    return false;
  }

  vtkNew<vtkMRMLVolumeSequenceStorageNode> storageNode;

  vtkNew<vtkMRMLSequenceNode> volumeSequenceNode;
  std::string volumeName = storageNode->GetFileNameWithoutExtension(fileName.c_str());
  volumeSequenceNode->SetName(scene->GenerateUniqueName(volumeName).c_str());
  scene->AddNode(volumeSequenceNode.GetPointer());

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
  sequenceBrowserNode->SetAndObserveMasterSequenceNodeID(volumeSequenceNode->GetID());

  // If save changes are allowed then proxy nodes are updated using shallow copy, which is much faster for images
  sequenceBrowserNode->SetSaveChanges(volumeSequenceNode.GetPointer(), true);

  if (createdBrowserNodePtr != NULL)
  {
    *createdBrowserNodePtr = sequenceBrowserNode.GetPointer();
  }

  // Show output volume in the slice viewer
  vtkMRMLVolumeNode* masterProxyNode = vtkMRMLVolumeNode::SafeDownCast(sequenceBrowserNode->GetProxyNode(volumeSequenceNode.GetPointer()));
  if (masterProxyNode)
  {
    vtkSlicerApplicationLogic* appLogic = this->GetApplicationLogic();
    vtkMRMLSelectionNode* selectionNode = appLogic ? appLogic->GetSelectionNode() : 0;
    if (selectionNode)
    {
      selectionNode->SetReferenceActiveVolumeID(masterProxyNode->GetID());
      if (appLogic)
      {
        appLogic->PropagateVolumeSelection();
        appLogic->FitSliceToAll();
      }
    }
  }

  return true;
}
