/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

#ifndef __vtkMRMLNodeSequencer_h
#define __vtkMRMLNodeSequencer_h

// VTK includes
#include <vtkObject.h>
#include <vtkSingleton.h>
#include <vtkSmartPointer.h>

#include <list>
#include <vector>

#include "vtkSlicerSequencesModuleMRMLExport.h"

class vtkMRMLNode;
class vtkMRMLScene;
class vtkIntArray;
class vtkMRMLSequenceNode;

/// \brief Singleton node that contain MRML node specific methods for treating them as sequences.
///
/// This class stores all information and methods that describe how to manage MRML nodes
/// in sequences.
/// Once Sequences are stabilized, MRML nodes can be improved to provide these methods and information
/// (e.g., list of events that indicate content change, fixing of Copy to always perform deep copy,
/// add shallow copy method) but for now just keep these outside of MRML nodes.
class VTK_SLICER_SEQUENCES_MODULE_MRML_EXPORT vtkMRMLNodeSequencer
  : public vtkObject
{
public:

  /// Information and methods that are needed for efficient recording and replay of sequences.
  class VTK_SLICER_SEQUENCES_MODULE_MRML_EXPORT NodeSequencer
  {
  public:
    NodeSequencer();
    virtual ~NodeSequencer();
    virtual void CopyNode(vtkMRMLNode* source, vtkMRMLNode* target, bool shallowCopy = false);
    virtual vtkMRMLNode* DeepCopyNodeToScene(vtkMRMLNode* source, vtkMRMLScene* scene);
    virtual vtkIntArray* GetRecordingEvents();
    virtual std::string GetSupportedNodeClassName();
    virtual bool IsNodeSupported(vtkMRMLNode* node);
    virtual bool IsNodeSupported(const std::string& nodeClassName);
    virtual void AddDefaultDisplayNodes(vtkMRMLNode* node);
    virtual void AddDefaultSequenceStorageNode(vtkMRMLSequenceNode* node);
    virtual std::string GetDefaultSequenceStorageNodeClassName();

  protected:
    void CopyNodeAttributes(vtkMRMLNode* source, vtkMRMLNode* target);
    
    vtkSmartPointer< vtkIntArray > RecordingEvents;
    // Name of the MRML node class that this sequencer supports.
    // It may be an abstract class.
    std::string SupportedNodeClassName;
    // Names of parent classes of SupportedNodeClass.
    // They are used for determining what is the most specific sequences for a node class.
    std::vector< std::string > SupportedNodeParentClassNames;
    // Name of the MRMLStorageNode class that should be used to store this node type.
    std::string DefaultSequenceStorageNodeClassName;
  };

  vtkTypeMacro(vtkMRMLNodeSequencer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// This is a singleton pattern New.  There will only be ONE
  /// reference to a vtkMRMLNodeSequencer object per process. Clients that
  /// call this must call Delete on the object so that the reference counting will work.
  /// The single instance will be unreferenced when the program exits.
  static vtkMRMLNodeSequencer *New();

  /// Return the singleton instance with no reference counting.
  static vtkMRMLNodeSequencer* GetInstance();

  NodeSequencer* GetNodeSequencer(vtkMRMLNode* node);

  /// Registers a new node sequencer.
  /// This class takes ownership of the sequencer pointer and will delete it when the sequencer is no longer needed.
  /// The nodeInstance pointer has to be provided so that it can be determined how specific the sequencer is
  /// (the sequencer most specific to a node is returned by GetNodeSequencer).
  void RegisterNodeSequencer(NodeSequencer* sequencer);

  /// Returns list of supported node class names, except generic vtkMRMLNode.
  const std::vector< std::string > & GetSupportedNodeClassNames();

protected:

  vtkMRMLNodeSequencer();
  virtual ~vtkMRMLNodeSequencer();

  VTK_SINGLETON_DECLARE(vtkMRMLNodeSequencer);

  /// Map from node class name to sequencer information
  /// NodeSequencers owns the NodeSequencer pointer and deletes the pointer when the class is destroyed.
  std::list< NodeSequencer* > NodeSequencers;

  /// Cached list of supported node class names, except generic vtkMRMLNode.
  std::vector< std::string > SupportedNodeClassNames;

private:

  vtkMRMLNodeSequencer(const vtkMRMLNodeSequencer&);
  void operator=(const vtkMRMLNodeSequencer&);

};

#ifndef __VTK_WRAP__
//BTX
VTK_SINGLETON_DECLARE_INITIALIZER(VTK_SLICER_SEQUENCES_MODULE_MRML_EXPORT,
                                  vtkMRMLNodeSequencer);
//ETX
#endif // __VTK_WRAP__

#endif
