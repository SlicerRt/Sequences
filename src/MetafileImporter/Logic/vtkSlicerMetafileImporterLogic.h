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

// .NAME vtkSlicerMetafileImporterLogic - slicer logic class for volumes manipulation
// .SECTION Description
// This class manages the logic associated with reading, saving,
// and changing propertied of the volumes


#ifndef __vtkSlicerMetafileImporterLogic_h
#define __vtkSlicerMetafileImporterLogic_h

// STD includes
#include <cstdlib>

// VTK includes
#include "vtkMatrix4x4.h"

// ITK includes
#include "itkImage.h"
#include "itkImageIOBase.h"
#include "itkImageBase.h"

// Slicer includes
#include "vtkSlicerModuleLogic.h"

// MetfileImporter includes
#include "vtkSlicerMetafileImporterModuleLogicExport.h"
#include "vtkSlicerMultiDimensionLogic.h"

// Enumerations
// Directly from PLUS -----------------------------------------------------------------

enum US_IMAGE_ORIENTATION
{
  US_IMG_ORIENT_XX,  /*!< undefined */
  US_IMG_ORIENT_UF, /*!< image x axis = unmarked transducer axis, image y axis = far transducer axis */
  US_IMG_ORIENT_UN, /*!< image x axis = unmarked transducer axis, image y axis = near transducer axis */
  US_IMG_ORIENT_MF, /*!< image x axis = marked transducer axis, image y axis = far transducer axis */
  US_IMG_ORIENT_MN, /*!< image x axis = marked transducer axis, image y axis = near transducer axis */
  US_IMG_ORIENT_FU, /*!< image x axis = far transducer axis, image y axis = unmarked transducer axis (usually for RF frames)*/
  US_IMG_ORIENT_NU, /*!< image x axis = near transducer axis, image y axis = unmarked transducer axis (usually for RF frames)*/
  US_IMG_ORIENT_FM, /*!< image x axis = far transducer axis, image y axis = marked transducer axis (usually for RF frames)*/
  US_IMG_ORIENT_NM, /*!< image x axis = near transducer axis, image y axis = marked transducer axis (usually for RF frames)*/
  US_IMG_ORIENT_LAST   /*!< just a placeholder for range checking, this must be the last defined orientation item */
};

enum US_IMAGE_TYPE
{
  US_IMG_TYPE_XX,    /*!< undefined */
  US_IMG_BRIGHTNESS, /*!< B-mode image */
  US_IMG_RF_REAL,    /*!< RF-mode image, signal is stored as a series of real values */
  US_IMG_RF_IQ_LINE, /*!< RF-mode image, signal is stored as a series of I and Q samples in a line (I1, Q1, I2, Q2, ...) */
  US_IMG_RF_I_LINE_Q_LINE, /*!< RF-mode image, signal is stored as a series of I samples in a line, then Q samples in the next line (I1, I2, ..., Q1, Q2, ...) */
  US_IMG_TYPE_LAST   /*!< just a placeholder for range checking, this must be the last defined image type */
};

typedef itk::ImageIOBase::IOComponentType ITKScalarPixelType;

// END ENUMERATIONS ------------------------------------------------------------------------------

typedef itk::ImageBase< 2 >                    ImageBaseType;
typedef ImageBaseType::Pointer                 ImageBasePointer;
typedef ImageBaseType::ConstPointer            ImageBaseConstPointer;  
struct FlipInfoType
{
  FlipInfoType() : hFlip(false), vFlip(false), doubleColumn(false), doubleRow(false) {};
  bool hFlip; // flip the image horizontally (pixel columns are reordered)
  bool vFlip; // flip the image vertically (pixel rows are reordered)
  bool doubleColumn; // keep pairs of pixel columns together (for RF_IQ_LINE encoded images)
  bool doubleRow; // keep pairs of pixel rows together (for RF_I_LINE_Q_LINE encoded images)
};


/// \ingroup Slicer_QtModules_ExtensionTemplate
class VTK_SLICER_METAFILEIMPORTER_MODULE_LOGIC_EXPORT vtkSlicerMetafileImporterLogic :
  public vtkSlicerModuleLogic
{
public:

  static vtkSlicerMetafileImporterLogic *New();
  vtkTypeMacro(vtkSlicerMetafileImporterLogic, vtkSlicerModuleLogic);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkSlicerMetafileImporterLogic();
  virtual ~vtkSlicerMetafileImporterLogic();

  virtual void SetMRMLSceneInternal(vtkMRMLScene* newScene);
  /// Register MRML Node classes to Scene. Gets called automatically when the MRMLScene is attached to this logic class.
  virtual void RegisterNodes();
  virtual void UpdateFromMRMLScene();
  virtual void OnMRMLSceneNodeAdded(vtkMRMLNode* node);
  virtual void OnMRMLSceneNodeRemoved(vtkMRMLNode* node);
private:

  vtkSlicerMetafileImporterLogic(const vtkSlicerMetafileImporterLogic&); // Not implemented
  void operator=(const vtkSlicerMetafileImporterLogic&);               // Not implemented

public:

  /*! Read file contents into the object */
  void Read();

  /*!
    Set input/output file name. The file contains only the image header in case of
    MHD images and the full image (including pixel data) in case of MHA images.
  */
  void SetFileName(const char* aFilename);

  /*! Logic for MultiDimension hierarchy to manipulate nodes */
  vtkSlicerMultiDimensionLogic* MultiDimensionLogic;

  /*! The root node for the multi-dimension hierarchy */
  vtkMRMLNode* rootNode;


protected:

  /*! Opens a file. Doesn't log error if it fails because it may be expected. */
  void FileOpen(FILE **stream, const char* filename, const char* flags);

  /*! Read all the fields in the metaimage file header */
  void ReadImageHeader();

  /*! Read pixel data from the metaimage */
  void ReadImagePixels();

  /*! Get full path to the file for storing the pixel data */
  std::string GetPixelDataFilePath();

  /*! Conversion between ITK and METAIO pixel types */
  void ConvertMetaElementTypeToItkPixelType(const std::string &elementTypeStr, ITKScalarPixelType &itkPixelType);

  /*! Set the image properties specified in the header */
  void SetImageProperty( const char* propertyName, const char* propertyValue );

  static void GetOrientedImage( unsigned char* imageDataPtr, US_IMAGE_ORIENTATION  inUsImageOrientation, US_IMAGE_TYPE inUsImageType, const int frameSizeInPx[2],
                     int numberOfBitsPerPixel, US_IMAGE_ORIENTATION  outUsImageOrientation, itk::Image<unsigned char, 2>::Pointer outUsOrientedImage );

  static void GetFlipAxes(US_IMAGE_ORIENTATION usImageOrientation1, US_IMAGE_TYPE usImageType1, US_IMAGE_ORIENTATION usImageOrientation2, FlipInfoType& flipInfo);

  static void FlipImage(const itk::Image<unsigned char, 2>::Pointer inUsImage, const FlipInfoType& flipInfo, itk::Image<unsigned char, 2>::Pointer outUsOrientedImage);

  static void FlipImageGeneric(void* inBuff, int width, int height, const FlipInfoType& flipInfo, void* outBuff);

private:

#ifdef _WIN32
  typedef __int64 FilePositionOffsetType;
#elif defined __APPLE__
  typedef off_t FilePositionOffsetType;
#else
  typedef off64_t FilePositionOffsetType;
#endif


  /*! Name of the file that contains the image header (*.MHA or *.MHD) */
  std::string FileName;
  /*! Name of the temporary file used to build up the header */
  std::string TempHeaderFileName;
  /*! Name of the temporary file used to build up the image data */
  std::string TempImageFileName;
  /*! Enable/disable zlib compression of pixel data */
  bool UseCompression;
  /*! Size of compressed data */
  unsigned int CompressedDataSize;
  /*! ASCII or binary */
  itk::ImageIOBase::FileType FileType;
  /*! Integer/float, short/long, signed/unsigned */
  ITKScalarPixelType PixelType;
  /*! Number of components (or channels). Only single-component images are supported. */
  int NumberOfComponents;
  /*! Number of image dimensions. Only 2 (single frame) or 3 (sequence of frames) are supported. */
  int NumberOfDimensions;
  /*! Frame size (first two elements) and number of frames (last element) */
  int Dimensions[3];
  /*! Current frame offset, this is used to build up frames one addition at a time */
  int m_CurrentFrameOffset;
  /*! Total bytes written */
  unsigned long long m_TotalBytesWritten;

  /*!
    Image orientation in memory is always MF for B-mode, but when reading/writing a file then
    any orientation can be used.
  */
  US_IMAGE_ORIENTATION ImageOrientationInFile;

  /*!
    Image orientation for reading into memory.
  */
  US_IMAGE_ORIENTATION ImageOrientationInMemory;

  /*!
    Image type (B-mode, RF, ...)
  */
  US_IMAGE_TYPE ImageType;

  /*! Position of the first pixel of the image data within the pixel data file */
  FilePositionOffsetType PixelDataFileOffset;
  /*! File name where the pixel data is stored */
  std::string PixelDataFileName;
  
};

#endif
