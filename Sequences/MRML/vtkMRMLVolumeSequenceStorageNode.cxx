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

#include "vtkNRRDReader.h"
#include "vtkNRRDWriter.h"

#include "vtkObjectFactory.h"
#include "vtkImageAppendComponents.h"
#include "vtkImageData.h"
#include "vtkImageExtractComponents.h"
#include "vtkNew.h"


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

  vtkNew<vtkNRRDReader> reader;
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
  
  // prepare volume node
  vtkNew<vtkMRMLVectorVolumeNode> vectorVolNode;

  // Read the volume
  reader->Update();

  // Add key-value pairs to sequence node
  typedef std::vector<std::string> KeyVector;
  KeyVector keys = reader->GetHeaderKeysVector();
  for ( KeyVector::iterator kit = keys.begin(); kit != keys.end(); ++kit)
  {
    volSequenceNode->SetAttribute((*kit).c_str(), reader->GetHeaderValue((*kit).c_str()));
  }
  volSequenceNode->SetIndexName("frame");
  volSequenceNode->SetIndexUnit("");

  // Copy image data to sequence of volume nodes
  vtkImageData* imageData = reader->GetOutput();
  if (imageData == NULL)
  {
    vtkErrorMacro("vtkMRMLVolumeSequenceStorageNode::ReadDataInternal: invalid image data");
    return 0;
  }
  int numberOfFrames = imageData->GetNumberOfScalarComponents();
  vtkNew<vtkImageExtractComponents> extractComponents;
#if (VTK_MAJOR_VERSION <= 5)
  extractComponents->SetInput (reader->GetOutput());
#else
  extractComponents->SetInputConnection(reader->GetOutputPort());
#endif  
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
    indexStr << frameIndex << std::ends; 
    
    std::ostringstream nameStr;
    nameStr << refNode->GetName() << std::setw(4) << std::setfill('0') << frameIndex << std::ends; 
    frameVolume->SetName( nameStr.str().c_str() );
    volSequenceNode->SetDataNodeAtValue(frameVolume.GetPointer(), indexStr.str().c_str() );
  }

  // success
  return 1;
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
  vtkNew<vtkNRRDWriter> writer;
  writer->SetFileName(fullName.c_str());
  appender->Update();
#if (VTK_MAJOR_VERSION <= 5)
  writer->SetInput(appender->GetImageData() );
#else
  writer->SetInputConnection(appender->GetOutputPort());
#endif
  writer->SetUseCompression(this->GetUseCompression());

  // set volume attributes
  writer->SetIJKToRASMatrix(ijkToRas.GetPointer());
  //writer->SetMeasurementFrameMatrix(mf.GetPointer());

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
const char* vtkMRMLVolumeSequenceStorageNode::GetDefaultWriteFileExtension()
{
  return "nrrd";
}
