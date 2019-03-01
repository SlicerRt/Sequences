/*=auto=========================================================================

  Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

=========================================================================auto=*/
///  vtkMRMLLinearTransformSequenceStorageNode - MRML node that can read/write
///  a Sequence node containing linear transforms in a single nrrd or mha file
///

#ifndef __vtkMRMLLinearTransformSequenceStorageNode_h
#define __vtkMRMLLinearTransformSequenceStorageNode_h

#include "vtkSlicerSequencesModuleMRMLExport.h"

#include "vtkMRMLNRRDStorageNode.h"

#include <deque>

class vtkMRMLSequenceNode;

enum SequenceFileType
{
  INVALID_SEQUENCE_FILE,
  METAIMAGE_SEQUENCE_FILE,
  NRRD_SEQUENCE_FILE
};

/// \ingroup Slicer_QtModules_Sequences
class VTK_SLICER_SEQUENCES_MODULE_MRML_EXPORT vtkMRMLLinearTransformSequenceStorageNode : public vtkMRMLNRRDStorageNode
{
  public:

  static vtkMRMLLinearTransformSequenceStorageNode *New();
  vtkTypeMacro(vtkMRMLLinearTransformSequenceStorageNode,vtkMRMLNRRDStorageNode);

  virtual vtkMRMLNode* CreateNodeInstance() VTK_OVERRIDE;

  ///
  /// Get node XML tag name (like Storage, Model)
  virtual const char* GetNodeTagName() VTK_OVERRIDE {return "LinearTransformSequenceStorage";};

  /// Return true if the node can be read in.
  virtual bool CanReadInReferenceNode(vtkMRMLNode *refNode) VTK_OVERRIDE;

  /// Return true if the node can be written by using thie writer.
  virtual bool CanWriteFromReferenceNode(vtkMRMLNode* refNode) VTK_OVERRIDE;
  virtual int WriteDataInternal(vtkMRMLNode *refNode) VTK_OVERRIDE;

  ///
  /// Return a default file extension for writting
  virtual const char* GetDefaultWriteFileExtension() VTK_OVERRIDE;

  /// Read all the fields in the metaimage file header.
  /// If sequence nodes are passed in createdNodes then they will be reused. New sequence nodes will be created if there are more transforms
  /// in the sequence metafile than pointers in createdNodes. The caller is responsible for deleting all nodes in createdNodes.
  /// Return number of created transform nodes.
  static int ReadSequenceFileTransforms(const std::string& fileName, vtkMRMLScene *scene,
    std::deque< vtkSmartPointer<vtkMRMLSequenceNode> > &createdNodes, std::map< int, std::string >& frameNumberToIndexValueMap,
    std::map< std::string, std::string > &imageMetaData, SequenceFileType fileType = METAIMAGE_SEQUENCE_FILE);

  /// Write the transform fields to the metaimage header
  static bool WriteSequenceMetafileTransforms(const std::string& fileName, std::deque< vtkMRMLSequenceNode* > &transformNodes,
    std::deque< std::string > &transformNames, vtkMRMLSequenceNode* masterNode, vtkMRMLSequenceNode* imageNode);

protected:
  vtkMRMLLinearTransformSequenceStorageNode();
  ~vtkMRMLLinearTransformSequenceStorageNode();
  vtkMRMLLinearTransformSequenceStorageNode(const vtkMRMLLinearTransformSequenceStorageNode&);
  void operator=(const vtkMRMLLinearTransformSequenceStorageNode&);

  /// Does the actual reading. Returns 1 on success, 0 otherwise.
  /// Returns 0 by default (read not supported).
  /// This implementation delegates most everything to the superclass
  /// but it has an early exit if the file to be read is incompatible.
  virtual int ReadDataInternal(vtkMRMLNode* refNode) VTK_OVERRIDE;

  /// Initialize all the supported write file types
  virtual void InitializeSupportedReadFileTypes() VTK_OVERRIDE;

  /// Initialize all the supported write file types
  virtual void InitializeSupportedWriteFileTypes() VTK_OVERRIDE;
};

#endif
