/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

=========================================================================auto=*/
///  vtkMRMLVolumeSequenceStorageNode - MRML node that can read/write
///  a Sequence node containing volumes in a single NRRD file
/// 

#ifndef __vtkMRMLVolumeSequenceStorageNode_h
#define __vtkMRMLVolumeSequenceStorageNode_h

#include "vtkSlicerSequencesModuleMRMLExport.h"

#include "vtkMRMLNRRDStorageNode.h"
#include <string>

/// \ingroup Slicer_QtModules_Sequences
class VTK_SLICER_SEQUENCES_MODULE_MRML_EXPORT vtkMRMLVolumeSequenceStorageNode : public vtkMRMLNRRDStorageNode
{
  public:

  static vtkMRMLVolumeSequenceStorageNode *New();
  vtkTypeMacro(vtkMRMLVolumeSequenceStorageNode,vtkMRMLNRRDStorageNode);

  virtual vtkMRMLNode* CreateNodeInstance();

  /// 
  /// Get node XML tag name (like Storage, Model)
  virtual const char* GetNodeTagName()  {return "VolumeSequenceStorage";};

  /// Return true if the node can be read in.
  virtual bool CanReadInReferenceNode(vtkMRMLNode *refNode);

  /// Return true if the node can be written by using thie writer.
  virtual bool CanWriteFromReferenceNode(vtkMRMLNode* refNode);
  virtual int WriteDataInternal(vtkMRMLNode *refNode);

  ///
  /// Return a default file extension for writting
  virtual const char* GetDefaultWriteFileExtension();

protected:
  vtkMRMLVolumeSequenceStorageNode();
  ~vtkMRMLVolumeSequenceStorageNode();
  vtkMRMLVolumeSequenceStorageNode(const vtkMRMLVolumeSequenceStorageNode&);
  void operator=(const vtkMRMLVolumeSequenceStorageNode&);

  /// Does the actual reading. Returns 1 on success, 0 otherwise.
  /// Returns 0 by default (read not supported).
  /// This implementation delegates most everything to the superclass
  /// but it has an early exit if the file to be read is incompatible.
  virtual int ReadDataInternal(vtkMRMLNode* refNode);
  
  /// Initialize all the supported write file types
  virtual void InitializeSupportedReadFileTypes();

  /// Initialize all the supported write file types
  virtual void InitializeSupportedWriteFileTypes();
};

#endif
