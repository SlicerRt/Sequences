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
#include "vtkMRMLSequenceStorageNode.h"

// MRML includes
#include <vtkMRMLDisplayableNode.h>
#include <vtkMRMLDisplayNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkCollection.h>
#include <vtkObjectFactory.h>
#include <vtkImageData.h>
#include <vtkTimerLog.h>

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
  this->SetIndexType(this->NumericIndex);
  this->HideFromEditorsOff();
  this->SequenceScene=vtkMRMLScene::New();
}

//----------------------------------------------------------------------------
vtkMRMLSequenceNode::~vtkMRMLSequenceNode()
{
  this->SequenceScene->Delete();
  this->SequenceScene=NULL;
  this->SetIndexName(NULL);
  this->SetIndexUnit(NULL);
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

  const char* indexTypeString=GetIndexTypeAsString();
  if (indexTypeString!=NULL)
  {    
    of << indent << " indexType=\"" << indexTypeString << "\"";
  }

  of << indent << " indexValues=\"";
  for(std::deque< IndexEntryType >::iterator indexIt=this->IndexEntries.begin(); indexIt!=this->IndexEntries.end(); ++indexIt)
  {
    if (indexIt!=this->IndexEntries.begin())
    {
      // not the first index, add a separator before adding values
      of << ";";
    }
    if (indexIt->DataNode==NULL)
    {
      // If we have a data node ID then store that, it is the most we know about the node that should be there
      if (!indexIt->DataNodeID.empty())
      {
        // this is normal when sequence node is in scene view
        of << indexIt->DataNodeID << ":" << indexIt->IndexValue;
      }
      else
      {
        vtkErrorMacro("Error while writing node "<<(this->GetID()?this->GetID():"(unknown)")<<" to XML: data node is invalid at index value "<<indexIt->IndexValue);
      }
    }
    else
    {
      of << indexIt->DataNode->GetID() << ":" << indexIt->IndexValue;
    }
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
    else if (!strcmp(attName, "indexType"))     
    {
      int indexType=GetIndexTypeFromString(attValue);
      if (indexType<0 || indexType>=vtkMRMLSequenceNode::NumberOfIndexTypes)
      {
        vtkErrorMacro("Invalid index type: "<<(attValue?attValue:"(empty). Assuming TextIndex."));
        indexType=vtkMRMLSequenceNode::TextIndex;
      }
      SetIndexType(indexType);
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

  this->IndexEntries.clear();
  for(std::deque< IndexEntryType >::iterator sourceIndexIt=snode->IndexEntries.begin(); sourceIndexIt!=snode->IndexEntries.end(); ++sourceIndexIt)
  {
    IndexEntryType seqItem;
    seqItem.IndexValue=sourceIndexIt->IndexValue;
    if (sourceIndexIt->DataNode!=NULL)
    {
      seqItem.DataNode=this->SequenceScene->GetNodeByID(sourceIndexIt->DataNode->GetID());
      seqItem.DataNodeID.clear();
    }
    if (seqItem.DataNode==NULL)
    {
      // data node was not found, at least copy its ID
      seqItem.DataNodeID=sourceIndexIt->DataNodeID;
      if (seqItem.DataNodeID.empty())
      {
        vtkWarningMacro("vtkMRMLSequenceNode::Copy: node was not found at index value "<<seqItem.IndexValue);
      }
    }
    this->IndexEntries.push_back(seqItem);
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

  std::string newNodeName = this->SequenceScene->GetUniqueNameByString( node->GetName() );
  vtkMRMLNode* newNode=this->SequenceScene->CopyNode(node);
  newNode->SetName( newNodeName.c_str() ); // Make sure all the node names in the sequence's scene are unique for saving purposes
  
  // Hack to enforce deep copying of volumes (otherwise, a separate image data is not stored for each frame)
  // TODO: Is there a better way to architecture this?
  vtkMRMLVolumeNode* volNode = vtkMRMLVolumeNode::SafeDownCast(newNode);
  if (volNode!=NULL)
  {
    vtkImageData* imageDataCopy = vtkImageData::New();
    imageDataCopy->DeepCopy(volNode->GetImageData());
    volNode->SetAndObserveImageData(imageDataCopy);
  }

  vtkMRMLDisplayableNode* displayableNode=vtkMRMLDisplayableNode::SafeDownCast(node);
  if (displayableNode!=NULL)
  {
    vtkMRMLDisplayableNode* newDisplayableNode=vtkMRMLDisplayableNode::SafeDownCast(newNode);
    int numOfDisplayNodes=displayableNode->GetNumberOfDisplayNodes();
    for (int displayNodeIndex=0; displayNodeIndex<numOfDisplayNodes; displayNodeIndex++)
    {
      vtkMRMLDisplayNode* displayNode=vtkMRMLDisplayNode::SafeDownCast(this->SequenceScene->CopyNode(displayableNode->GetNthDisplayNode(displayNodeIndex)));

      // Performance optimization: disable auto WW/WL computation in scalar display nodes, as it computation time is very significant
      // and it would be performed each time when switching between volumes
      vtkMRMLScalarVolumeDisplayNode *scalarVolumeDisplayNode = vtkMRMLScalarVolumeDisplayNode::SafeDownCast(displayNode);
      if (scalarVolumeDisplayNode)
      {
        scalarVolumeDisplayNode->AutoWindowLevelOff();
      }

      newDisplayableNode->SetAndObserveNthDisplayNodeID(displayNodeIndex, displayNode->GetID());
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
  this->IndexEntries[seqItemIndex].DataNodeID.clear();
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
std::string vtkMRMLSequenceNode::GetDataNodeTagName()
{
  std::string undefinedReturn="undefined";
  if (this->IndexEntries.empty())
  {
    return undefinedReturn;
  }
  // All the nodes should be of the same class, so just get the class from the first one
  vtkMRMLNode* node=this->IndexEntries[0].DataNode;
  if (node==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::GetDataNodeClassName node is invalid");
    return undefinedReturn;
  }
  const char* tagName=node->GetNodeTagName();
  if (tagName==NULL)
  {
    return undefinedReturn;
  }
  return tagName;
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
    if (indexIt->DataNode==NULL)
    {
      indexIt->DataNode = this->SequenceScene->GetNodeByID(indexIt->DataNodeID);
      if (indexIt->DataNode!=NULL)
      {
        // clear the ID to remove redundancy in the data
        indexIt->DataNodeID.clear();
      }
    }
  }
}

//-----------------------------------------------------------
void vtkMRMLSequenceNode::SetIndexTypeFromString(const char *indexTypeString)
{
  int indexType=GetIndexTypeFromString(indexTypeString);
  this->SetIndexType(indexType);
}

//-----------------------------------------------------------
const char* vtkMRMLSequenceNode::GetIndexTypeAsString()
{
  return vtkMRMLSequenceNode::GetIndexTypeAsString(this->IndexType);
}

//-----------------------------------------------------------
const char* vtkMRMLSequenceNode::GetIndexTypeAsString(int indexType)
{
  switch (indexType)
  {
  case vtkMRMLSequenceNode::NumericIndex: return "numeric";
  case vtkMRMLSequenceNode::TextIndex: return "text";
  default:
    return NULL;
  }
}

//-----------------------------------------------------------
int vtkMRMLSequenceNode::GetIndexTypeFromString(const char* indexTypeString)
{
  if (indexTypeString==NULL)
  {
    return -1;
  }
  for (int i=0; i<vtkMRMLSequenceNode::NumberOfIndexTypes; i++)
  {
    if (strcmp(indexTypeString, GetIndexTypeAsString(i))==0)
    {
      // found it
      return i;
    }
  }
  return -1;
}
