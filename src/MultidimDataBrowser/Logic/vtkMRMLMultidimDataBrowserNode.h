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

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

#ifndef __vtkMRMLMultidimDataBrowserNode_h
#define __vtkMRMLMultidimDataBrowserNode_h

// MRML includes
#include <vtkMRML.h>
#include <vtkMRMLNode.h>

// STD includes
#include <set>
#include <map>

#include "vtkSlicerMultidimDataBrowserModuleLogicExport.h"

class vtkMRMLHierarchyNode;

class VTK_SLICER_MULTIDIMDATABROWSER_MODULE_LOGIC_EXPORT vtkMRMLMultidimDataBrowserNode : public vtkMRMLNode
{
public:
  static vtkMRMLMultidimDataBrowserNode *New();
  vtkTypeMacro(vtkMRMLMultidimDataBrowserNode,vtkMRMLNode);
  void PrintSelf(ostream& os, vtkIndent indent);

  /// Create instance of a GAD node. 
  virtual vtkMRMLNode* CreateNodeInstance();

  /// Set node attributes from name/value pairs 
  virtual void ReadXMLAttributes( const char** atts);

  /// Write this node's information to a MRML file in XML format. 
  virtual void WriteXML(ostream& of, int indent);

  /// Copy the node's attributes to this object 
  virtual void Copy(vtkMRMLNode *node);

  /// Get unique node XML tag name (like Volume, Model) 
  virtual const char* GetNodeTagName() {return "MultidimDataBrowser";};

  /// Set the multi-dimension hierarchy node root
  void SetAndObserveRootNodeID(const char *rootNodeID);
  /// Get the multi-dimension hierarchy node root
  vtkMRMLHierarchyNode* GetRootNode();
  
  /// Set the selected sequence node (branch in the hierarchy, corresponding to a specific parameter value)
  void SetAndObserveSelectedSequenceNodeID(const char *selectedSequenceNodeID);
  /// Get the selected sequence node (branch in the hierarchy, corresponding to a specific parameter value)
  vtkMRMLHierarchyNode* GetSelectedSequenceNode();

  /// Set the selected output virtual node (that will contain the copy a specific parameter value)
  void SetAndObserveVirtualOutputNodeID(const char *virtualOutputNodeID);
  /// Get the selected output virtual node (that will contain the copy a specific parameter value)
  vtkMRMLHierarchyNode* GetVirtualOutputNode();

  /// Get/Set automatic playback (automatic continuous changing of selected sequence nodes)
  vtkGetMacro(PlaybackActive, bool);
  vtkSetMacro(PlaybackActive, bool);
  vtkBooleanMacro(PlaybackActive, bool);

  /// Get/Set playback rate in fps (frames per second)
  vtkGetMacro(PlaybackRateFps, double);
  vtkSetMacro(PlaybackRateFps, double);

  /// Get/Set playback looping (restart from the first sequence node when reached the last one)
  vtkGetMacro(PlaybackLooped, bool);
  vtkSetMacro(PlaybackLooped, bool);
  vtkBooleanMacro(PlaybackLooped, bool);
  
protected:
  vtkMRMLMultidimDataBrowserNode();
  ~vtkMRMLMultidimDataBrowserNode();
  vtkMRMLMultidimDataBrowserNode(const vtkMRMLMultidimDataBrowserNode&);
  void operator=(const vtkMRMLMultidimDataBrowserNode&);

protected:
  bool PlaybackActive;
  double PlaybackRateFps;
  bool PlaybackLooped;
};

#endif
