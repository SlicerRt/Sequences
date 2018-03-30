/*=auto=========================================================================

Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

See COPYRIGHT.txt
or http://www.slicer.org/copyright/copyright.txt for details.

=========================================================================auto=*/

#include <algorithm>

#include "vtkMRMLVolumeSequenceStorageNode.h"

#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLSequenceNode.h"
#include "vtkMRMLVectorVolumeNode.h"

#include "vtkSlicerVersionConfigure.h"
#if Slicer_VERSION_MAJOR > 4 || (Slicer_VERSION_MAJOR == 4 && Slicer_VERSION_MINOR >= 9)
  #include "vtkTeemNRRDReader.h"
  #include "vtkTeemNRRDWriter.h"
#else
  #include "vtkNRRDReader.h"
  #include "vtkNRRDWriter.h"
#endif

#include "vtkObjectFactory.h"
#include "vtkImageAppendComponents.h"
#include "vtkImageData.h"
#include "vtkImageExtractComponents.h"
#include "vtkNew.h"
#include "vtkStringArray.h"

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLVolumeSequenceStorageNode);

//----------------------------------------------------------------------------
vtkMRMLVolumeSequenceStorageNode::vtkMRMLVolumeSequenceStorageNode()
{
}

//----------------------------------------------------------------------------
vtkMRMLVolumeSequenceStorageNode::~vtkMRMLVolumeSequenceStorageNode()
{
}

//----------------------------------------------------------------------------
bool vtkMRMLVolumeSequenceStorageNode::CanReadInReferenceNode(vtkMRMLNode *refNode)
{
  return refNode->IsA("vtkMRMLSequenceNode");
}

//----------------------------------------------------------------------------
int vtkMRMLVolumeSequenceStorageNode::ReadDataInternal(vtkMRMLNode* refNode)
{
  if (!this->CanReadInReferenceNode(refNode))
    {
    return 0;
    }

  vtkMRMLSequenceNode* volSequenceNode = dynamic_cast<vtkMRMLSequenceNode*>(refNode);
  if (!volSequenceNode)
    {
    vtkErrorMacro("ReadDataInternal: not a Sequence node.");
    return 0;
    }

  std::string fullName = this->GetFullNameFromFileName();
  if (fullName == std::string(""))
    {
    vtkErrorMacro("ReadData: File name not specified");
    return 0;
    }

#if Slicer_VERSION_MAJOR > 4 || (Slicer_VERSION_MAJOR == 4 && Slicer_VERSION_MINOR >= 9)
  vtkNew<vtkTeemNRRDReader> reader;
#else
  vtkNew<vtkNRRDReader> reader;
#endif
  reader->SetFileName(fullName.c_str());

  // Check if this is a NRRD file that we can read
  if (!reader->CanReadFile(fullName.c_str()))
    {
    vtkDebugMacro("vtkMRMLVolumeSequenceStorageNode: This is not a nrrd file");
    return 0;
    }

  // Set up reader
  if (this->CenterImage)
    {
    reader->SetUseNativeOriginOff();
    }
  else
    {
    reader->SetUseNativeOriginOn();
    }

  // Read the header to see if the NRRD file corresponds to the
  // MRML Node
  reader->UpdateInformation();

  // Read the volume
  reader->Update();

  // Read index information and custom attributes
  std::vector< std::string > indexValues;
  typedef std::vector<std::string> KeyVector;
  KeyVector keys = reader->GetHeaderKeysVector();
  for ( KeyVector::iterator kit = keys.begin(); kit != keys.end(); ++kit)
  {
    if (*kit == "axis 0 index type")
    {
      volSequenceNode->SetIndexTypeFromString(reader->GetHeaderValue((*kit).c_str()));
    }
    else if (*kit == "axis 0 index values")
    {
      std::string indexValue;
      for (std::istringstream indexValueList(reader->GetHeaderValue((*kit).c_str()));
        indexValueList >> indexValue;)
      {
        // Encode string to make sure there are no spaces in the serialized index value (space is used as separator)
        indexValues.push_back(vtkMRMLNode::URLDecodeString(indexValue.c_str()));
      }
    }
    else
    {
      volSequenceNode->SetAttribute((*kit).c_str(), reader->GetHeaderValue((*kit).c_str()));
    }
  }

  const char* sequenceAxisLabel = reader->GetAxisLabel(0);
  volSequenceNode->SetIndexName(sequenceAxisLabel ? sequenceAxisLabel : "frame");
  const char* sequenceAxisUnit = reader->GetAxisUnit(0);
  volSequenceNode->SetIndexUnit(sequenceAxisUnit ? sequenceAxisUnit : "");

  // Copy image data to sequence of volume nodes
  vtkImageData* imageData = reader->GetOutput();
  if (imageData == NULL || imageData->GetPointData()==NULL || imageData->GetPointData()->GetScalars() == NULL)
  {
    vtkErrorMacro("vtkMRMLVolumeSequenceStorageNode::ReadDataInternal: invalid image data");
    return 0;
  }
  int numberOfFrames = imageData->GetNumberOfScalarComponents();
  vtkNew<vtkImageExtractComponents> extractComponents;
  extractComponents->SetInputConnection(reader->GetOutputPort());
  for (int frameIndex = 0; frameIndex<numberOfFrames; frameIndex++)
  {
    extractComponents->SetComponents(frameIndex);
    extractComponents->Update();
    vtkNew<vtkImageData> frameVoxels;
    frameVoxels->DeepCopy(extractComponents->GetOutput());
    // Slicer expects normalized image position and spacing
    frameVoxels->SetSpacing( 1, 1, 1 );
    frameVoxels->SetOrigin( 0, 0, 0 );
    vtkNew<vtkMRMLScalarVolumeNode> frameVolume;
    frameVolume->SetAndObserveImageData(frameVoxels.GetPointer());
    frameVolume->SetRASToIJKMatrix(reader->GetRasToIjkMatrix());

    std::ostringstream indexStr;
    if (static_cast<int>(indexValues.size()) > frameIndex)
    {
      indexStr << indexValues[frameIndex] << std::ends;
    }
    else
    {
      indexStr << frameIndex << std::ends;
    }

    std::ostringstream nameStr;
    nameStr << refNode->GetName() << "_" << std::setw(4) << std::setfill('0') << frameIndex << std::ends;
    frameVolume->SetName( nameStr.str().c_str() );
    volSequenceNode->SetDataNodeAtValue(frameVolume.GetPointer(), indexStr.str().c_str() );
  }

  // success
  return 1;
}

//----------------------------------------------------------------------------
bool vtkMRMLVolumeSequenceStorageNode::CanWriteFromReferenceNode(vtkMRMLNode *refNode)
{
  vtkMRMLSequenceNode* volSequenceNode = vtkMRMLSequenceNode::SafeDownCast(refNode);
  if (volSequenceNode == NULL)
    {
    vtkDebugMacro("vtkMRMLVolumeSequenceStorageNode::CanWriteFromReferenceNode: invalid volSequenceNode");
    return false;
    }
  if (volSequenceNode->GetNumberOfDataNodes() == 0)
    {
    vtkDebugMacro("vtkMRMLVolumeSequenceStorageNode::CanWriteFromReferenceNode: no data nodes");
    return false;
    }

  vtkMRMLVolumeNode* firstFrameVolume = vtkMRMLVolumeNode::SafeDownCast(volSequenceNode->GetNthDataNode(0));
  if (firstFrameVolume == NULL)
    {
    vtkDebugMacro("vtkMRMLVolumeSequenceStorageNode::CanWriteFromReferenceNode: only volume nodes can be written");
    return false;
    }

  int firstFrameVolumeExtent[6] = { 0, -1, 0, -1, 0, -1 };
  int firstFrameVolumeScalarType = VTK_VOID;
  int firstFrameVolumeNumberOfComponents = 0;
  if (firstFrameVolume->GetImageData())
    {
    firstFrameVolume->GetImageData()->GetExtent(firstFrameVolumeExtent);
    firstFrameVolumeScalarType = firstFrameVolume->GetImageData()->GetScalarType();
    firstFrameVolumeNumberOfComponents = firstFrameVolume->GetImageData()->GetNumberOfScalarComponents();
    if (firstFrameVolumeNumberOfComponents != 1)
      {
      vtkDebugMacro("vtkMRMLVolumeSequenceStorageNode::CanWriteFromReferenceNode: only single scalar component volumes can be written by VTK NRRD writer");
      return false;
      }
    }

  int numberOfFrameVolumes = volSequenceNode->GetNumberOfDataNodes();
  for (int frameIndex = 1; frameIndex<numberOfFrameVolumes; frameIndex++)
    {
    vtkMRMLVolumeNode* currentFrameVolume = vtkMRMLVolumeNode::SafeDownCast(volSequenceNode->GetNthDataNode(frameIndex));
    if (currentFrameVolume == NULL)
      {
      vtkDebugMacro("vtkMRMLVolumeSequenceStorageNode::CanWriteFromReferenceNode: only volume nodes can be written (frame "<<frameIndex<<")");
      return false;
      }
    vtkNew<vtkMatrix4x4> currentVolumeIjkToRas;
    currentFrameVolume->GetIJKToRASMatrix(currentVolumeIjkToRas.GetPointer());

    int currentFrameVolumeExtent[6] = { 0, -1, 0, -1, 0, -1 };
    int currentFrameVolumeScalarType = VTK_VOID;
    int currentFrameVolumeNumberOfComponents = 0;
    if (currentFrameVolume->GetImageData())
      {
      currentFrameVolume->GetImageData()->GetExtent(currentFrameVolumeExtent);
      currentFrameVolumeScalarType = currentFrameVolume->GetImageData()->GetScalarType();
      currentFrameVolumeNumberOfComponents = currentFrameVolume->GetImageData()->GetNumberOfScalarComponents();
      }
    for (int i = 0; i < 6; i++)
      {
      if (firstFrameVolumeExtent[i] != currentFrameVolumeExtent[i])
        {
        vtkDebugMacro("vtkMRMLVolumeSequenceStorageNode::CanWriteFromReferenceNode: extent mismatch (frame " << frameIndex << ")");
        return false;
        }
      }
    if (currentFrameVolumeScalarType != firstFrameVolumeScalarType)
      {
      vtkDebugMacro("vtkMRMLVolumeSequenceStorageNode::CanWriteFromReferenceNode: scalar type mismatch (frame " << frameIndex << ")");
      return false;
      }
    if (currentFrameVolumeNumberOfComponents != firstFrameVolumeNumberOfComponents)
      {
      vtkDebugMacro("vtkMRMLVolumeSequenceStorageNode::CanWriteFromReferenceNode: number of components mismatch (frame " << frameIndex << ")");
      return false;
      }
    }

  return true;
}

//----------------------------------------------------------------------------
int vtkMRMLVolumeSequenceStorageNode::WriteDataInternal(vtkMRMLNode *refNode)
{
  vtkMRMLSequenceNode* volSequenceNode = vtkMRMLSequenceNode::SafeDownCast(refNode);
  if (volSequenceNode==NULL)
    {
    vtkErrorMacro(<< "vtkMRMLVolumeSequenceStorageNode::WriteDataInternal: Do not recognize node type " << refNode->GetClassName());
    return 0;
    }

  vtkNew<vtkMatrix4x4> ijkToRas;
  int frameVolumeDimensions[3] = {0};
  int frameVolumeScalarType = VTK_VOID;
  if (volSequenceNode->GetNumberOfDataNodes()>0)
  {
    vtkMRMLVolumeNode* frameVolume = vtkMRMLVolumeNode::SafeDownCast(volSequenceNode->GetNthDataNode(0));
    if (frameVolume==NULL)
    {
      vtkErrorMacro(<< "vtkMRMLVolumeSequenceStorageNode::WriteDataInternal: Data node is not a volume");
      return 0;
    }
    frameVolume->GetIJKToRASMatrix(ijkToRas.GetPointer());
    if (frameVolume->GetImageData())
    {
      frameVolume->GetImageData()->GetDimensions(frameVolumeDimensions);
      frameVolumeScalarType = frameVolume->GetImageData()->GetScalarType();
    }
  }

  vtkNew<vtkImageAppendComponents> appender;

  int numberOfFrameVolumes = volSequenceNode->GetNumberOfDataNodes();
  for (int frameIndex=0; frameIndex<numberOfFrameVolumes; frameIndex++)
  {
    vtkMRMLVolumeNode* frameVolume = vtkMRMLVolumeNode::SafeDownCast(volSequenceNode->GetNthDataNode(frameIndex));
    if (frameVolume==NULL)
    {
      vtkErrorMacro(<< "vtkMRMLVolumeSequenceStorageNode::WriteDataInternal: Data node "<<frameIndex<<" is not a volume");
      return 0;
    }
    //TODO: check if frameVolume->GetIJKToRASMatrix() is the same as ijkToRas
    // either save the IJK to RAS into each frame or resample or return with error
    int currentFrameVolumeDimensions[3] = {0};
    int currentFrameVolumeScalarType = VTK_VOID;
    if (frameVolume->GetImageData())
    {
      frameVolume->GetImageData()->GetDimensions(currentFrameVolumeDimensions);
      currentFrameVolumeScalarType = frameVolume->GetImageData()->GetScalarType();
    }
    if (currentFrameVolumeDimensions[0] != frameVolumeDimensions[0]
    || currentFrameVolumeDimensions[0] != frameVolumeDimensions[0]
    || currentFrameVolumeDimensions[0] != frameVolumeDimensions[0]
    || currentFrameVolumeScalarType != frameVolumeScalarType)
    {
      vtkErrorMacro(<< "vtkMRMLVolumeSequenceStorageNode::WriteDataInternal: Data node "<<frameIndex<<" size or scalar type mismatch ("
        <<"got "<<currentFrameVolumeDimensions[0]<<"x"<<currentFrameVolumeDimensions[1]<<"x"<<currentFrameVolumeDimensions[2]<<" "<<vtkImageScalarTypeNameMacro(currentFrameVolumeScalarType)<<", "
        <<"expected "<<frameVolumeDimensions[0]<<"x"<<frameVolumeDimensions[1]<<"x"<<frameVolumeDimensions[2]<<" "<<vtkImageScalarTypeNameMacro(frameVolumeScalarType) );
      return 0;
    }
    if (frameVolume->GetImageData())
    {
      appender->AddInputData(frameVolume->GetImageData());
    }
  }

  std::string fullName = this->GetFullNameFromFileName();
  if (fullName == std::string(""))
    {
    vtkErrorMacro("WriteData: File name not specified");
    return 0;
    }
  // Use here the NRRD Writer
#if Slicer_VERSION_MAJOR > 4 || (Slicer_VERSION_MAJOR == 4 && Slicer_VERSION_MINOR >= 9)
  vtkNew<vtkTeemNRRDWriter> writer;
  writer->SetVectorAxisKind(nrrdKindList);
#else
  vtkNew<vtkNRRDWriter> writer;
#endif
  writer->SetFileName(fullName.c_str());
  appender->Update();
  writer->SetInputConnection(appender->GetOutputPort());
  writer->SetUseCompression(this->GetUseCompression());

  // set volume attributes
  writer->SetIJKToRASMatrix(ijkToRas.GetPointer());
  //writer->SetMeasurementFrameMatrix(mf.GetPointer());

  // Write index information
  if (!volSequenceNode->GetIndexName().empty())
  {
    writer->SetAxisLabel(0, volSequenceNode->GetIndexName().c_str());
  }
  if (!volSequenceNode->GetIndexUnit().empty())
  {
    writer->SetAxisUnit(0, volSequenceNode->GetIndexUnit().c_str());
  }
  if (!volSequenceNode->GetIndexTypeAsString().empty())
  {
    writer->SetAttribute("axis 0 index type", volSequenceNode->GetIndexTypeAsString());
  }
  if (numberOfFrameVolumes > 0)
  {
    std::stringstream ssIndexValues;
    for (int frameIndex = 0; frameIndex < numberOfFrameVolumes; frameIndex++)
    {
      if (frameIndex > 0)
      {
        ssIndexValues << " ";
      }
      // Encode string to make sure there are no spaces in the serialized index value (space is used as separator)
      ssIndexValues << vtkMRMLNode::URLEncodeString(volSequenceNode->GetNthIndexValue(frameIndex).c_str());
    }
    writer->SetAttribute("axis 0 index values", ssIndexValues.str());
  }

  // pass down all MRML attributes to NRRD
  std::vector<std::string> attributeNames = volSequenceNode->GetAttributeNames();
  std::vector<std::string>::iterator ait = attributeNames.begin();
  for (; ait != attributeNames.end(); ++ait)
    {
    writer->SetAttribute((*ait), volSequenceNode->GetAttribute((*ait).c_str()));
    }

  writer->Write();
  int writeFlag = 1;
  if (writer->GetWriteError())
    {
    vtkErrorMacro("ERROR writing NRRD file " << (writer->GetFileName() == NULL ? "null" : writer->GetFileName()));
    writeFlag = 0;
    }

  this->StageWriteData(refNode);

  return writeFlag;
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeSequenceStorageNode::InitializeSupportedReadFileTypes()
{
  this->SupportedReadFileTypes->InsertNextValue("Volume sequence (.seq.nrrd)");
  this->SupportedReadFileTypes->InsertNextValue("Volume sequence (.seq.nhdr)");
  this->SupportedReadFileTypes->InsertNextValue("Volume sequence (.nrrd)");
  this->SupportedReadFileTypes->InsertNextValue("Volume sequence (.nhdr)");
}

//----------------------------------------------------------------------------
void vtkMRMLVolumeSequenceStorageNode::InitializeSupportedWriteFileTypes()
{
  this->SupportedWriteFileTypes->InsertNextValue("Volume sequence (.seq.nrrd)");
  this->SupportedWriteFileTypes->InsertNextValue("Volume sequence (.seq.nhdr)");
  this->SupportedWriteFileTypes->InsertNextValue("Volume sequence (.nrrd)");
  this->SupportedWriteFileTypes->InsertNextValue("Volume sequence (.nhdr)");
}

//----------------------------------------------------------------------------
const char* vtkMRMLVolumeSequenceStorageNode::GetDefaultWriteFileExtension()
{
  return "seq.nrrd";
}
