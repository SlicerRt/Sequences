/*=auto=========================================================================
 
 Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.
 
 See COPYRIGHT.txt
 or http://www.slicer.org/copyright/copyright.txt for details.
 
 =========================================================================auto=*/
///  vtkMRMLBitStreamSequenceStorageNode - MRML node that can read/write
///  a Sequence node containing bit stream in a text file
///

#ifndef __vtkMRMLBitStreamSequenceStorageNode_h
#define __vtkMRMLBitStreamSequenceStorageNode_h

#include "vtkSlicerSequencesModuleMRMLExport.h"

#include "vtkMRMLStorageNode.h"
#include <string>


/// \ingroup Slicer_QtModules_Sequences
class VTK_SLICER_SEQUENCES_MODULE_MRML_EXPORT vtkMRMLBitStreamSequenceStorageNode : public vtkMRMLStorageNode
{
public:
  
  static vtkMRMLBitStreamSequenceStorageNode *New();
  vtkTypeMacro(vtkMRMLBitStreamSequenceStorageNode,vtkMRMLStorageNode);
  
  virtual vtkMRMLNode* CreateNodeInstance();
  
  ///
  /// Get node XML tag name (like Storage, Model)
  virtual const char* GetNodeTagName()  {return "BitStreamSequenceStorage";};
  
  /// Return true if the node can be read in.
  virtual bool CanReadInReferenceNode(vtkMRMLNode *refNode);
  
  /// Return true if the node can be written by using thie writer.
  virtual bool CanWriteFromReferenceNode(vtkMRMLNode* refNode);
  
  /// Return a default file extension for writting
  virtual const char* GetDefaultWriteFileExtension();
  
protected:
  vtkMRMLBitStreamSequenceStorageNode();
  ~vtkMRMLBitStreamSequenceStorageNode();
  vtkMRMLBitStreamSequenceStorageNode(const vtkMRMLBitStreamSequenceStorageNode&);
  void operator=(const vtkMRMLBitStreamSequenceStorageNode&);
  
  
  virtual int WriteDataInternal(vtkMRMLNode *refNode);

  /// Does the actual reading. Returns 1 on success, 0 otherwise.
  /// Returns 0 by default (read not supported).
  /// This implementation delegates most everything to the superclass
  /// but it has an early exit if the file to be read is incompatible.
  virtual int ReadDataInternal(vtkMRMLNode* refNode);
  
  /// Initialize all the supported write file types
  virtual void InitializeSupportedReadFileTypes();
  
  /// Initialize all the supported write file types
  virtual void InitializeSupportedWriteFileTypes();
  
  /// String Operation
  int GetTagValue(char* headerString, int headerLenght, const char* tag, int tagLength, std::string &tagValueString, int&tagValueLength);
  
  /// String Operation
  int ReadElementsInSingleLine(std::string& firstElement, std::string& secondElement, FILE* stream);
};

#endif
