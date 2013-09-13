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
#include "vtkSlicerMultiDimensionLogic.h"

// MRML includes

// VTK includes
#include <vtkNew.h>
#include "vtkMatrix4x4.h"
#include "vtkMRMLLinearTransformNode.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkImageData.h"
#include "vtkMRMLScalarVolumeDisplayNode.h"
#include "vtkSmartPointer.h"

#ifdef ENABLE_PERFORMANCE_PROFILING
#include "vtkTimerLog.h"
#endif

// STD includes
#include <cassert>
#include <sstream>



//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerMetafileImporterLogic);

//----------------------------------------------------------------------------
vtkSlicerMetafileImporterLogic
::vtkSlicerMetafileImporterLogic() 
{
  this->MultiDimensionLogic = NULL;
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
  assert(this->GetMRMLScene() != 0);
}

//---------------------------------------------------------------------------
void vtkSlicerMetafileImporterLogic::UpdateFromMRMLScene()
{
  assert(this->GetMRMLScene() != 0);
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
::ReadTransforms( std::string fileName )
{

  // Open in binary mode because we determine the start of the image buffer also during this read
  const char* flags = "rb";
  FILE* stream = fopen( fileName.c_str(), flags ); // TODO: Removed error

  char line[ MAX_LINE_LENGTH + 1 ] = { 0 };

  this->FrameToTimeMap.clear();

  //int rootNodeDisableModify = this->RootNode->StartModify();
  while ( fgets( line, MAX_LINE_LENGTH, stream ) )
  {

    std::string lineStr = line;

    // Split line into name and value
    size_t equalSignFound;
    equalSignFound = lineStr.find_first_of( "=" );
    if ( equalSignFound == std::string::npos )
    {
      // vtkWarningMacro("Parsing line failed, equal sign is missing ("<<lineStr<<")");
      continue;
    }
    std::string name = lineStr.substr( 0, equalSignFound );
    std::string value = lineStr.substr( equalSignFound + 1 );

    // Trim spaces from the left and right
    Trim( name );
    Trim( value );

    // Only consider the Seq_Frame
    if ( name.compare( 0, SEQMETA_FIELD_FRAME_FIELD_PREFIX.size(), SEQMETA_FIELD_FRAME_FIELD_PREFIX ) != 0 )
    {
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
      // vtkWarningMacro("Parsing line failed, underscore is missing from frame field name ("<<lineStr<<")");
      continue;
    }

    std::string frameNumberStr = name.substr( 0, underscoreFound ); // 0000
    std::string frameFieldName = name.substr( underscoreFound + 1 ); // CustomTransform

    int frameNumber = 0;
    StringToInt( frameNumberStr.c_str(), frameNumber ); // TODO: Removed warning
      
    // Convert the string to transform and add transform to hierarchy
    if ( frameFieldName.find( "Transform" ) != std::string::npos && frameFieldName.find( "Status" ) == std::string::npos )
    {
      vtkSmartPointer<vtkMRMLLinearTransformNode> currentTransform = vtkSmartPointer<vtkMRMLLinearTransformNode>::New();
      UpdateTransformNodeFromString(currentTransform, value);
      std::stringstream transformName;
      transformName << frameFieldName.c_str();// << "_" << std::setw( 5 ) << std::setfill( '0' ) << frameNumber;
      currentTransform->SetName( transformName.str().c_str() );
      currentTransform->SetHideFromEditors(true);

      this->GetMRMLScene()->AddNode( currentTransform );

      std::stringstream valueString;
      valueString << frameNumber;
      this->MultiDimensionLogic->AddDataNodeAtValue( this->RootNode, currentTransform, valueString.str().c_str() );
    }

    if ( frameFieldName.find( "Timestamp" ) != std::string::npos )
    {
      std::stringstream frameStream;
      frameStream << frameNumber;
      this->FrameToTimeMap[ frameStream.str() ] = value;
      std::string testValue = this->FrameToTimeMap[ frameStream.str() ];
      std::string test;
    }

    if ( ferror( stream ) )
    {
      // vtkErrorMacro("Error reading the file "<<this->FileName);
      break;
    }
    if ( feof( stream ) )
    {
      break;
    }

  }
  //this->RootNode->EndModify(rootNodeDisableModify);

  fclose( stream );
}

//----------------------------------------------------------------------------
// Read the spacing and dimentions of the image.
void vtkSlicerMetafileImporterLogic
::ReadImages( std::string fileName )
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
  int rootNodeDisableModify = this->RootNode->StartModify();
  for ( int i = 0; i < dimensions[2]; i++ )
  {     
    // Add the image slice to scene as a volume
    vtkSmartPointer< vtkMRMLScalarVolumeDisplayNode > displayNode = vtkSmartPointer< vtkMRMLScalarVolumeDisplayNode >::New();

    vtkSmartPointer< vtkMRMLScalarVolumeNode > slice = vtkSmartPointer< vtkMRMLScalarVolumeNode >::New();
    vtkSmartPointer<vtkImageData> sliceImageData=vtkSmartPointer<vtkImageData>::New();
    sliceImageData->DeepCopy(emptySliceImageData);
    sliceImageData->AllocateScalars();
    unsigned char* startPtr=(unsigned char*)imageData->GetScalarPointer(0, 0, i);
    memcpy(sliceImageData->GetScalarPointer(), startPtr, sliceSize);

    slice->SetAndObserveImageData(sliceImageData);
    std::stringstream sliceName;
    sliceName << "Image";// << "_" << std::setw( 5 ) << std::setfill( '0' ) << i;
    slice->SetName( sliceName.str().c_str() );
    slice->SetScene( this->GetMRMLScene() );
    slice->SetHideFromEditors(true);

    slice->SetAndObserveDisplayNodeID( displayNode->GetID() );

    this->GetMRMLScene()->AddNode( slice );

    std::stringstream valueString;
    valueString << i;
    this->MultiDimensionLogic->AddDataNodeAtValue( this->RootNode, slice, valueString.str().c_str() );
  }
  this->RootNode->EndModify(rootNodeDisableModify);

}


//----------------------------------------------------------------------------
void vtkSlicerMetafileImporterLogic
::Read( std::string fileName )
{
//  this->GetMRMLScene()->StartState(vtkMRMLScene::BatchProcessState);

  // Setup hierarchy structure
  this->RootNode = this->MultiDimensionLogic->CreateMultiDimensionRootNode();
  this->RootNode->SetAttribute("MultiDimension.Name", "Time");
  this->RootNode->SetAttribute("MultiDimension.Unit", "Sec");
  int dotFound = fileName.find_last_of( "." );
  int slashFound = fileName.find_last_of( "/" );
  std::string rootName=fileName.substr( slashFound + 1, dotFound - slashFound - 1 );
  this->RootNode->SetName( rootName.c_str() );

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

  this->MultiDimensionLogic->UpdateValues( this->RootNode, this->FrameToTimeMap );

//  this->GetMRMLScene()->EndState(vtkMRMLScene::BatchProcessState);

  // Loading is completed indicate to modules that the hierarchy is changed
  this->RootNode->Modified();
  
  this->RootNode=NULL;
  this->FrameToTimeMap.clear();
}
