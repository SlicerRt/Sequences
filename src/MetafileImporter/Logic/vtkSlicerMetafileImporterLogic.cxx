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

// MetafileImporter Logic includes
#include "vtkSlicerMetafileImporterLogic.h"
#include "vtkSlicerMultiDimensionLogic.h"

// MRML includes

// VTK includes
#include <vtkNew.h>
#include "vtkMatrix4x4.h"
#include "vtkMRMLLinearTransformNode.h"

// STD includes
#include <cassert>


// Directly from PLUS --------------------------------------------------------

#include "itk_zlib.h"
#include "vtkSlicerMetafileImporterLogic.h"
#include "vtksys/SystemTools.hxx"
#include <iomanip>
#include <iostream>


#ifdef _WIN32
  #define FSEEK _fseeki64
  #define FTELL _ftelli64
#else
  #define FSEEK fseek
  #define FTELL ftell
#endif


//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerMetafileImporterLogic);

//----------------------------------------------------------------------------
vtkSlicerMetafileImporterLogic::vtkSlicerMetafileImporterLogic() 
: UseCompression(false)
, FileType(itk::ImageIOBase::Binary)
, PixelType(itk::ImageIOBase::UNKNOWNCOMPONENTTYPE)
, NumberOfComponents(1)
, NumberOfDimensions(3)
, m_CurrentFrameOffset(0)
, m_TotalBytesWritten(0)
, ImageOrientationInFile(US_IMG_ORIENT_XX)
, ImageOrientationInMemory(US_IMG_ORIENT_XX)
, ImageType(US_IMG_TYPE_XX)
, PixelDataFileOffset(0)
{
  this->Dimensions[0]=0;
  this->Dimensions[1]=0;
  this->Dimensions[2]=0;

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

//-------------------------------------------------------
std::string Trim(const char* c)
{
  std::string str = c;
  str.erase(str.find_last_not_of(" \t\r\n")+1);
  str.erase(0,str.find_first_not_of(" \t\r\n"));
  return str;
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


//----------------------------------------------------------------------------
US_IMAGE_ORIENTATION GetUsImageOrientationFromString( const char* imgOrientationStr )
{
  US_IMAGE_ORIENTATION imgOrientation = US_IMG_ORIENT_XX;

  //TODO: Make lettercase independents
  if ( imgOrientationStr == NULL )
  {
    return imgOrientation;
  }
  else if ( strcmp(imgOrientationStr, "UF" ) == 0 )
  {
    imgOrientation = US_IMG_ORIENT_UF;
  }
  else if ( strcmp(imgOrientationStr, "UN" ) == 0 )
  {
    imgOrientation = US_IMG_ORIENT_UN;
  }
  else if ( strcmp(imgOrientationStr, "MF" ) == 0 )
  {
    imgOrientation = US_IMG_ORIENT_MF;
  }
  else if ( strcmp(imgOrientationStr, "MN" ) == 0 )
  {
    imgOrientation = US_IMG_ORIENT_MN;
  }
  else if ( strcmp(imgOrientationStr, "FU" ) == 0 )
  {
    imgOrientation = US_IMG_ORIENT_FU;
  }
  else if ( strcmp(imgOrientationStr, "NU" ) == 0 )
  {
    imgOrientation = US_IMG_ORIENT_NU;
  }
  else if ( strcmp(imgOrientationStr, "FM" ) == 0 )
  {
    imgOrientation = US_IMG_ORIENT_FM;
  }
  else if ( strcmp(imgOrientationStr, "NM" ) == 0 )
  {
    imgOrientation = US_IMG_ORIENT_NM;
  }

  return imgOrientation;
}

//----------------------------------------------------------------------------
US_IMAGE_TYPE GetUsImageTypeFromString( const char* imgTypeStr )
{
  US_IMAGE_TYPE imgType = US_IMG_TYPE_XX;
  if ( imgTypeStr == NULL )
  {
    return imgType;
  }
  else if ( strcmp(imgTypeStr, "BRIGHTNESS" ) == 0 )
  {
    imgType = US_IMG_BRIGHTNESS;
  }
  else if ( strcmp(imgTypeStr, "RF_REAL" ) == 0 )
  {
    imgType = US_IMG_RF_REAL;
  }
  else if ( strcmp(imgTypeStr, "RF_IQ_LINE" ) == 0 )
  {
    imgType = US_IMG_RF_IQ_LINE;
  }
  else if ( strcmp(imgTypeStr, "RF_I_LINE_Q_LINE" ) == 0 )
  {
    imgType = US_IMG_RF_I_LINE_Q_LINE;
  }
  return imgType;
}


//----------------------------------------------------------------------------
int GetNumberOfBytesPerPixel(ITKScalarPixelType pixelType)
{
  switch (pixelType)
  {
    case itk::ImageIOBase::UCHAR: return sizeof(unsigned char);
    case itk::ImageIOBase::CHAR: return sizeof(char);
    case itk::ImageIOBase::USHORT: return sizeof(unsigned short);
    case itk::ImageIOBase::SHORT: return sizeof(short);
    case itk::ImageIOBase::UINT: return sizeof(unsigned int);
    case itk::ImageIOBase::INT: return sizeof(int);
    case itk::ImageIOBase::ULONG: return sizeof(unsigned long);
    case itk::ImageIOBase::LONG: return sizeof(long);
    case itk::ImageIOBase::FLOAT: return sizeof(float);
    case itk::ImageIOBase::DOUBLE: return sizeof(double);
    default:
      // vtkErrorMacro("GetNumberOfBytesPerPixel: unknown pixel type");
      return itk::ImageIOBase::UNKNOWNCOMPONENTTYPE;
  }
}


// Size of MetaIO fields, in bytes (adopted from metaTypes.h)
enum
{
  MET_NONE, MET_ASCII_CHAR, MET_CHAR, MET_UCHAR, MET_SHORT,
  MET_USHORT, MET_INT, MET_UINT, MET_LONG, MET_ULONG,
  MET_LONG_LONG, MET_ULONG_LONG, MET_FLOAT, MET_DOUBLE, MET_STRING,
  MET_CHAR_ARRAY, MET_UCHAR_ARRAY, MET_SHORT_ARRAY, MET_USHORT_ARRAY, MET_INT_ARRAY,
  MET_UINT_ARRAY, MET_LONG_ARRAY, MET_ULONG_ARRAY, MET_LONG_LONG_ARRAY, MET_ULONG_LONG_ARRAY,
  MET_FLOAT_ARRAY, MET_DOUBLE_ARRAY, MET_FLOAT_MATRIX, MET_OTHER,
  // insert values before this line
  MET_NUM_VALUE_TYPES
};
static const unsigned char MET_ValueTypeSize[MET_NUM_VALUE_TYPES] =
{
   0, 1, 1, 1, 2,
   2, 4, 4, 4, 4,
   8, 8, 4, 8, 1,
   1, 1, 2, 2, 4,
   4, 4, 4, 8, 8,
   4, 8, 4, 0
};






static const int MAX_LINE_LENGTH=1000;


static const char* SEQMETA_FIELD_US_IMG_ORIENT = "UltrasoundImageOrientation";
static const char* SEQMETA_FIELD_US_IMG_TYPE = "UltrasoundImageType";
static const char* SEQMETA_FIELD_ELEMENT_DATA_FILE = "ElementDataFile";
static const char* SEQMETA_FIELD_VALUE_ELEMENT_DATA_FILE_LOCAL = "LOCAL";

static std::string SEQMETA_FIELD_FRAME_FIELD_PREFIX = "Seq_Frame";
static std::string SEQMETA_FIELD_IMG_STATUS = "ImageStatus";


vtkMRMLLinearTransformNode* StringToTransformNode( std::string str )
{
  std::stringstream ss( str );
  
  double e00; ss >> e00; double e01; ss >> e01; double e02; ss >> e02; double e03; ss >> e03;
  double e10; ss >> e10; double e11; ss >> e11; double e12; ss >> e12; double e13; ss >> e13;
  double e20; ss >> e20; double e21; ss >> e21; double e22; ss >> e22; double e23; ss >> e23;
  double e30; ss >> e30; double e31; ss >> e31; double e32; ss >> e32; double e33; ss >> e33;

  vtkMatrix4x4* matrix = vtkMatrix4x4::New();

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

  vtkMRMLLinearTransformNode* transformNode = vtkMRMLLinearTransformNode::New();
  transformNode->SetAndObserveMatrixTransformToParent( matrix );

  return transformNode;
}




//----------------------------------------------------------------------------
void vtkSlicerMetafileImporterLogic::ReadImageHeader()
{
  FILE *stream=NULL;
  // open in binary mode because we determine the start of the image buffer also during this read
  this->FileOpen(&stream, this->FileName.c_str(), "rb" ); // TODO: Removed error

  char line[MAX_LINE_LENGTH+1]={0};
  while (fgets( line, MAX_LINE_LENGTH, stream ))
  {

    std::string lineStr=line;

    // Split line into name and value
    size_t equalSignFound;
    equalSignFound=lineStr.find_first_of("=");
    if (equalSignFound==std::string::npos)
    {
      // vtkWarningMacro("Parsing line failed, equal sign is missing ("<<lineStr<<")");
      continue;
    }
    std::string name=lineStr.substr(0,equalSignFound);
    std::string value=lineStr.substr(equalSignFound+1);

    // trim spaces from the left and right
    Trim(name);
    Trim(value);

    if (name.compare(0,SEQMETA_FIELD_FRAME_FIELD_PREFIX.size(),SEQMETA_FIELD_FRAME_FIELD_PREFIX)!=0)
    {
      // field
      this->SetImageProperty( name.c_str(), value.c_str() );

      // Arrived to ElementDataFile, this is the last element
      if (name.compare(SEQMETA_FIELD_ELEMENT_DATA_FILE)==0)
      {
        this->PixelDataFileName=value;
        if (value.compare(SEQMETA_FIELD_VALUE_ELEMENT_DATA_FILE_LOCAL)==0)
        {
          // pixel data stored locally
          this->PixelDataFileOffset=FTELL(stream);
        }
        else
        {
          // pixel data stored in separate file
          this->PixelDataFileOffset=0;
        }
        // this is the last element of the header
        break;
      }
    }
    else
    {
      // frame field
      // name: Seq_Frame0000_CustomTransform
      name.erase(0,SEQMETA_FIELD_FRAME_FIELD_PREFIX.size()); // 0000_CustomTransform

      // Split line into name and value
      size_t underscoreFound;
      underscoreFound=name.find_first_of("_");
      if (underscoreFound==std::string::npos)
      {
        // vtkWarningMacro("Parsing line failed, underscore is missing from frame field name ("<<lineStr<<")");
        continue;
      }
      std::string frameNumberStr=name.substr(0,underscoreFound); // 0000
      std::string frameFieldName=name.substr(underscoreFound+1); // CustomTransform

      int frameNumber=0;
      StringToInt(frameNumberStr.c_str(),frameNumber); // TODO: Removed warning
      
      // Convert the string to transform and add transform to hierarchy
      vtkMRMLLinearTransformNode* currentTransform = StringToTransformNode( value );
      this->MultiDimensionLogic->AddChildNodeAtTimePoint( rootNode, currentTransform, &frameNumberStr[0] );

      if (ferror(stream))
      {
        // vtkErrorMacro("Error reading the file "<<this->FileName);
        break;
      }
      if (feof(stream))
      {
        break;
      }
    }
  }

  fclose( stream );

}

//----------------------------------------------------------------------------
// Read the spacing and dimentions of the image.
void vtkSlicerMetafileImporterLogic::ReadImagePixels()
{
  int numberOfErrors=0;

  FILE *stream=NULL;

  FileOpen( &stream, GetPixelDataFilePath().c_str(), "rb" );  // TODO: Removed error macro

  int frameCount=this->Dimensions[2];
  unsigned int frameSizeInBytes=0;
  if (this->Dimensions[0]>0 && this->Dimensions[1]>0)
  {
    frameSizeInBytes=this->Dimensions[0]*this->Dimensions[1]*GetNumberOfBytesPerPixel(this->PixelType);
  }

  std::vector<unsigned char> allFramesPixelBuffer;
  if (this->UseCompression)
  {
    unsigned int allFramesPixelBufferSize=frameCount*frameSizeInBytes;

    try
    {
      allFramesPixelBuffer.resize(allFramesPixelBufferSize);
    }
    catch(std::bad_alloc& e)
    {
      cerr << e.what() << endl;
      // vtkErrorMacro("vtkSlicerMetafileImporterLogic::ReadImagePixels failed due to out of memory. Try to reduce image buffer sizes or use a 64-bit build of Plus.");
      return;
    }

    unsigned int allFramesCompressedPixelBufferSize = this->CompressedDataSize;
    std::vector<unsigned char> allFramesCompressedPixelBuffer;
    allFramesCompressedPixelBuffer.resize(allFramesCompressedPixelBufferSize);

    FSEEK(stream, this->PixelDataFileOffset, SEEK_SET);
    if (fread(&(allFramesCompressedPixelBuffer[0]), 1, allFramesCompressedPixelBufferSize, stream)!=allFramesCompressedPixelBufferSize)
    {
      // vtkErrorMacro("Could not read "<<allFramesCompressedPixelBufferSize<<" bytes from "<<GetPixelDataFilePath());
      fclose( stream );
      return;
    }

    uLongf unCompSize = allFramesPixelBufferSize;
    if (uncompress((Bytef*)&(allFramesPixelBuffer[0]), &unCompSize, (const Bytef*)&(allFramesCompressedPixelBuffer[0]), allFramesCompressedPixelBufferSize)!=Z_OK)
    {
      // vtkErrorMacro("Cannot uncompress the pixel data");
      fclose( stream );
      return;
    }
    if (unCompSize!=allFramesPixelBufferSize)
    {
      // vtkErrorMacro("Cannot uncompress the pixel data: uncompressed data is less than expected");
      fclose( stream );
      return;
    }

  }

  std::vector<unsigned char> pixelBuffer;
  pixelBuffer.resize(frameSizeInBytes);
  for (int frameNumber=0; frameNumber<frameCount; frameNumber++)
  {

    itk::Image<unsigned char, 2>::Pointer imagePointer;

    // TODO: Create a volume node here
    //CreateTrackedFrameIfNonExisting(frameNumber);
    //TrackedFrame* trackedFrame=this->TrackedFrameList->GetTrackedFrame(frameNumber);

    // TODO: Deal with invalid image statuses
    // Allocate frame only if it is valid
    //const char* imgStatus = trackedFrame->GetCustomFrameField(SEQMETA_FIELD_IMG_STATUS.c_str());
    //if ( imgStatus != NULL  ) // Found the image status field
    //{
    //  // Save status field
    //  std::string strImgStatus(imgStatus);
    //
    //  // Delete image status field from tracked frame
    //  // Image status can be determine by trackedFrame->GetImageData()->IsImageValid()
    //  trackedFrame->DeleteCustomFrameField(SEQMETA_FIELD_IMG_STATUS.c_str());
    //
    //  if ( STRCASECMP(strImgStatus.c_str(), "OK") != 0 )// Image status _not_ OK
    //  {
    //   // vtkWarningMacro("Frame #" << frameNumber << " image data is invalid, no need to allocate data in the tracked frame list.");
    //  continue;
    //  }
    //}

    // TODO: Set the volume node image properties here
    //trackedFrame->GetImageData()->SetImageOrientation(this->ImageOrientationInMemory);
    //trackedFrame->GetImageData()->SetImageType(this->ImageType);
    //if (trackedFrame->GetImageData()->AllocateFrame(this->Dimensions, this->PixelType)!=PLUS_SUCCESS)
    //{
    //  // vtkErrorMacro("Cannot allocate memory for frame "<<frameNumber);
    //  numberOfErrors++;
    //  continue;
    //}

    if (!this->UseCompression)
    {
      FilePositionOffsetType offset=PixelDataFileOffset+frameNumber*frameSizeInBytes;
      FSEEK(stream, offset, SEEK_SET);
      if (fread(&(pixelBuffer[0]), 1, frameSizeInBytes, stream)!=frameSizeInBytes)
      {
        // vtkErrorMacro("Could not read "<<frameSizeInBytes<<" bytes from "<<GetPixelDataFilePath());
      }
      // TODO: Add the image data to a volume node, and add the volume node to the hierarchy
      this->GetOrientedImage(&(pixelBuffer[0]), this->ImageOrientationInFile, this->ImageType, this->Dimensions, this->PixelType, this->ImageOrientationInMemory, imagePointer );
    }
    else
    {
      // TODO: Add the image data to a volume node, and add the volume node to the hierarchy
      this->GetOrientedImage(&(allFramesPixelBuffer[0])+frameNumber*frameSizeInBytes, this->ImageOrientationInFile, this->ImageType, this->Dimensions, this->PixelType, this->ImageOrientationInMemory, imagePointer );
    }
  }

  fclose( stream );

  if (numberOfErrors>0)
  {
    return;
  }

}


void vtkSlicerMetafileImporterLogic::SetFileName( const char* aFilename )
{
  this->FileName.clear();
  this->PixelDataFileName.clear();

  if( aFilename == NULL )
  {
    // vtkErrorMacro("Invalid metaimage file name");
  }
  
  this->FileName = aFilename;
  // Trim whitespace and " characters from the beginning and end of the filename
  this->FileName.erase(this->FileName.find_last_not_of(" \"\t\r\n")+1);
  this->FileName.erase(0,this->FileName.find_first_not_of(" \"\t\r\n"));

  // Set pixel data filename at the same time
  std::string fileExt = vtksys::SystemTools::GetFilenameLastExtension(this->FileName);
  if ( strcmp(fileExt.c_str(),".mha")==0) // TODO: Remove case sensitivity
  {
    this->PixelDataFileName=SEQMETA_FIELD_VALUE_ELEMENT_DATA_FILE_LOCAL;
  }
  else if ( strcmp(fileExt.c_str(),".mhd")==0) // TODO: Remove case sensitivity
  {
    std::string pixFileName=vtksys::SystemTools::GetFilenameWithoutExtension(this->FileName);
    if (this->UseCompression)
    {
      pixFileName+=".zraw";
    }
    else
    {
      pixFileName+=".raw";
    }

    this->PixelDataFileName=pixFileName;
  }
  else
  {
    // vtkWarningMacro("Writing sequence metafile with '" << fileExt << "' extension is not supported. Using mha extension instead.");
    this->FileName+=".mha";
    this->PixelDataFileName=SEQMETA_FIELD_VALUE_ELEMENT_DATA_FILE_LOCAL;
  }

}



//----------------------------------------------------------------------------
void vtkSlicerMetafileImporterLogic::Read()
{
  // TODO: Setup hierarchy structure

  this->rootNode = this->MultiDimensionLogic->CreateMultiDimensionRootNode();

  this->ReadImageHeader(); // TODO: Removed error macro
  // this->ReadImagePixels(); // TODO: Removed error macro

}




//----------------------------------------------------------------------------
std::string vtkSlicerMetafileImporterLogic::GetPixelDataFilePath()
{
  // TODO: Remove case sensitivity
  if (strcmp(SEQMETA_FIELD_VALUE_ELEMENT_DATA_FILE_LOCAL, this->PixelDataFileName.c_str())==0)
  {
    // LOCAL => data is stored in one file
    return this->FileName;
  }

  std::string dir=vtksys::SystemTools::GetFilenamePath(this->FileName);
  if (!dir.empty())
  {
    dir+="/";
  }
  std::string path=dir+this->PixelDataFileName;
  return path;
}



//----------------------------------------------------------------------------
void vtkSlicerMetafileImporterLogic
::SetImageProperty( const char* propertyName, const char* propertyValue )
{
  // Binary Data
  if ( strcmp( propertyName, "BinaryData" ) == 0 )
  {
    if ( strcmp( propertyValue, "TRUE" ) == 0 )
    {
      this->FileType=itk::ImageIOBase::Binary;
    }
    else
    {
      this->FileType=itk::ImageIOBase::ASCII;
    }
  }

  // Compressed Data
  if ( strcmp( propertyName, "CompressedData" ) == 0 )
  {
    if ( strcmp( propertyValue, "TRUE" ) == 0 )
    {
      this->UseCompression = true;
    }
    else
    {
      this->UseCompression = false;
      this->FileType=itk::ImageIOBase::ASCII;
    }
  }

  // Compressed Data Size
  if ( strcmp( propertyName, "CompressedDataSize" ) == 0 )
  {
    StringToInt( propertyValue, this->CompressedDataSize );
  }

  // Element Number of Channels
  if ( strcmp( propertyName, "ElementNumberOfChannels" ) == 0 )
  {
    int numberOfComponents = 1;
    StringToInt( propertyValue, numberOfComponents );
    if ( numberOfComponents != 1 )
    {
      // vtkErrorMacro("Images images with ElementNumberOfChannels=1 are supported. This image has "<<numberOfComponents<<" channels.");
    }
  }

  // Number of dimensions
  if ( strcmp( propertyName, "NDims" ) == 0 )
  {
    StringToInt( propertyValue, this->NumberOfDimensions ); //TODO: Removed error macro
    if ( this->NumberOfDimensions !=2 && this->NumberOfDimensions !=3)
    {
      // vtkErrorMacro("Invalid dimension (shall be 2 or 3)");
    }
  }

  // Image orientation
  if ( strcmp( propertyName, SEQMETA_FIELD_US_IMG_ORIENT ) == 0 )
  {
    this->ImageOrientationInFile = GetUsImageOrientationFromString( propertyValue );
  }

  // Ultrasound image orientation
  if ( strcmp( propertyName, SEQMETA_FIELD_US_IMG_TYPE ) == 0 )
  {
    this->ImageType = GetUsImageTypeFromString( propertyValue );
  }

  // Dimension sizes
  if ( strcmp( propertyName, "DimSize" ) == 0 )
  {
    std::stringstream ssDimSize( propertyValue );
    for( int i = 0; i < 3; i++ )
    {
      int dimSize=0;
      if ( i < this->NumberOfDimensions )
      {
        ssDimSize >> dimSize;
      }
      this->Dimensions[i]=dimSize;
    }
  }

  // If no specific image orientation is requested then determine it automatically from the image type
  // B-mode: MF
  // RF-mode: FM
  if (this->ImageOrientationInMemory==US_IMG_ORIENT_XX)
  {
    switch (this->ImageType)
    {
    case US_IMG_BRIGHTNESS:
      this->ImageOrientationInMemory = US_IMG_ORIENT_MF;
      break;
    case US_IMG_RF_I_LINE_Q_LINE:
    case US_IMG_RF_IQ_LINE:
    case US_IMG_RF_REAL:
      this->ImageOrientationInMemory = US_IMG_ORIENT_FM;
      break;
    default:
      if ( this->Dimensions[0] == 0 )
      {
        // vtkWarningMacro("Only tracking data is available in the metafile");
      }
      else
      {
        // vtkWarningMacro("Cannot determine image orientation automatically, unknown image type "<<this->ImageType<<", use the same orientation in memory as in the file");
      }
      this->ImageOrientationInMemory = this->ImageOrientationInFile;
    }
  }

}



// ---------------------------------------------------------------------------

void vtkSlicerMetafileImporterLogic
::GetOrientedImage( unsigned char* imageDataPtr, US_IMAGE_ORIENTATION  inUsImageOrientation, US_IMAGE_TYPE inUsImageType, const int frameSizeInPx[2],
                     int numberOfBitsPerPixel, US_IMAGE_ORIENTATION  outUsImageOrientation, itk::Image<unsigned char, 2>::Pointer outUsOrientedImage )
{

  if ( imageDataPtr == NULL )
  {
    // vtkErrorMacro("Failed to convert image data to MF orientation - input image is null!"); 
    return; 
  }

  if ( outUsOrientedImage.IsNull() )
  {
    // vtkErrorMacro("Failed to convert image data to MF orientation - output image is null!"); 
    return; 
  }

  if ( numberOfBitsPerPixel != sizeof(itk::Image<unsigned char, 2>::PixelType)*8 )
  {
    // vtkErrorMacro("Failed to convert image data to MF orientation - pixel size mismatch (input: " << numberOfBitsPerPixel << " bits, output: " << sizeof(itk::Image<unsigned char, 2>::PixelType)*8 << " bits)!"); 
    return; 
  }

  itk::Image<unsigned char, 2>::Pointer inUsImage = itk::Image<unsigned char, 2>::New(); 
  itk::Image<unsigned char, 2>::SizeType size;
  size[0] = frameSizeInPx[0];
  size[1] = frameSizeInPx[1];
  itk::Image<unsigned char, 2>::IndexType start;
  start[0] = 0;
  start[1] = 0;
  itk::Image<unsigned char, 2>::RegionType region;
  region.SetSize(size);
  region.SetIndex(start);
  inUsImage->SetRegions(region);
  itk::Image<unsigned char, 2>::PixelContainer::Pointer pixelContainer = itk::Image<unsigned char, 2>::PixelContainer::New(); 
  pixelContainer->SetImportPointer(reinterpret_cast<itk::Image<unsigned char, 2>::PixelType*>(imageDataPtr), frameSizeInPx[0]*frameSizeInPx[1], false); 
  inUsImage->SetPixelContainer(pixelContainer); 

  // Now deal with the images
  if ( inUsImage.IsNull() )
  {
    // vtkErrorMacro("Failed to convert image data MF orientation - input image is null!"); 
    return; 
  }

  outUsOrientedImage->SetOrigin(inUsImage->GetOrigin());
  outUsOrientedImage->SetSpacing(inUsImage->GetSpacing());
  outUsOrientedImage->SetDirection(inUsImage->GetDirection());
  outUsOrientedImage->SetLargestPossibleRegion(inUsImage->GetLargestPossibleRegion());
  outUsOrientedImage->SetRequestedRegion(inUsImage->GetRequestedRegion());
  outUsOrientedImage->SetBufferedRegion(inUsImage->GetBufferedRegion());

  try 
  {
    outUsOrientedImage->Allocate(); 
  }
  catch(itk::ExceptionObject & err)
  {
    // vtkErrorMacro("Failed to allocate memory for the image conversion: " << err.GetDescription() ); 
    return; 
  }

  FlipInfoType flipInfo;
  GetFlipAxes(inUsImageOrientation, inUsImageType, outUsImageOrientation, flipInfo);  // TODO: Removed error handling

  if (!flipInfo.hFlip && !flipInfo.vFlip)
  {
    // We need to copy the raw data since we're using the image array as an OutputImageType::PixelContainer
    long bufferSize = inUsImage->GetLargestPossibleRegion().GetSize()[0]*inUsImage->GetLargestPossibleRegion().GetSize()[1]*sizeof(itk::Image<unsigned char, 2>::PixelType); 
    memcpy(outUsOrientedImage->GetBufferPointer(), inUsImage->GetBufferPointer(), bufferSize); 
    return; 
  }

  // Performance profiling showed that ITK's flip image filter (itk::FlipImageFilter ) is very slow,
  // therefore, an alternative implementation was tried, which does not use this filter.
  // Execution time of the alternative implementation in releaes mode does not seem to be
  // much faster, so for now keep using the flip image filter.
  return FlipImage(inUsImage, flipInfo, outUsOrientedImage);
}

//----------------------------------------------------------------------------
void vtkSlicerMetafileImporterLogic
::GetFlipAxes(US_IMAGE_ORIENTATION usImageOrientation1, US_IMAGE_TYPE usImageType1, US_IMAGE_ORIENTATION usImageOrientation2, FlipInfoType& flipInfo)
{  
  flipInfo.doubleRow=false;
  flipInfo.doubleColumn=false;
  if (usImageType1==US_IMG_RF_I_LINE_Q_LINE)
  {
    if ( usImageOrientation1==US_IMG_ORIENT_FM
      || usImageOrientation1==US_IMG_ORIENT_FU
      || usImageOrientation1==US_IMG_ORIENT_NM
      || usImageOrientation1==US_IMG_ORIENT_NU )
    {
      // keep line pairs together
      flipInfo.doubleRow=true;
    }
    else
    {
      // vtkErrorMacro("RF scanlines are expected to be in image rows");
      return;
    }
  }
  else if (usImageType1==US_IMG_RF_IQ_LINE)
  {
    if ( usImageOrientation1==US_IMG_ORIENT_FM
      || usImageOrientation1==US_IMG_ORIENT_FU
      || usImageOrientation1==US_IMG_ORIENT_NM
      || usImageOrientation1==US_IMG_ORIENT_NU )
    {
      // keep IQ value pairs together
      flipInfo.doubleColumn=true;
    }
    else
    {
      // vtkErrorMacro("RF scanlines are expected to be in image rows");
      return;
    }
  }

  flipInfo.hFlip=false;
  flipInfo.vFlip=false;
  if ( usImageOrientation1 == US_IMG_ORIENT_XX ) 
  {
    // vtkErrorMacro("Failed to determine the necessary image flip - unknown input image orientation 1"); 
    return; 
  }

  if ( usImageOrientation2 == US_IMG_ORIENT_XX ) 
  {
    // vtkErrorMacro("Failed to determine the necessary image flip - unknown input image orientation 2"); 
    return; 
  }
  if (usImageOrientation1==usImageOrientation2)
  {
    // no flip
    return;
  }
  if ((usImageOrientation1==US_IMG_ORIENT_UF && usImageOrientation2==US_IMG_ORIENT_MF)||
    (usImageOrientation1==US_IMG_ORIENT_MF && usImageOrientation2==US_IMG_ORIENT_UF)||
    (usImageOrientation1==US_IMG_ORIENT_UN && usImageOrientation2==US_IMG_ORIENT_MN)||
    (usImageOrientation1==US_IMG_ORIENT_MN && usImageOrientation2==US_IMG_ORIENT_UN)||
    (usImageOrientation1==US_IMG_ORIENT_FU && usImageOrientation2==US_IMG_ORIENT_NU)||
    (usImageOrientation1==US_IMG_ORIENT_NU && usImageOrientation2==US_IMG_ORIENT_FU)||
    (usImageOrientation1==US_IMG_ORIENT_FM && usImageOrientation2==US_IMG_ORIENT_NM)||
    (usImageOrientation1==US_IMG_ORIENT_NM && usImageOrientation2==US_IMG_ORIENT_FM))
  {
    // flip x
    flipInfo.hFlip=true;
    flipInfo.vFlip=false;
    return;
  }
  if ((usImageOrientation1==US_IMG_ORIENT_UF && usImageOrientation2==US_IMG_ORIENT_UN)||
    (usImageOrientation1==US_IMG_ORIENT_MF && usImageOrientation2==US_IMG_ORIENT_MN)||
    (usImageOrientation1==US_IMG_ORIENT_UN && usImageOrientation2==US_IMG_ORIENT_UF)||
    (usImageOrientation1==US_IMG_ORIENT_MN && usImageOrientation2==US_IMG_ORIENT_MF)||
    (usImageOrientation1==US_IMG_ORIENT_FU && usImageOrientation2==US_IMG_ORIENT_FM)||
    (usImageOrientation1==US_IMG_ORIENT_NU && usImageOrientation2==US_IMG_ORIENT_NM)||
    (usImageOrientation1==US_IMG_ORIENT_FM && usImageOrientation2==US_IMG_ORIENT_FU)||
    (usImageOrientation1==US_IMG_ORIENT_NM && usImageOrientation2==US_IMG_ORIENT_NU))
  {
    // flip y
    flipInfo.hFlip=false;
    flipInfo.vFlip=true;
    return;
  }
  if ((usImageOrientation1==US_IMG_ORIENT_UF && usImageOrientation2==US_IMG_ORIENT_MN)||
    (usImageOrientation1==US_IMG_ORIENT_MF && usImageOrientation2==US_IMG_ORIENT_UN)||
    (usImageOrientation1==US_IMG_ORIENT_UN && usImageOrientation2==US_IMG_ORIENT_MF)||
    (usImageOrientation1==US_IMG_ORIENT_MN && usImageOrientation2==US_IMG_ORIENT_UF)||
    (usImageOrientation1==US_IMG_ORIENT_FU && usImageOrientation2==US_IMG_ORIENT_NM)||
    (usImageOrientation1==US_IMG_ORIENT_NU && usImageOrientation2==US_IMG_ORIENT_FM)||
    (usImageOrientation1==US_IMG_ORIENT_FM && usImageOrientation2==US_IMG_ORIENT_NU)||
    (usImageOrientation1==US_IMG_ORIENT_NM && usImageOrientation2==US_IMG_ORIENT_FU))
  {
    // flip xy
    flipInfo.hFlip=true;
    flipInfo.vFlip=true;
    return;
  }
  // vtkErrorMacro("Image orientation conversion between orientations "<<GetStringFromUsImageOrientation(usImageOrientation1)
  //  <<" and "<<GetStringFromUsImageOrientation(usImageOrientation2)
  //  <<" is not supported (image transpose is not allowed, only reordering of rows and/or columns");
  return;
}

//----------------------------------------------------------------------------
void vtkSlicerMetafileImporterLogic
::FlipImageGeneric(void* inBuff, int width, int height, const FlipInfoType& flipInfo, void* outBuff)
{
  if (flipInfo.doubleRow)
  {
    if (height%2 != 0)
    {
      // vtkErrorMacro("Cannot flip image with pairs of rows kept together, as number of rows is odd ("<<height<<")");
      return;
    }
    width*=2;
    height/=2;
  }
  if (flipInfo.doubleColumn)
  {
    if (width%2 != 0)
    {
      // vtkErrorMacro("Cannot flip image with pairs of columns kept together, as number of columns is odd ("<<width<<")");
      return;
    }
  }

  if (!flipInfo.hFlip && flipInfo.vFlip)
  {
    // flip Y    
    itk::Image<unsigned char, 2>::PixelType* inputPixel=(itk::Image<unsigned char, 2>::PixelType*)inBuff;
    // Set the target position pointer to the first pixel of the last row
    itk::Image<unsigned char, 2>::PixelType* outputPixel=((itk::Image<unsigned char, 2>::PixelType*)outBuff)+width*(height-1);
    // Copy the image row-by-row, reversing the row order
    for (int y=height; y>0; y--)
    {
      memcpy(outputPixel, inputPixel, width*sizeof(itk::Image<unsigned char, 2>::PixelType));
      inputPixel+=width;
      outputPixel-=width;
    }
  }
  else if (flipInfo.hFlip && !flipInfo.vFlip)
  {
    // flip X    
    if (flipInfo.doubleColumn)
    {
      itk::Image<unsigned char, 2>::PixelType* inputPixel=(itk::Image<unsigned char, 2>::PixelType*)inBuff;
      // Set the target position pointer to the last pixel of the first row
      itk::Image<unsigned char, 2>::PixelType* outputPixel=((itk::Image<unsigned char, 2>::PixelType*)outBuff)+width-2;
      // Copy the image row-by-row, reversing the pixel order in each row
      for (int y=height; y>0; y--)
      {
        for (int x=width/2; x>0; x--)
        {
          *(outputPixel-1)=*inputPixel;
          *outputPixel=*(inputPixel+1);
          inputPixel+=2;
          outputPixel-=2;
        }
        outputPixel+=2*width;
      }
    }
    else
    {
      itk::Image<unsigned char, 2>::PixelType* inputPixel=(itk::Image<unsigned char, 2>::PixelType*)inBuff;
      // Set the target position pointer to the last pixel of the first row
      itk::Image<unsigned char, 2>::PixelType* outputPixel=((itk::Image<unsigned char, 2>::PixelType*)outBuff)+width-1;
      // Copy the image row-by-row, reversing the pixel order in each row
      for (int y=height; y>0; y--)
      {
        for (int x=width; x>0; x--)
        {
          *outputPixel=*inputPixel;
          inputPixel++;
          outputPixel--;
        }
        outputPixel+=2*width;
      }
    }
  }
  else if (flipInfo.hFlip && flipInfo.vFlip)
  {
    // flip X and Y
    if (flipInfo.doubleColumn)
    {
      itk::Image<unsigned char, 2>::PixelType* inputPixel=(itk::Image<unsigned char, 2>::PixelType*)inBuff;
      // Set the target position pointer to the last pixel
      itk::Image<unsigned char, 2>::PixelType* outputPixel=((itk::Image<unsigned char, 2>::PixelType*)outBuff)+height*width-1;
      // Copy the image pixelpair-by-pixelpair, reversing the pixel order
      for (int p=width*height/2; p>0; p--)
      {
        *(outputPixel-1)=*inputPixel;
        *outputPixel=*(inputPixel+1);
        inputPixel+=2;
        outputPixel-=2;
      }
    }
    else
    {
      itk::Image<unsigned char, 2>::PixelType* inputPixel=(itk::Image<unsigned char, 2>::PixelType*)inBuff;
      // Set the target position pointer to the last pixel
      itk::Image<unsigned char, 2>::PixelType* outputPixel=((itk::Image<unsigned char, 2>::PixelType*)outBuff)+height*width-1;
      // Copy the image pixel-by-pixel, reversing the pixel order
      for (int p=width*height; p>0; p--)
      {
        *outputPixel=*inputPixel;
        inputPixel++;
        outputPixel--;
      }
    }
  }

  return;
}

//----------------------------------------------------------------------------
void vtkSlicerMetafileImporterLogic
::FlipImage(const itk::Image<unsigned char, 2>::Pointer inUsImage, const FlipInfoType& flipInfo, itk::Image<unsigned char, 2>::Pointer outUsOrientedImage)
{
  outUsOrientedImage->SetOrigin(inUsImage->GetOrigin());
  outUsOrientedImage->SetSpacing(inUsImage->GetSpacing());
  outUsOrientedImage->SetDirection(inUsImage->GetDirection());
  outUsOrientedImage->SetLargestPossibleRegion(inUsImage->GetLargestPossibleRegion());
  outUsOrientedImage->SetRequestedRegion(inUsImage->GetRequestedRegion());
  outUsOrientedImage->SetBufferedRegion(inUsImage->GetBufferedRegion());

  try 
  {
    outUsOrientedImage->Allocate(); 
  }
  catch(itk::ExceptionObject & err)
  {
    // vtkErrorMacro("Failed to allocate memory for the image conversion: " << err.GetDescription() ); 
    return; 
  }

  itk::Image<unsigned char, 2>::SizeType imageSize=inUsImage->GetLargestPossibleRegion().GetSize();
  int width=imageSize[0];
  int height=imageSize[1];

  return FlipImageGeneric(inUsImage->GetBufferPointer(), width, height, flipInfo, outUsOrientedImage->GetBufferPointer());
}



//----------------------------------------------------------------------------
void vtkSlicerMetafileImporterLogic::FileOpen(FILE **stream, const char* filename, const char* flags)
{
#ifdef _WIN32
  if (fopen_s(stream, filename, flags)!=0)
  {
    (*stream)=NULL;
  }
#else
  (*stream)=fopen(filename, flags);
#endif
  if ((*stream)==0)
  {
    return;
  }
  return;
}


 
