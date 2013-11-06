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
#include "vtkSlicerMultidimDataLogic.h"

// MRMLMultidimData includes
#include "vtkMRMLMultidimDataNode.h"

// MRML includes
#include "vtkMRMLLinearTransformNode.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLScalarVolumeDisplayNode.h"
#include "vtkMRMLNRRDStorageNode.h"

// VTK includes
#include <vtkNew.h>
#include "vtkMatrix4x4.h"
#include "vtkImageData.h"
#include "vtkSmartPointer.h"

#ifdef ENABLE_PERFORMANCE_PROFILING
#include "vtkTimerLog.h"
#endif

// STD includes
#include <sstream>
#include <algorithm>

static const char IMAGE_NODE_BASE_NAME[]="Image";

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerMetafileImporterLogic);

//----------------------------------------------------------------------------
vtkSlicerMetafileImporterLogic
::vtkSlicerMetafileImporterLogic() 
{
  this->MultidimDataLogic = NULL;
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
void vtkSlicerMetafileImporterLogic::SetMultidimDataLogic(vtkSlicerMultidimDataLogic* multiDimensionLogic)
{
  this->MultidimDataLogic=multiDimensionLogic;
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

  transformNode->SetAndObserveMatrixTransformToParent( matrix );
}


// Constants for reading transforms
static const int MAX_LINE_LENGTH = 1000;

static std::string SEQMETA_FIELD_FRAME_FIELD_PREFIX = "Seq_Frame";
static std::string SEQMETA_FIELD_IMG_STATUS = "ImageStatus";

//----------------------------------------------------------------------------
void vtkSlicerMetafileImporterLogic
::ReadTransforms( const std::string& fileName )
{

  // Open in binary mode because we determine the start of the image buffer also during this read
  const char* flags = "rb";
  FILE* stream = fopen( fileName.c_str(), flags ); // TODO: Removed error

  char line[ MAX_LINE_LENGTH + 1 ] = { 0 };

  this->FrameNumberToParameterValueMap.clear();

  // This structure contains all the transform nodes that are read from the file.
  // The nodes are not added immediately to the scene to allow them properly named, using the timestamp parameter value.
  // Maps the frame number to a vector of transform nodes that belong to that frame.
  std::map<int,std::vector<vtkMRMLLinearTransformNode*>> importedTransformNodes;

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
      currentTransform->SetName( frameFieldName.c_str() );
      importedTransformNodes[frameNumber].push_back(currentTransform);
    }

    if ( frameFieldName.find( "Timestamp" ) != std::string::npos )
    {
      this->FrameNumberToParameterValueMap[frameNumber] = value;
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

  for (int currentFrameNumber=0; currentFrameNumber<=lastFrameNumber; currentFrameNumber++)
  {
    std::map<int,std::vector<vtkMRMLLinearTransformNode*>>::iterator transformsForCurrentFrame=importedTransformNodes.find(currentFrameNumber);
    if ( transformsForCurrentFrame == importedTransformNodes.end() )
    {
      // no transforms for this frame
      continue;
    }
    std::string paramValueString=this->FrameNumberToParameterValueMap[currentFrameNumber];
    for (std::vector<vtkMRMLLinearTransformNode*>::iterator transformIt=transformsForCurrentFrame->second.begin(); transformIt!=transformsForCurrentFrame->second.end(); ++transformIt)
    {
      vtkMRMLLinearTransformNode* transform=(*transformIt);
      transform->SetHideFromEditors(true);
      // The transform name at this point is simply the field name (e.g., ProbeToTracker), need to generate a full name (ProbeToTracker time=123.43s)     
      this->GetMRMLScene()->AddNode(transform);
      transform->Delete(); // ownership transferred to the scene
      this->RootNode->SetDataNodeAtValue(transform, transform->GetName(), paramValueString.c_str() );
      this->RootNode->UpdateNodeName(transform, paramValueString.c_str());
    }
  }  

}

//----------------------------------------------------------------------------
// Read the spacing and dimentions of the image.
void vtkSlicerMetafileImporterLogic
::ReadImages( const std::string& fileName )
{
  // Grab the image data from the mha file

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

  vtkSmartPointer< vtkImageData > imageData = imageReader->GetOutput();
  int* dimensions = imageData->GetDimensions();
  double* spacing = imageData->GetSpacing();

  vtkSmartPointer<vtkImageData> emptySliceImageData=vtkSmartPointer<vtkImageData>::New();
  emptySliceImageData->SetDimensions(dimensions[0],dimensions[1],1);
  emptySliceImageData->SetSpacing(spacing[0],spacing[1],1);
  emptySliceImageData->SetOrigin(0,0,0);  
  emptySliceImageData->SetScalarType(imageData->GetScalarType());
  emptySliceImageData->SetNumberOfScalarComponents(imageData->GetNumberOfScalarComponents()); 
  
  int sliceSize=imageData->GetIncrements()[2];
  for ( int frameNumber = 0; frameNumber < dimensions[2]; frameNumber++ )
  {     
    // Add the image slice to scene as a volume    

    vtkSmartPointer< vtkMRMLScalarVolumeNode > slice = vtkSmartPointer< vtkMRMLScalarVolumeNode >::New();
    vtkSmartPointer<vtkImageData> sliceImageData=vtkSmartPointer<vtkImageData>::New();
    sliceImageData->DeepCopy(emptySliceImageData);
    sliceImageData->AllocateScalars();
    unsigned char* startPtr=(unsigned char*)imageData->GetScalarPointer(0, 0, frameNumber);
    memcpy(sliceImageData->GetScalarPointer(), startPtr, sliceSize);

    slice->SetName(IMAGE_NODE_BASE_NAME);
    slice->SetAndObserveImageData(sliceImageData);

    std::string paramValueString=this->FrameNumberToParameterValueMap[frameNumber];
    slice->SetHideFromEditors(true);
    this->GetMRMLScene()->AddNode(slice);

    this->RootNode->SetDataNodeAtValue(slice, slice->GetName(), paramValueString.c_str() );
    this->RootNode->UpdateNodeName(slice, paramValueString.c_str());

    // Create display node
    // TODO: add the display node to the MultidimData hierarchy?
    vtkSmartPointer< vtkMRMLScalarVolumeDisplayNode > displayNode = vtkSmartPointer< vtkMRMLScalarVolumeDisplayNode >::New();
    displayNode->SetDefaultColorMap();
    displayNode->SetHideFromEditors(true);
    std::string displayNodeName=std::string(slice->GetName())+" display";
    displayNode->SetName(displayNodeName.c_str());
    this->GetMRMLScene()->AddNode( displayNode );
    slice->SetAndObserveDisplayNodeID( displayNode->GetID() );   

    // Create storage node
    vtkMRMLStorageNode *storageNode = slice->CreateDefaultStorageNode();    
    if (storageNode)
    {
      this->GetMRMLScene()->AddNode(storageNode);
      storageNode->Delete(); // now the scene owns the storage node
      slice->SetAndObserveStorageNodeID(storageNode->GetID());
      std::string filename=std::string(slice->GetName())+".nrrd";
      storageNode->SetFileName(filename.c_str());
      slice->StorableModified(); // marks as modified, so the volume will be written to file on save
    }
    else
    {
      vtkErrorMacro("Failed to create storage node for the imported slice");
    }

  }
}

//----------------------------------------------------------------------------
void vtkSlicerMetafileImporterLogic
::Read( std::string fileName )
{
//  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState);

  // Setup hierarchy structure
  vtkSmartPointer<vtkMRMLMultidimDataNode> rootNode = vtkSmartPointer<vtkMRMLMultidimDataNode>::New();  
  this->GetMRMLScene()->AddNode(rootNode);
  this->RootNode = rootNode;

  this->RootNode->SetDimensionName("time");
  this->RootNode->SetUnit("s");
  int dotFound = fileName.find_last_of( "." );
  int slashFound = fileName.find_last_of( "/" );
  std::string rootName=fileName.substr( slashFound + 1, dotFound - slashFound - 1 );
  this->RootNode->SetName( rootName.c_str() );

  int rootNodeDisableModify = this->RootNode->StartModify();
#ifdef ENABLE_PERFORMANCE_PROFILING
  vtkSmartPointer<vtkTimerLog> timer=vtkSmartPointer<vtkTimerLog>::New();      
  timer->StartTimer();
#endif
  this->ReadTransforms( fileName ); // TODO: Removed error macro
#ifdef ENABLE_PERFORMANCE_PROFILING
  timer->StopTimer();
  vtkWarningMacro("ReadTransforms time: " << timer->GetElapsedTime() << "sec\n");
  timer->StartTimer();
#endif
  this->ReadImages( fileName ); // TODO: Removed error macro
#ifdef ENABLE_PERFORMANCE_PROFILING
  timer->StopTimer();
  vtkWarningMacro("ReadImages time: " << timer->GetElapsedTime() << "sec\n");
#endif

//  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState);

  this->RootNode->EndModify(rootNodeDisableModify);

  // Loading is completed indicate to modules that the hierarchy is changed
  this->RootNode->Modified();
  
  this->RootNode=NULL;
  this->FrameNumberToParameterValueMap.clear();
}
