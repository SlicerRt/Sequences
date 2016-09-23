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
  class NodeSequencer
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

  protected:
    vtkSmartPointer< vtkIntArray > RecordingEvents;
    std::string SupportedNodeClassName;
    std::vector< std::string > SupportedNodeParentClassNames;
  };

  vtkTypeMacro(vtkMRMLNodeSequencer, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

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

protected:

  vtkMRMLNodeSequencer();
  virtual ~vtkMRMLNodeSequencer();

  VTK_SINGLETON_DECLARE(vtkMRMLNodeSequencer);

  /// Map from node class name to sequencer information
  /// NodeSequencers owns the NodeSequencer pointer and deletes the pointer when the class is destroyed.
  std::list< NodeSequencer* > NodeSequencers;

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
