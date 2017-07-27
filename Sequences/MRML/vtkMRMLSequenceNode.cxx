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
#include "vtkMRMLLinearTransformSequenceStorageNode.h"
#include "vtkMRMLNodeSequencer.h"
#include "vtkMRMLSequenceNode.h"
#include "vtkMRMLSequenceStorageNode.h"
#include "vtkMRMLVolumeSequenceStorageNode.h"

// MRML includes
#include <vtkMRMLScene.h>

// VTK includes
#include <vtkNew.h>
#include <vtkCollection.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
//#include <vtkImageData.h>
#include <vtkSmartPointer.h>
#include <vtkTimerLog.h>

// STD includes
#include <sstream>

#define SAFE_CHAR_POINTER(unsafeString) ( unsafeString==NULL?"":unsafeString )

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLSequenceNode);

//----------------------------------------------------------------------------
vtkMRMLSequenceNode::vtkMRMLSequenceNode()
: SequenceScene(0)
, IndexType(vtkMRMLSequenceNode::NumericIndex)
, NumericIndexValueTolerance(0.001)
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
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceNode::RemoveAllDataNodes()
{
  this->IndexEntries.clear();
  this->SequenceScene->Delete();
  this->SequenceScene=vtkMRMLScene::New();
  this->Modified();
  this->StorableModifiedTime.Modified();
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);

  // Write all MRML node attributes into output stream
  vtkIndent indent(nIndent);

  if (!this->IndexName.empty())
  {
    of << indent << " indexName=\"" << this->IndexName << "\"";
  }
  if (!this->IndexUnit.empty())
  {
    of << indent << " indexUnit=\"" << this->IndexUnit << "\"";
  }

  std::string indexTypeString=GetIndexTypeAsString();
  if (!indexTypeString.empty())
  {    
    of << indent << " indexType=\"" << indexTypeString << "\"";
  }

  of << indent << " numericIndexValueTolerance=\"" << this->NumericIndexValueTolerance << "\"";

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
    else if (!strcmp(attName, "numericIndexValueTolerance"))
    {
      std::stringstream ss;
      ss << attValue;
      double numericIndexValueTolerance = 0.001;
      ss >> numericIndexValueTolerance;
      this->SetNumericIndexValueTolerance(numericIndexValueTolerance);
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
  bool modified = false;

  if (!this->IndexEntries.empty())
  {
    this->IndexEntries.clear();
    modified = true;
  }
  
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
      modified = true;
    }
  }

  if (modified)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLSequenceNode::Copy(vtkMRMLNode *anode)
{
  int wasModified = this->StartModify();
  Superclass::Copy(anode);

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
    vtkMRMLNodeSequencer::GetInstance()->GetNodeSequencer(node)->DeepCopyNodeToScene(node, this->SequenceScene);
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
  this->Modified();
  this->StorableModifiedTime.Modified();

  this->EndModify(wasModified);
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);
  os << indent << "indexName: " << this->IndexName + "\n";
  os << indent << "indexUnit: " << this->IndexUnit + "\n";

  std::string indexTypeString = GetIndexTypeAsString();
  os << indent << "indexType: " << indexTypeString << "\n";

  os << indent << "numericIndexValueTolerance: " << this->NumericIndexValueTolerance << "\n";

  os << indent << "indexValues: ";
  if (this->IndexEntries.empty())
  {
    os << "(none)";
  }
  else
  {
    os << this->IndexEntries[0].IndexValue;
    if (this->IndexEntries.size() > 1)
    {
      os << " ... " << this->IndexEntries[this->IndexEntries.size()-1].IndexValue;
      os << " (" << this->IndexEntries.size() << " items)";
    }
  }
  os << "\n";
}

//----------------------------------------------------------------------------
bool vtkMRMLSequenceNode::UpdateDataNodeAtValue(vtkMRMLNode* node, const std::string& indexValue, bool shallowCopy /* = false */)
{
  if (node==NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::UpdateDataNodeAtValue failed, invalid node"); 
    return false;
  }
  vtkMRMLNode* nodeToBeUpdated = this->GetDataNodeAtValue(indexValue);
  if (!nodeToBeUpdated)
  {
    vtkDebugMacro("vtkMRMLSequenceNode::UpdateDataNodeAtValue failed, indexValue not found");
    return false;
  }
  std::string originalName = (nodeToBeUpdated->GetName() ? nodeToBeUpdated->GetName() : "");
  vtkMRMLNodeSequencer::GetInstance()->GetNodeSequencer(node)->CopyNode(node, nodeToBeUpdated, shallowCopy);
  if (!originalName.empty())
  {
    // Restore original name to prevent changing of node name in the sequence node
    // (proxy node may always have the same name or have the index value in it,
    // none of which is preferable in the sequence scene).
    nodeToBeUpdated->SetName(originalName.c_str());
  }
  this->Modified();
  this->StorableModifiedTime.Modified();
  return true;
}

//----------------------------------------------------------------------------
int vtkMRMLSequenceNode::GetInsertPosition(const std::string& indexValue)
{
  int insertPosition = this->IndexEntries.size();
  if (this->IndexType == vtkMRMLSequenceNode::NumericIndex && !this->IndexEntries.empty())
  {
    int itemNumber = this->GetItemNumberFromIndexValue(indexValue, false);
    double numericIndexValue = atof(indexValue.c_str());
    double foundNumericIndexValue = atof(this->IndexEntries[itemNumber].IndexValue.c_str());
    if (numericIndexValue < foundNumericIndexValue) // Deals with case of index value being smaller than any in the sequence and numeric tolerances
    {
      insertPosition = itemNumber;
    }
    else
    {
      insertPosition = itemNumber + 1;
    }
  }
  return insertPosition;
}

//----------------------------------------------------------------------------
vtkMRMLNode* vtkMRMLSequenceNode::SetDataNodeAtValue(vtkMRMLNode* node, const std::string& indexValue)
{
  if (node == NULL)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::SetDataNodeAtValue failed, invalid node");
    return NULL;
  }

  // Add a copy of the node to the sequence's scene
  vtkMRMLNode* newNode = vtkMRMLNodeSequencer::GetInstance()->GetNodeSequencer(node)->DeepCopyNodeToScene(node, this->SequenceScene);
  int seqItemIndex = this->GetItemNumberFromIndexValue(indexValue);
  if (seqItemIndex<0)
  {
    // The sequence item doesn't exist yet
    seqItemIndex = GetInsertPosition(indexValue);
    // Create new item    
    IndexEntryType seqItem;
    seqItem.IndexValue = indexValue;
    this->IndexEntries.insert(this->IndexEntries.begin() + seqItemIndex, seqItem);
  }
  this->IndexEntries[seqItemIndex].DataNode = newNode;
  this->IndexEntries[seqItemIndex].DataNodeID.clear();
  this->Modified();
  this->StorableModifiedTime.Modified();
  return newNode;
}

//----------------------------------------------------------------------------
void vtkMRMLSequenceNode::RemoveDataNodeAtValue(const std::string& indexValue)
{
  int seqItemIndex = GetItemNumberFromIndexValue(indexValue);
  if (seqItemIndex<0)
  {
    vtkWarningMacro("vtkMRMLSequenceNode::RemoveDataNodeAtValue: node was not found at index value "<<indexValue);
    return;
  }
  // TODO: remove associated nodes as well (such as storage node)?
  this->SequenceScene->RemoveNode(this->IndexEntries[seqItemIndex].DataNode);
  this->IndexEntries.erase(this->IndexEntries.begin()+seqItemIndex);
  this->Modified();
  this->StorableModifiedTime.Modified();
}

//---------------------------------------------------------------------------
int vtkMRMLSequenceNode::GetItemNumberFromIndexValue(const std::string& indexValue, bool exactMatchRequired /* =true */)
{
  int numberOfSeqItems=this->IndexEntries.size();
  if (numberOfSeqItems == 0)
  {
    return -1;
  }

  // Binary search will be faster for numeric index
  if (this->IndexType == NumericIndex)
  {
    int lowerBound = 0;
    int upperBound = numberOfSeqItems-1;

    // Deal with index values not within the range of index values in the Sequence
    double numericIndexValue = atof(indexValue.c_str());
    double lowerNumericIndexValue = atof(this->IndexEntries[lowerBound].IndexValue.c_str());
    double upperNumericIndexValue = atof(this->IndexEntries[upperBound].IndexValue.c_str());
    if (numericIndexValue <= lowerNumericIndexValue + this->NumericIndexValueTolerance)
    {
      if (numericIndexValue < lowerNumericIndexValue - this->NumericIndexValueTolerance && exactMatchRequired)
      {
        return -1;
      }
      else
      {
        return lowerBound;
      }
    }
    if (numericIndexValue >= upperNumericIndexValue - this->NumericIndexValueTolerance)
    {
      if (numericIndexValue > upperNumericIndexValue + this->NumericIndexValueTolerance && exactMatchRequired)
      {
        return -1;
      }
      else
      {
        return upperBound;
      }
    }

    while (upperBound - lowerBound > 1)
    {
      // Note that if middle is equal to either lowerBound or upperBound then upperBound - lowerBound <= 1
      int middle = int((lowerBound + upperBound)/2);
      double middleNumericIndexValue = atof(this->IndexEntries[middle].IndexValue.c_str());
      if (fabs(numericIndexValue - middleNumericIndexValue) <= this->NumericIndexValueTolerance)
      {
        return middle;
      }
      if (numericIndexValue > middleNumericIndexValue)
      {
        lowerBound = middle;
      }
      if (numericIndexValue < middleNumericIndexValue)
      {
        upperBound = middle;
      }
    }
    if (!exactMatchRequired)
    {
      return lowerBound;
    }
  }

  // Need linear search for non-numeric index
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
vtkMRMLNode* vtkMRMLSequenceNode::GetDataNodeAtValue(const std::string& indexValue, bool exactMatchRequired /* =true */)
{
  if (this->SequenceScene==NULL)
  {
    vtkErrorMacro("GetDataNodesAtValue failed, invalid scene");
    return NULL;
  }
  int seqItemIndex = this->GetItemNumberFromIndexValue(indexValue, exactMatchRequired);
  if (seqItemIndex < 0)
  {
    // not found
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
bool vtkMRMLSequenceNode::UpdateIndexValue(const std::string& oldIndexValue, const std::string& newIndexValue)
{
  if (oldIndexValue == newIndexValue)
  {
    // no change
    return true;
  }
  int oldSeqItemIndex = GetItemNumberFromIndexValue(oldIndexValue);
  if (oldSeqItemIndex<0)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::UpdateIndexValue failed, no data node found with index value "<<oldIndexValue); 
    return false;
  }
  if (this->GetItemNumberFromIndexValue(newIndexValue) >= 0)
  {
    vtkErrorMacro("vtkMRMLSequenceNode::UpdateIndexValue failed, data node is already defined at index value " << newIndexValue);
    return false;
  }
  // Update the index value
  this->IndexEntries[oldSeqItemIndex].IndexValue = newIndexValue;
  if (this->IndexType == vtkMRMLSequenceNode::NumericIndex)
  {
    IndexEntryType movingEntry = this->IndexEntries[oldSeqItemIndex];
    // Remove from current position
    this->IndexEntries.erase(this->IndexEntries.begin() + oldSeqItemIndex);
    // Insert into new position
    int insertPosition = this->GetInsertPosition(newIndexValue);
    this->IndexEntries.insert(this->IndexEntries.begin() + insertPosition, movingEntry);
  }
  this->Modified();
  this->StorableModifiedTime.Modified();
  return true;
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
std::string vtkMRMLSequenceNode::GetDefaultStorageNodeClassName(const char* filename /* =NULL */)
{
  // Use specific volume sequence storage node, if possible
  std::vector< vtkSmartPointer<vtkMRMLStorageNode> > specializedStorageNodes;
  specializedStorageNodes.push_back(vtkSmartPointer<vtkMRMLVolumeSequenceStorageNode>::New());
  specializedStorageNodes.push_back(vtkSmartPointer<vtkMRMLLinearTransformSequenceStorageNode>::New());
  for (std::vector< vtkSmartPointer<vtkMRMLStorageNode> >::iterator specializedStorageNodeIt = specializedStorageNodes.begin();
    specializedStorageNodeIt != specializedStorageNodes.end(); ++specializedStorageNodeIt)
  {
    if (filename && (*specializedStorageNodeIt)->GetSupportedFileExtension(filename, false, true).empty())
    {
      // cannot write into the required file extension
      continue;
    }
    if ((*specializedStorageNodeIt)->CanWriteFromReferenceNode(this))
    {
      return (*specializedStorageNodeIt)->GetClassName();
    }
  } // Use generic storage node
  return "vtkMRMLSequenceStorageNode";
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
std::string vtkMRMLSequenceNode::GetIndexTypeAsString()
{
  return vtkMRMLSequenceNode::GetIndexTypeAsString(this->IndexType);
}

//-----------------------------------------------------------
std::string vtkMRMLSequenceNode::GetIndexTypeAsString(int indexType)
{
  switch (indexType)
  {
  case vtkMRMLSequenceNode::NumericIndex: return "numeric";
  case vtkMRMLSequenceNode::TextIndex: return "text";
  default:
    return "";
  }
}

//-----------------------------------------------------------
int vtkMRMLSequenceNode::GetIndexTypeFromString(const std::string& indexTypeString)
{
  for (int i=0; i<vtkMRMLSequenceNode::NumberOfIndexTypes; i++)
  {
    if (indexTypeString == GetIndexTypeAsString(i))
    {
      // found it
      return i;
    }
  }
  return -1;
}
