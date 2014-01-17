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
#include "vtkMRMLSequenceStorageNode.h"

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
: IndexName(0)
, IndexUnit(0)
, SequenceScene(0)
{
  this->SetIndexName("time");
  this->SetIndexUnit("s");
  this->HideFromEditorsOff();
  this->SequenceScene=vtkMRMLScene::New();
}

//----------------------------------------------------------------------------
vtkMRMLSequenceNode::~vtkMRMLSequenceNode()
{
  this->SequenceScene->Delete();
  this->SequenceScene=NULL;
  SetIndexName(NULL);
  SetIndexUnit(NULL);
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceNode::RemoveAllDataNodes()
{  
  this->SequenceScene->Delete();
  this->SequenceScene=vtkMRMLScene::New();
  this->IndexEntries.clear();
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  if (this->IndexName != NULL)
  {
    of << indent << " indexName=\"" << this->IndexName << "\"";
  }
  if (this->IndexUnit != NULL)
  {
    of << indent << " indexUnit=\"" << this->IndexUnit << "\"";
  }

  of << indent << " indexValues=\"";
  for(std::deque< IndexEntryType >::iterator indexIt=this->IndexEntries.begin(); indexIt!=this->IndexEntries.end(); ++indexIt)
  {
    if (indexIt!=this->IndexEntries.begin())
    {
      // not the first index, add a separator before adding values
      of << ";";
    }
    of << indexIt->DataNode->GetID() << ":" << indexIt->IndexValue;
  }
  of << "\"";

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
    if (!strcmp(attName, "indexName")) 
    {
      this->SetIndexName(attValue);
    }
    else if (!strcmp(attName, "indexUnit")) 
    {
      this->SetIndexUnit(attValue);
    }
    else if (!strcmp(attName, "indexValues")) 
    {
      ReadIndexValues(attValue);
    }
  }
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceNode::ReadIndexValues(const std::string& indexText)
{
  this->IndexEntries.clear();

  std::stringstream ss(indexText);
  std::string nodeId_indexValue;
  while (std::getline(ss, nodeId_indexValue, ';'))
  {
    std::size_t indexValueSeparatorPos = nodeId_indexValue.find_first_of(':');
    if (indexValueSeparatorPos>0 && indexValueSeparatorPos != std::string::npos)
    {
      std::string nodeId = nodeId_indexValue.substr(0, indexValueSeparatorPos);
      std::string indexValue = nodeId_indexValue.substr(indexValueSeparatorPos+1, nodeId_indexValue.size()-indexValueSeparatorPos-1);
  
      IndexEntryType indexEntry;
      indexEntry.IndexValue=indexValue;
      // The nodes are not read yet, so we can only store the node ID and get the pointer to the node later (in UpdateScene())
      indexEntry.DataNodeID=nodeId;
      indexEntry.DataNode=NULL;
      this->IndexEntries.push_back(indexEntry);
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

  this->SetIndexName(snode->GetIndexName());
  this->SetIndexUnit(snode->GetIndexUnit());

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
void vtkMRMLSequenceNode::SetDataNodeAtValue(vtkMRMLNode* node, const char* indexValue)
{
  if (node==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::SetDataNodeAtValue failed, invalid node"); 
    return;
  }
  if (indexValue==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::SetDataNodeAtValue failed, invalid indexValue"); 
    return;
  }

  vtkMRMLNode* newNode=this->SequenceScene->CopyNode(node);

  vtkMRMLDisplayableNode* displayableNode=vtkMRMLDisplayableNode::SafeDownCast(node);
  if (displayableNode!=NULL)
  {
    vtkMRMLDisplayableNode* newDisplayableNode=vtkMRMLDisplayableNode::SafeDownCast(newNode);
    if (this->IndexEntries.size()==0)
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
      //GetDisplayNodesAtValue(displayNodes, indexValue);

    }
    else
    {
      // Overwrite the display nodes wih the display node(s) of the first node
      vtkMRMLDisplayableNode* firstDisplayableNode=vtkMRMLDisplayableNode::SafeDownCast(this->IndexEntries[0].DataNode);
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
  
  int seqItemIndex=GetSequenceItemIndex(indexValue);
  if (seqItemIndex<0)
  {
    // The sequence item doesn't exist yet
    IndexEntryType seqItem;
    seqItem.IndexValue=indexValue;
    this->IndexEntries.push_back(seqItem);
    seqItemIndex=this->IndexEntries.size()-1;
  }
  this->IndexEntries[seqItemIndex].DataNode=newNode;
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceNode::RemoveDataNodeAtValue(const char* indexValue)
{
  if (indexValue==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::RemoveDataNodeAtValue failed, invalid indexValue"); 
    return;
  }

  int seqItemIndex=GetSequenceItemIndex(indexValue);
  if (seqItemIndex<0)
  {
    vtkWarningMacro("vtkMRMLSequenceNode::RemoveDataNodeAtValue: node was not found at index value "<<indexValue);
    return;
  }
  // TODO: remove associated nodes as well (such as storage node)?
  this->SequenceScene->RemoveNode(this->IndexEntries[seqItemIndex].DataNode);
  this->IndexEntries.erase(this->IndexEntries.begin()+seqItemIndex);
}

//----------------------------------------------------------------------------
int vtkMRMLSequenceNode::GetSequenceItemIndex(const char* indexValue)
{
  if (indexValue==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::GetSequenceItemIndex failed, invalid index value"); 
    return -1;
  }
  int numberOfSeqItems=this->IndexEntries.size();
  for (int i=0; i<numberOfSeqItems; i++)
  {
    if (this->IndexEntries[i].IndexValue.compare(indexValue)==0)
    {
      return i;
    }
  }
  return -1;
}

//---------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLSequenceNode::GetDataNodeAtValue(const char* indexValue)
{
  if (indexValue==NULL)
  {
    vtkErrorMacro("GetDataNodesAtValue failed, invalid index value"); 
    return NULL;
  }
  if (this->SequenceScene==NULL)
  {
    vtkErrorMacro("GetDataNodesAtValue failed, invalid scene");
    return NULL;
  }
  int seqItemIndex=GetSequenceItemIndex(indexValue);
  if (seqItemIndex<0)
  {
    // sequence item is not found
    return NULL;
  }
  return this->IndexEntries[seqItemIndex].DataNode;
}

//---------------------------------------------------------------------------
std::string vtkMRMLSequenceNode::GetNthIndexValue(int seqItemIndex)
{
  if (seqItemIndex<0 || seqItemIndex>=this->IndexEntries.size())
  {
    vtkErrorMacro("vtkMRMLSequenceNode::GetNthIndexValue failed, invalid seqItemIndex value: "<<seqItemIndex);
    return "";
  }
  return this->IndexEntries[seqItemIndex].IndexValue;
}

//-----------------------------------------------------------------------------
int vtkMRMLSequenceNode::GetNumberOfDataNodes()
{
  return this->IndexEntries.size();
}

//-----------------------------------------------------------------------------
void vtkMRMLSequenceNode::UpdateIndexValue(const char* oldIndexValue, const char* newIndexValue)
{
  if (oldIndexValue==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::UpdateIndexValue failed, invalid oldIndexValue"); 
    return;
  }
  if (newIndexValue==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::UpdateIndexValue failed, invalid newIndexValue"); 
    return;
  }
  if (strcmp(oldIndexValue,newIndexValue)==0)
  {
    // no change
    return;
  }
  int seqItemIndex=GetSequenceItemIndex(oldIndexValue);
  if (seqItemIndex<0)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::UpdateIndexValue failed, no data node found with index value "<<oldIndexValue); 
    return;
  }
  // Update the index value
  this->IndexEntries[seqItemIndex].IndexValue=newIndexValue;
}

//-----------------------------------------------------------------------------
std::string vtkMRMLSequenceNode::GetDataNodeClassName()
{
  if (this->IndexEntries.empty())
  {
    return "";
  }
  // All the nodes should be of the same class, so just get the class from the first one
  vtkMRMLNode* node=this->IndexEntries[0].DataNode;
  if (node==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::GetDataNodeClassName node is invalid");
    return "";
  }
  const char* className=node->GetClassName();
  return SAFE_CHAR_POINTER(className);
}

//-----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLSequenceNode::GetNthDataNode(int itemNumber)
{
  if (this->IndexEntries.size()<=itemNumber)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::GetNthDataNode failed: itemNumber "<<itemNumber<<" is out of range");
    return NULL;
  }
  return this->IndexEntries[itemNumber].DataNode;
}

//---------------------------------------------------------------------------
void vtkMRMLSequenceNode::GetDisplayNodesAtValue(std::vector< vtkMRMLDisplayNode* > &displayNodes, const char* indexValue)
{
  displayNodes.clear();
  if (indexValue==NULL)
  {
    vtkErrorMacro("GetDisplayNodesAtValue failed, invalid index value"); 
    return;
  }
  if (this->SequenceScene==NULL)
  {
    vtkErrorMacro("GetDisplayNodesAtValue failed, invalid scene");
    return;
  }
  int seqItemIndex=GetSequenceItemIndex(indexValue);
  if (seqItemIndex<0)
  {
    // sequence item is not found
    return;
  }
  vtkMRMLDisplayableNode* displayableNode=vtkMRMLDisplayableNode::SafeDownCast(this->IndexEntries[seqItemIndex].DataNode);
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

//-----------------------------------------------------------------------------
vtkMRMLScene* vtkMRMLSequenceNode::GetSequenceScene()
{
  return this->SequenceScene;
}

//-----------------------------------------------------------------------------
vtkMRMLStorageNode* vtkMRMLSequenceNode::CreateDefaultStorageNode()
{
  return vtkMRMLSequenceStorageNode::New();
}

//-----------------------------------------------------------
void vtkMRMLSequenceNode::UpdateScene(vtkMRMLScene *scene)
{
  Superclass::UpdateScene(scene);

  // By now the storage node imported the sequence scene, so we can get the pointers to the data nodes
  for(std::deque< IndexEntryType >::iterator indexIt=this->IndexEntries.begin(); indexIt!=this->IndexEntries.end(); ++indexIt)
  {
    indexIt->DataNode = this->SequenceScene->GetNodeByID(indexIt->DataNodeID);
    indexIt->DataNodeID.clear(); // clear the ID to remove redundancy in the data
  }
}
