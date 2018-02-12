/*=auto=========================================================================
 
 Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.
 
 See COPYRIGHT.txt
 or http://www.slicer.org/copyright/copyright.txt for details.
 
 =========================================================================auto=*/
#include <algorithm>

#include "vtkMRMLBitStreamSequenceStorageNode.h"
#include "vtkMRMLSequenceNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLBitStreamNode.h"
#include <vtkCollection.h>

#include "vtkObjectFactory.h"
#include "vtkNew.h"
#include "vtkStringArray.h"

#include "vtksys/SystemTools.hxx"

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLBitStreamSequenceStorageNode);

// Add the helper functions

//-------------------------------------------------------
inline void Trim(std::string &str)
{
  str.erase(str.find_last_not_of(" \t\r\n") + 1);
  str.erase(0, str.find_first_not_of(" \t\r\n"));
}


//----------------------------------------------------------------------------
vtkMRMLBitStreamSequenceStorageNode::vtkMRMLBitStreamSequenceStorageNode()
{
}

//----------------------------------------------------------------------------
vtkMRMLBitStreamSequenceStorageNode::~vtkMRMLBitStreamSequenceStorageNode()
{
}

//----------------------------------------------------------------------------
bool vtkMRMLBitStreamSequenceStorageNode::CanReadInReferenceNode(vtkMRMLNode *refNode)
{
  return refNode->IsA("vtkMRMLSequenceNode");
}



int vtkMRMLBitStreamSequenceStorageNode::GetTagValue(char* headerString, int headerLenght, const char* tag, int tagLength, std::string &tagValueString, int&tagValueLength)
{
  int beginIndex = -1;
  int endIndex = -1;
  int index = 0;
  for(index = 0; index < headerLenght; index ++ )
  {
    if (index < headerLenght -tagLength)
    {
      std::string stringTemp(&(headerString[index]), &(headerString[index + tagLength]));
      if(strcmp(stringTemp.c_str(),tag)==0)
      {
        beginIndex = index+tagLength+2;
      }
    }
    std::string stringTemp2(&(headerString[index]), &(headerString[index + 1]));
    if(beginIndex>=0 && (strcmp(stringTemp2.c_str(), "\n") == 0))
    {
      endIndex = index;
      break;
    }
  }
  if(beginIndex>=0 &&(endIndex>beginIndex))
  {
    tagValueString = std::string(&(headerString[beginIndex]), &(headerString[endIndex]));
    return 1;
  }
  else
  {
    return 0;
  }
}

int vtkMRMLBitStreamSequenceStorageNode::ReadElementsInSingleLine(std::string& firstElement, std::string& secondElement, FILE* stream)
{
  int stringLineLength = 0;
  std::string data = std::string(" ");
  bool bCanBeRead = true;
  while(bCanBeRead)
    {
    bCanBeRead = fread(&data[0],1,1,stream);
    stringLineLength++;
    if (strcmp(data.c_str(),"\n")==0)
      {
      fseek(stream, -stringLineLength, SEEK_CUR);
      break;
      }
    }
  if (!bCanBeRead) return -1;
  char *lineString = new char[stringLineLength];
  fread(lineString,stringLineLength,1,stream);
  std::string stringOperator(lineString);
  size_t pos = 0;
  if ((pos = stringOperator.find(":")) != std::string::npos)
    {
    firstElement = std::string(lineString,pos);
    std::string stringMsgLength(lineString+pos,stringLineLength-pos);
    stringMsgLength.erase(stringMsgLength.find_last_not_of(" :\t\r\n") + 1);
    stringMsgLength.erase(0, stringMsgLength.find_first_not_of(" :\t\r\n")); // Get rid of ":"
    stringMsgLength.erase(0, stringMsgLength.find_first_not_of(" :\t\r\n")); // Get rid of " "
    secondElement = std::string(stringMsgLength);
    }
  else
    {
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkMRMLBitStreamSequenceStorageNode::ReadDataInternal(vtkMRMLNode* refNode)
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
  volSequenceNode->RemoveAllDataNodes();
  std::string fullName = this->GetFullNameFromFileName();
  if (fullName == std::string(""))
  {
    vtkErrorMacro("ReadData: File name not specified");
    return 0;
  }
  
  FILE* stream = fopen(fullName.c_str(),"rb");
  
  // Check if this is a  file that we can read
  if (stream == NULL)
  {
    vtkDebugMacro("vtkMRMLBitStreamSequenceStorageNode: This is not a text file");
    return 0;
  }
  std::string data("  ");
  int headerLength = 0;
  while(fread(&data[0],2,1,stream)){
    fseek(stream, -1, SEEK_CUR);
    headerLength ++;
    if (strcmp(data.c_str(),"\n\n")==0)
    {
      fseek(stream, -headerLength, SEEK_CUR);
      break;
    }
  }
  char * headerString = new char[headerLength];
  fread(headerString, headerLength,1,stream);
  bool fileValid = true;
  std::string tagValueString("");
  int tagValueLength;
  if(GetTagValue(headerString, headerLength, "ObjectType", 10, tagValueString, tagValueLength))
  {
    if (strcmp(tagValueString.c_str(), "BitStream")==0)
    {
      fileValid *= true;
    }
    else
    {
      fileValid = false;
    }
  }
  if(GetTagValue(headerString, headerLength, "Codec", 5, tagValueString, tagValueLength))
  {
    if (strcmp(tagValueString.c_str(), IGTL_VIDEO_CODEC_NAME_H264)==0 ||
        strcmp(tagValueString.c_str(), IGTL_VIDEO_CODEC_NAME_VP9)==0
        )
    {
      fileValid *= true;
    }
    else
    {
      fileValid = false;
    }
  }
  std::string volumeName = "";
  if(GetTagValue(headerString, headerLength, "VolumeName", 10, tagValueString, tagValueLength))
  {
    fileValid *= true;
    volumeName = tagValueString;
  }
  else
  {
    fileValid = false;
  }
  fread(headerString, 2,1,stream); // get rid of the following two line breaks
  if(fileValid)
  {
    
    if(this->GetScene())
    {
      vtkCollection* collection =  NULL;
      vtkMRMLScene* scene = this->GetScene();
      collection = scene->GetNodesByClassByName("vtkMRMLBitStreamNode",volumeName.c_str());
      int nCol = collection->GetNumberOfItems();
      if (nCol > 0)
      {
        for (int i = 0; i < nCol; i ++)
        {
          this->GetScene()->RemoveNode(vtkMRMLNode::SafeDownCast(collection->GetItemAsObject(i)));
        }
      }
      vtkMRMLBitStreamNode * frameProxyNode = vtkMRMLBitStreamNode::New();
      this->GetScene()->AddNode(frameProxyNode);
      frameProxyNode->SetUpVideoDeviceByName(volumeName.c_str());
      while(1)
      {
        std::string timeStamp("");
        std::string stringMessageLength("");
        int errorCode = this->ReadElementsInSingleLine(timeStamp, stringMessageLength, stream);
        if (errorCode == -1)
          {
          break;
          }
        std::string FrameType("");
        std::string isKeyFrame("");
        errorCode = this->ReadElementsInSingleLine(FrameType, isKeyFrame, stream);
        igtl::VideoMessage::Pointer keyFrameBuffer = igtl::VideoMessage::New();
        if (errorCode == -1)
          {
          break;
          }
        else if (errorCode == 1 && atoi(stringMessageLength.c_str()) > 0)
          {
          igtl::MessageHeader::Pointer headerMsg = igtl::MessageHeader::New();
          headerMsg->InitPack();
          fread(headerMsg->GetPackPointer(),IGTL_HEADER_SIZE, 1, stream);
          headerMsg->Unpack();
          igtl::VideoMessage::Pointer buffer = igtl::VideoMessage::New();
          buffer->SetMessageHeader(headerMsg);
          buffer->AllocatePack();
          fread(buffer->GetPackBodyPointer(), atoi(stringMessageLength.c_str())-IGTL_HEADER_SIZE, 1, stream);
          igtl_header* h = (igtl_header*) buffer->GetPackPointer();
          igtl_header_convert_byte_order(h);
          frameProxyNode->SetIsCopied(false);
          if(strncmp(FrameType.c_str(), "PrecedingKeyFrame", 17) == 0)
            {
            frameProxyNode->SetKeyFrameStream(buffer);
            }
          else if(strncmp(FrameType.c_str(), "IsKeyFrame", 10) == 0)
            {
            if(strncmp(isKeyFrame.c_str(), "1", 10) == 0)
              {
              frameProxyNode->SetKeyFrameStream(buffer);
              }
            frameProxyNode->SetMessageStream(buffer);
            volSequenceNode->SetDataNodeAtValue(frameProxyNode, std::string(timeStamp));
            }
            fread(&data[0],1,1,stream); // get rid of last line break
          }
      }
    }
  }
  
  return 1;
}

//----------------------------------------------------------------------------
bool vtkMRMLBitStreamSequenceStorageNode::CanWriteFromReferenceNode(vtkMRMLNode *refNode)
{
  vtkMRMLSequenceNode* sequenceNode = vtkMRMLSequenceNode::SafeDownCast(refNode);
  if (sequenceNode == NULL)
  {
    vtkDebugMacro("vtkMRMLBitStreamSequenceStorageNode::CanWriteFromReferenceNode: input is not a sequence node");
    return false;
  }
  if (sequenceNode->GetNumberOfDataNodes() == 0)
  {
    vtkDebugMacro("vtkMRMLBitStreamSequenceStorageNode::CanWriteFromReferenceNode: no data nodes");
    return false;
  }
  int numberOfFrameVolumes = sequenceNode->GetNumberOfDataNodes();
  for (int frameIndex = 0; frameIndex < numberOfFrameVolumes; frameIndex++)
  {
    vtkMRMLBitStreamNode* bitstream = vtkMRMLBitStreamNode::SafeDownCast(sequenceNode->GetNthDataNode(frameIndex));
    if (bitstream == NULL)
    {
      vtkDebugMacro("vtkMRMLBitStreamSequenceStorageNode::CanWriteFromReferenceNode: stream nodes has not bit stream (frame " << frameIndex << ")");
      return false;
    }
  }
  return true;
}

//----------------------------------------------------------------------------
int vtkMRMLBitStreamSequenceStorageNode::WriteDataInternal(vtkMRMLNode *refNode)
{
  vtkMRMLSequenceNode* bitStreamSequenceNode = vtkMRMLSequenceNode::SafeDownCast(refNode);
  if (bitStreamSequenceNode==NULL)
  {
    vtkErrorMacro(<< "vtkMRMLBitStreamSequenceStorageNode::WriteDataInternal: Do not recognize node type " << refNode->GetClassName());
    return 0;
  }
  char* volumeName = (char*)"";
  std::string codecName = "";
  if (bitStreamSequenceNode->GetNumberOfDataNodes()>0)
  {
    vtkMRMLBitStreamNode* frameBitStream = vtkMRMLBitStreamNode::SafeDownCast(bitStreamSequenceNode->GetNthDataNode(0));
    if (frameBitStream==NULL)
    {
      vtkErrorMacro(<< "vtkMRMLBitStreamSequenceStorageNode::WriteDataInternal: Data node is not a bit stream");
      return 0;
    }
    volumeName = frameBitStream->GetName();
    codecName = std::string(frameBitStream->GetCodecName().c_str(),IGTL_VIDEO_CODEC_NAME_SIZE);
  }
  std::string fullName = this->GetFullNameFromFileName();
  if (fullName == std::string(""))
  {
    vtkErrorMacro("WriteData: File name not specified");
    return 0;
  }
  // If header file exists then append transform info before element data file line
  // Append the transform information to the end of the file
  std::stringstream defaultHeaderOutStream;
  defaultHeaderOutStream
  << "ObjectType: BitStream" << std::endl
  << "Codec: " << codecName.c_str() <<std::endl
  << "VolumeName: " << volumeName << std::endl;
  // Append the bit stream to the end of the file
  std::ofstream outStream(fullName.c_str(), std::ios_base::binary);
  outStream << defaultHeaderOutStream.str();
  outStream << std::setfill('0');
  outStream << std::endl;
  outStream << std::endl;
  int numberOfFrameBitStreams = bitStreamSequenceNode->GetNumberOfDataNodes();
  vtkMRMLBitStreamNode* frameBitStream = vtkMRMLBitStreamNode::SafeDownCast(bitStreamSequenceNode->GetNthDataNode(0));
  std::string timeStamp = bitStreamSequenceNode->GetNthIndexValue(0);
  if (frameBitStream!=NULL && frameBitStream->GetMessageValid()>0 && timeStamp.size())
    {
    igtl::VideoMessage::Pointer keyFrameMsg = frameBitStream->GetKeyFrameStream();
    igtl::VideoMessage::Pointer frameMsg = frameBitStream->GetMessageStreamBuffer();
    keyFrameMsg->Unpack();
    frameMsg->Unpack();
    if (frameMsg->GetFrameType() == keyFrameMsg->GetFrameType())
      {
      int messageLength = keyFrameMsg->GetPackSize();
      outStream.write(timeStamp.c_str(), timeStamp.size());
      outStream <<": "<<messageLength << std::endl;
      outStream<< "IsKeyFrame: " << 1 << std::endl;
      outStream.write((char*)frameBitStream->GetKeyFrameStream()->GetPackPointer(), messageLength);
      outStream << std::endl;
      }
    else
      {
      int messageLength = keyFrameMsg->GetPackSize();
      outStream.write(timeStamp.c_str(), timeStamp.size());
      outStream <<": "<<messageLength << std::endl;;
      outStream<< "PrecedingKeyFrame: " << 1 << std::endl;
      outStream.write((char*)frameBitStream->GetKeyFrameStream()->GetPackPointer(), messageLength);
      outStream << std::endl;
      messageLength = frameMsg->GetPackSize();
      outStream.write(timeStamp.c_str(), timeStamp.size());
      outStream <<": "<<messageLength << std::endl;
      outStream<< "IsKeyFrame: " << 0 << std::endl;
      outStream.write((char*)frameBitStream->GetMessageStreamBuffer()->GetPackPointer(), messageLength);
      outStream << std::endl;
      }
    }
  for (int frameIndex=1; frameIndex<numberOfFrameBitStreams; frameIndex++)
  {
    vtkMRMLBitStreamNode* frameBitStream = vtkMRMLBitStreamNode::SafeDownCast(bitStreamSequenceNode->GetNthDataNode(frameIndex));
    std::string timeStamp = bitStreamSequenceNode->GetNthIndexValue(frameIndex);
    if (frameBitStream!=NULL && frameBitStream->GetMessageValid()>0 && timeStamp.size())
    {
      igtl::VideoMessage::Pointer frameMsg = frameBitStream->GetMessageStreamBuffer();
      int messageLength = frameMsg->GetPackSize();
      if (messageLength > 0)
      {
        outStream.write(timeStamp.c_str(), timeStamp.size());
        outStream <<": "<<messageLength << std::endl;
        if(frameBitStream->GetKeyFrameUpdated())
          {
          outStream<< "IsKeyFrame: " << 1 << std::endl;
          }
        else
          {
          outStream<< "IsKeyFrame: " << 0 << std::endl;
          }
        outStream.write((char*)frameBitStream->GetMessageStreamBuffer()->GetPackPointer(), messageLength);
        outStream << std::endl;
      }
    }
  }
  
  outStream.close();
  
  this->StageWriteData(refNode);
  
  return 1;
}


//----------------------------------------------------------------------------
void vtkMRMLBitStreamSequenceStorageNode::InitializeSupportedReadFileTypes()
{
  this->SupportedWriteFileTypes->InsertNextValue("Video Bit Stream (.bin)");
  this->SupportedWriteFileTypes->InsertNextValue("Video Bit Stream (.seq.bin)");
}

//----------------------------------------------------------------------------
void vtkMRMLBitStreamSequenceStorageNode::InitializeSupportedWriteFileTypes()
{
  this->SupportedWriteFileTypes->InsertNextValue("Video Bit Stream (.bin)");
  this->SupportedWriteFileTypes->InsertNextValue("Video Bit Stream (.seq.bin)");
}

//----------------------------------------------------------------------------
const char* vtkMRMLBitStreamSequenceStorageNode::GetDefaultWriteFileExtension()
{
  return "bin";
}
