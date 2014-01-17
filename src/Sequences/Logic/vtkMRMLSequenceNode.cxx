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

// MRMLSequence includes
#include "vtkMRMLSequenceNode.h"
#include "vtkMRMLDisplayableNode.h"
#include "vtkMRMLDisplayNode.h"

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkCollection.h>
#include <vtkObjectFactory.h>

// STD includes
#include <sstream>

#define SAFE_CHAR_POINTER(unsafeString) ( unsafeString==NULL?"":unsafeString )

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLSequenceNode);

//----------------------------------------------------------------------------
vtkMRMLSequenceNode::vtkMRMLSequenceNode()
: DimensionName(0)
, Unit(0)
, SequenceScene(0)
{
  this->SetDimensionName("time");
  this->SetUnit("s");
  this->HideFromEditors = false;
  this->SequenceScene=vtkMRMLScene::New();
}

//----------------------------------------------------------------------------
vtkMRMLSequenceNode::~vtkMRMLSequenceNode()
{
  this->SequenceScene->Delete();
  this->SequenceScene=NULL;
  SetDimensionName(NULL);
  SetUnit(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceNode::RemoveAllDataNodes()
{  
  this->SequenceScene->Delete();
  this->SequenceScene=vtkMRMLScene::New();
  this->SequenceItems.clear();
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  if (this->DimensionName != NULL)
  {
    of << indent << " dimensionName=\"" << this->DimensionName << "\"";
  }
  if (this->Unit != NULL)
  {
    of << indent << " unit=\"" << this->Unit << "\"";
  }

  // TODO: implement a vtkMRMLSequenceStorageNode that reads/writes the SequenceScene from/to file
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);

  // Read all MRML node attributes from two arrays of names and values
  const char* attName;
  const char* attValue;
  while (*atts != NULL) 
  {
    attName = *(atts++);
    attValue = *(atts++);
    if (!strcmp(attName, "dimensionName")) 
    {
      this->SetDimensionName(attValue);
    }
    else if (!strcmp(attName, "unit")) 
    {
      this->SetUnit(attValue);
    }
  }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLSequenceNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
  this->DisableModifiedEventOn();

  vtkMRMLSequenceNode *snode = (vtkMRMLSequenceNode *) anode;

  this->SetDimensionName(snode->GetDimensionName());
  this->SetUnit(snode->GetUnit());

  // Clear nodes: RemoveAllNodes is not a public method, so it's simpler to just delete and recreate the scene
  this->SequenceScene->Delete();
  this->SequenceScene=vtkMRMLScene::New();

  for (int n=0; n < snode->SequenceScene->GetNodes()->GetNumberOfItems(); n++)
  {
    vtkMRMLNode* node = (vtkMRMLNode*)snode->SequenceScene->GetNodes()->GetItemAsObject(n);
    if (node==NULL)
    {
      vtkErrorMacro("Invalid node in vtkMRMLSequenceNode");
      continue;
    }
    this->SequenceScene->CopyNode(node);
  }

  this->DisableModifiedEventOff();
  this->InvokePendingModifiedEvent();
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceNode::SetDataNodeAtValue(vtkMRMLNode* node, const char* parameterValue)
{
  if (node==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::SetDataNodeAtValue failed, invalid node"); 
    return;
  }
  if (parameterValue==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::SetDataNodeAtValue failed, invalid parameterValue"); 
    return;
  }

  vtkMRMLNode* newNode=this->SequenceScene->CopyNode(node);

  vtkMRMLDisplayableNode* displayableNode=vtkMRMLDisplayableNode::SafeDownCast(node);
  if (displayableNode!=NULL)
  {
    vtkMRMLDisplayableNode* newDisplayableNode=vtkMRMLDisplayableNode::SafeDownCast(newNode);
    if (this->SequenceItems.size()==0)
    {
      // This is the first node, so make a copy of the display node for the sequence
      int numOfDisplayNodes=displayableNode->GetNumberOfDisplayNodes();
      for (int displayNodeIndex=0; displayNodeIndex<numOfDisplayNodes; displayNodeIndex++)
      {
        vtkMRMLDisplayNode* displayNode=vtkMRMLDisplayNode::SafeDownCast(this->SequenceScene->CopyNode(displayableNode->GetNthDisplayNode(displayNodeIndex)));
        newDisplayableNode->SetAndObserveNthDisplayNodeID(displayNodeIndex, displayNode->GetID());
      }

      // TODO: jsut for test
      //std::vector< vtkMRMLDisplayNode* > displayNodes;
      //GetDisplayNodesAtValue(displayNodes, parameterValue);

    }
    else
    {
      // Overwrite the display nodes wih the display node(s) of the first node
      vtkMRMLDisplayableNode* firstDisplayableNode=vtkMRMLDisplayableNode::SafeDownCast(this->SequenceItems[0].DataNode);
      if (firstDisplayableNode!=NULL)
      {
        newDisplayableNode->RemoveAllDisplayNodeIDs();
        int numOfFirstDisplayNodes=firstDisplayableNode->GetNumberOfDisplayNodes();        
        for (int firstDisplayNodeIndex=0; firstDisplayNodeIndex<numOfFirstDisplayNodes; firstDisplayNodeIndex++)
        {
          newDisplayableNode->AddAndObserveDisplayNodeID(firstDisplayableNode->GetNthDisplayNodeID(firstDisplayNodeIndex));
        }
      }
      else
      {
        vtkErrorMacro("First node is not a displayable node");
      }
    }
  }
  
  int seqItemIndex=GetSequenceItemIndex(parameterValue);
  if (seqItemIndex<0)
  {
    // The sequence item doesn't exist yet
    SequenceItemType seqItem;
    seqItem.ParameterValue=parameterValue;
    this->SequenceItems.push_back(seqItem);
    seqItemIndex=this->SequenceItems.size()-1;
  }
  this->SequenceItems[seqItemIndex].DataNode=newNode;
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceNode::RemoveDataNodeAtValue(const char* parameterValue)
{
  if (parameterValue==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::RemoveDataNodeAtValue failed, invalid parameterValue"); 
    return;
  }

  int seqItemIndex=GetSequenceItemIndex(parameterValue);
  if (seqItemIndex<0)
  {
    vtkWarningMacro("vtkMRMLSequenceNode::RemoveDataNodeAtValue: node was not found at parameter value "<<parameterValue);
    return;
  }
  // TODO: remove associated nodes as well (such as storage node)?
  this->SequenceScene->RemoveNode(this->SequenceItems[seqItemIndex].DataNode);
  this->SequenceItems.erase(this->SequenceItems.begin()+seqItemIndex);
}

//----------------------------------------------------------------------------
int vtkMRMLSequenceNode::GetSequenceItemIndex(const char* parameterValue)
{
  if (parameterValue==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::GetSequenceItemIndex failed, invalid parameter value"); 
    return -1;
  }
  int numberOfSeqItems=this->SequenceItems.size();
  for (int i=0; i<numberOfSeqItems; i++)
  {
    if (this->SequenceItems[i].ParameterValue.compare(parameterValue)==0)
    {
      return i;
    }
  }
  return -1;
}

//---------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLSequenceNode::GetDataNodeAtValue(const char* parameterValue)
{
  if (parameterValue==NULL)
  {
    vtkErrorMacro("GetDataNodesAtValue failed, invalid parameter value"); 
    return NULL;
  }
  if (this->SequenceScene==NULL)
  {
    vtkErrorMacro("GetDataNodesAtValue failed, invalid scene");
    return NULL;
  }
  int seqItemIndex=GetSequenceItemIndex(parameterValue);
  if (seqItemIndex<0)
  {
    // sequence item is not found
    return NULL;
  }
  return this->SequenceItems[seqItemIndex].DataNode;
}

//---------------------------------------------------------------------------
std::string vtkMRMLSequenceNode::GetNthParameterValue(int seqItemIndex)
{
  if (seqItemIndex<0 || seqItemIndex>=this->SequenceItems.size())
  {
    vtkErrorMacro("vtkMRMLSequenceNode::GetNthParameterValue failed, invalid seqItemIndex value: "<<seqItemIndex);
    return "";
  }
  return this->SequenceItems[seqItemIndex].ParameterValue;
}

//-----------------------------------------------------------------------------
int vtkMRMLSequenceNode::GetNumberOfDataNodes()
{
  return this->SequenceItems.size();
}

//-----------------------------------------------------------------------------
void vtkMRMLSequenceNode::UpdateParameterValue(const char* oldParameterValue, const char* newParameterValue)
{
  if (oldParameterValue==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::UpdateParameterValue failed, invalid oldParameterValue"); 
    return;
  }
  if (newParameterValue==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::UpdateParameterValue failed, invalid newParameterValue"); 
    return;
  }
  if (strcmp(oldParameterValue,newParameterValue)==0)
  {
    // no change
    return;
  }
  int seqItemIndex=GetSequenceItemIndex(oldParameterValue);
  if (seqItemIndex<0)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::UpdateParameterValue failed, no bundle found with parameter value "<<oldParameterValue); 
    return;
  }
  // Update the parameter value
  this->SequenceItems[seqItemIndex].ParameterValue=newParameterValue;
}

//-----------------------------------------------------------------------------
std::string vtkMRMLSequenceNode::GetDataNodeClassName()
{
  if (this->SequenceItems.empty())
  {
    return "";
  }
  // All the nodes should be of the same class, so just get the class from the first one
  vtkMRMLNode* node=this->SequenceItems[0].DataNode;
  if (node==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::GetDataNodeClassName node is invalid");
    return "";
  }
  const char* className=node->GetClassName();
  return SAFE_CHAR_POINTER(className);
}

//-----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLSequenceNode::GetNthDataNode(int bundleIndex)
{
  if (this->SequenceItems.size()<=bundleIndex)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::GetNthDataNode failed: bundleIndex "<<bundleIndex<<" is out of range");
    return NULL;
  }
  return this->SequenceItems[bundleIndex].DataNode;
}

//---------------------------------------------------------------------------
void vtkMRMLSequenceNode::GetDisplayNodesAtValue(std::vector< vtkMRMLDisplayNode* > &displayNodes, const char* parameterValue)
{
  displayNodes.clear();
  if (parameterValue==NULL)
  {
    vtkErrorMacro("GetDisplayNodesAtValue failed, invalid parameter value"); 
    return;
  }
  if (this->SequenceScene==NULL)
  {
    vtkErrorMacro("GetDisplayNodesAtValue failed, invalid scene");
    return;
  }
  int seqItemIndex=GetSequenceItemIndex(parameterValue);
  if (seqItemIndex<0)
  {
    // sequence item is not found
    return;
  }
  vtkMRMLDisplayableNode* displayableNode=vtkMRMLDisplayableNode::SafeDownCast(this->SequenceItems[seqItemIndex].DataNode);
  if (displayableNode==NULL)
  {
    // not a displayable node, so there are no display nodes
    return;
  }
  int numOfDisplayNodes=displayableNode->GetNumberOfDisplayNodes();
  for (int displayNodeIndex=0; displayNodeIndex<numOfDisplayNodes; displayNodeIndex++)
  {
    vtkMRMLDisplayNode* displayNode=vtkMRMLDisplayNode::SafeDownCast(displayableNode->GetNthDisplayNode(displayNodeIndex));
    displayNodes.push_back(displayNode);
  }  
}
