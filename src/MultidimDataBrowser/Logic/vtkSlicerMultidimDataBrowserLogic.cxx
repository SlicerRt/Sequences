/*==============================================================================

  Program: 3D Slicer

  Portions (c) Copyright Brigham and Women's Hospital (BWH) All Rights Reserved.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

==============================================================================*/

// MultidimData Logic includes
#include "vtkSlicerMultidimDataBrowserLogic.h"
#include "vtkMRMLMultidimDataBrowserNode.h"
#include "vtkMRMLMultidimDataNode.h"

// MRML includes
#include "vtkMRMLModelNode.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLScalarVolumeDisplayNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// STL includes
#include <algorithm>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerMultidimDataBrowserLogic);

//----------------------------------------------------------------------------
vtkSlicerMultidimDataBrowserLogic::vtkSlicerMultidimDataBrowserLogic()
: UpdateVirtualOutputNodesInProgress(false)
{
}

//----------------------------------------------------------------------------
vtkSlicerMultidimDataBrowserLogic::~vtkSlicerMultidimDataBrowserLogic()
{  
}

//----------------------------------------------------------------------------
void vtkSlicerMultidimDataBrowserLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}

//---------------------------------------------------------------------------
void vtkSlicerMultidimDataBrowserLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerMultidimDataBrowserLogic::RegisterNodes()
{
  if (this->GetMRMLScene()==NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLMultidimDataBrowserNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerMultidimDataBrowserLogic::UpdateFromMRMLScene()
{
  if (this->GetMRMLScene()==NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
  // TODO: probably we need to add observer to all vtkMRMLMultidimDataBrowserNode-type nodes
}

//---------------------------------------------------------------------------
void vtkSlicerMultidimDataBrowserLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (node==NULL)
  {
    vtkErrorMacro("An invalid node is attempted to be added");
    return;
  }
  if (node->IsA("vtkMRMLMultidimDataBrowserNode"))
  {
    vtkDebugMacro("OnMRMLSceneNodeAdded: Have a vtkMRMLMultidimDataBrowserNode node");
    vtkUnObserveMRMLNodeMacro(node); // remove any previous observation that might have been added
    vtkObserveMRMLNodeMacro(node);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerMultidimDataBrowserLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (node==NULL)
  {
    vtkErrorMacro("An invalid node is attempted to be removed");
    return;
  }
  if (node->IsA("vtkMRMLMultidimDataBrowserNode"))
  {
    vtkDebugMacro("OnMRMLSceneNodeRemoved: Have a vtkMRMLMultidimDataBrowserNode node");
    vtkUnObserveMRMLNodeMacro(node);
  } 
}

//---------------------------------------------------------------------------
void vtkSlicerMultidimDataBrowserLogic::UpdateVirtualOutputNodes(vtkMRMLMultidimDataBrowserNode* browserNode)
{
  if (this->UpdateVirtualOutputNodesInProgress)
  {
    // avoid infinitie loops (caused by triggering a node update during a node update)
    return;
  }
  if (browserNode==NULL)
  {
    vtkWarningMacro("vtkSlicerMultidimDataBrowserLogic::UpdateVirtualOutputNodes failed: browserNode is invalid");
    return;
  }
  
  if (browserNode->GetRootNode()==NULL)
  {
    browserNode->RemoveAllVirtualOutputNodes();
    return;
  }

  vtkMRMLScene* scene=browserNode->GetScene();
  if (scene==NULL)
  {
    vtkErrorMacro("vtkSlicerMultidimDataBrowserLogic::UpdateVirtualOutputNodes failed: scene is invalid");
    return;
  }

  this->UpdateVirtualOutputNodesInProgress=true;
  
  int selectedBundleIndex=browserNode->GetSelectedBundleIndex();
  std::string parameterValue;
  if (selectedBundleIndex>=0)
  {
    parameterValue=browserNode->GetRootNode()->GetNthParameterValue(selectedBundleIndex);
  }

  std::vector< vtkMRMLMultidimDataNode* > synchronizedRootNodes;
  browserNode->GetSynchronizedRootNodes(synchronizedRootNodes, true);
  
  for (std::vector< vtkMRMLMultidimDataNode* >::iterator sourceRootNodeIt=synchronizedRootNodes.begin(); sourceRootNodeIt!=synchronizedRootNodes.end(); ++sourceRootNodeIt)
  {
    vtkMRMLMultidimDataNode* synchronizedRootNode=(*sourceRootNodeIt);
    if (synchronizedRootNode==NULL)
    {
      vtkErrorMacro("Synchronized root node is invalid");
      continue;
    }
    vtkMRMLNode* sourceNode=synchronizedRootNode->GetDataNodeAtValue(parameterValue.c_str());
    if (sourceNode==NULL)
    {
      // no source node is available for the chosen time point
      continue;
    }
    
    // Get the current target output node
    vtkMRMLNode* targetOutputNode=browserNode->GetVirtualOutputDataNode(synchronizedRootNode);    
    if (targetOutputNode!=NULL)
    {
      // a virtual output node with the requested role exists already
      if (strcmp(targetOutputNode->GetClassName(),sourceNode->GetClassName())!=0)
      {
        // this node is of a different class, cannot be reused
        targetOutputNode=NULL;
      }
    }

    // Create the virtual output node (and display nodes) if it doesn't exist yet
    if (targetOutputNode==NULL)
    {
      // Get the display nodes      
      std::vector< vtkMRMLDisplayNode* > sourceDisplayNodes;
      synchronizedRootNode->GetDisplayNodesAtValue(sourceDisplayNodes, parameterValue.c_str());

      // Add the new data and display nodes to the virtual outputs      
      targetOutputNode=browserNode->AddVirtualOutputNodes(sourceNode,sourceDisplayNodes,synchronizedRootNode);
    }

    if (targetOutputNode==NULL)
    {
      // failed to add target output node
      continue;
    }

    // Get the display nodes
    std::vector< vtkMRMLDisplayNode* > targetDisplayNodes;
    {
      vtkMRMLDisplayableNode* targetDisplayableNode=vtkMRMLDisplayableNode::SafeDownCast(targetOutputNode);
      if (targetDisplayableNode!=NULL)
      {
        int numOfDisplayNodes=targetDisplayableNode->GetNumberOfDisplayNodes();
        for (int displayNodeIndex=0; displayNodeIndex<numOfDisplayNodes; displayNodeIndex++)
        {
          targetDisplayNodes.push_back(targetDisplayableNode->GetNthDisplayNode(displayNodeIndex));
        }
      }
    }
    
    // Update the target node with the contents of the source node    

    // Slice browser is updated when there is a rename, but we want to avoid update, because
    // the source node may be hidden from editors and it would result in removing the target node
    // from the slicer browser. To avoid update of the slice browser, we set the name in advance.
    targetOutputNode->SetName(sourceNode->GetName());

    // Mostly it is a shallow copy (for example for volumes, models)
    //targetOutputNode->CopyWithSingleModifiedEvent(sourceNode);
    ShallowCopy(targetOutputNode, sourceNode);

    // Generation of data node name: root node name (dimension = parameterValue unit)
    const char* rootName=synchronizedRootNode->GetName();
    const char* dimensionName=synchronizedRootNode->GetDimensionName();
    const char* unit=synchronizedRootNode->GetUnit();
    std::string dataNodeName;
    dataNodeName+=(rootName?rootName:"?");
    dataNodeName+=" [";
    if (dimensionName)
    {
      dataNodeName+=dimensionName;
      dataNodeName+="=";
    }
    dataNodeName+=parameterValue;
    if (unit)
    {
      dataNodeName+=unit;
    }
    dataNodeName+="]";
    targetOutputNode->SetName(dataNodeName.c_str());

    vtkMRMLDisplayableNode* targetDisplayableNode=vtkMRMLDisplayableNode::SafeDownCast(targetOutputNode);
    if (targetDisplayableNode!=NULL)
    {
      // Get the minimum of the display nodes that are already in the target displayable node
      // and the number of display nodes that we need at the end
      int numOfDisplayNodes=targetDisplayNodes.size();
      if (numOfDisplayNodes>targetDisplayableNode->GetNumberOfDisplayNodes())
      {
        numOfDisplayNodes=targetDisplayableNode->GetNumberOfDisplayNodes();
      }

      for (int displayNodeIndex=0; displayNodeIndex<numOfDisplayNodes; displayNodeIndex++)
      {
        // SetAndObserveNthDisplayNodeID takes a long time, only run it if the display node ptr changed
        if (targetDisplayableNode->GetNthDisplayNode(displayNodeIndex)!=targetDisplayNodes[displayNodeIndex])
        {
          targetDisplayableNode->SetAndObserveNthDisplayNodeID(displayNodeIndex,targetDisplayNodes[displayNodeIndex]->GetID());
        }
      }
      if (targetDisplayableNode->GetNumberOfDisplayNodes()>targetDisplayNodes.size())
      {
        // there are some extra display nodes in the target data node that we have to delete
        while (targetDisplayableNode->GetNumberOfDisplayNodes()>targetDisplayNodes.size())
        {
          // remove last display node
          targetDisplayableNode->RemoveNthDisplayNodeID(targetDisplayableNode->GetNumberOfDisplayNodes()-1);
        }
      }
      if (targetDisplayableNode->GetNumberOfDisplayNodes()<targetDisplayNodes.size())
      {
        // we need to add some more display nodes
        for (int displayNodeIndex=numOfDisplayNodes; displayNodeIndex<targetDisplayNodes.size(); displayNodeIndex++)
        {
          targetDisplayableNode->AddAndObserveDisplayNodeID(targetDisplayNodes[displayNodeIndex]->GetID());
        }
      }
    }

  }

  this->UpdateVirtualOutputNodesInProgress=false;
}

//---------------------------------------------------------------------------
void vtkSlicerMultidimDataBrowserLogic::ProcessMRMLNodesEvents(vtkObject *caller, unsigned long event, void *vtkNotUsed(callData))
{
  vtkMRMLMultidimDataBrowserNode *browserNode = vtkMRMLMultidimDataBrowserNode::SafeDownCast(caller);
  if (browserNode==NULL)
  {
    vtkErrorMacro("Expected a vtkMRMLMultidimDataBrowserNode");
    return;
  }

  UpdateVirtualOutputNodes(browserNode);
}

//---------------------------------------------------------------------------
void vtkSlicerMultidimDataBrowserLogic::ShallowCopy(vtkMRMLNode* target, vtkMRMLNode* source)
{
  if (target->IsA("vtkMRMLScalarVolumeNode"))
  {
    vtkMRMLScalarVolumeNode* targetScalarVolumeNode=vtkMRMLScalarVolumeNode::SafeDownCast(target);
    vtkMRMLScalarVolumeNode* sourceScalarVolumeNode=vtkMRMLScalarVolumeNode::SafeDownCast(source);
    targetScalarVolumeNode->SetAndObserveImageData(sourceScalarVolumeNode->GetImageData());
    //targetScalarVolumeNode->SetAndObserveTransformNodeID(sourceScalarVolumeNode->GetTransformNodeID());
    vtkSmartPointer<vtkMatrix4x4> ijkToRasmatrix=vtkSmartPointer<vtkMatrix4x4>::New();
    sourceScalarVolumeNode->GetIJKToRASMatrix(ijkToRasmatrix);
    targetScalarVolumeNode->SetIJKToRASMatrix(ijkToRasmatrix);
    targetScalarVolumeNode->SetLabelMap(sourceScalarVolumeNode->GetLabelMap());
    targetScalarVolumeNode->SetName(sourceScalarVolumeNode->GetName());
  }
  if (target->IsA("vtkMRMLModelNode"))
  {
    vtkMRMLModelNode* targetModelNode=vtkMRMLModelNode::SafeDownCast(target);
    vtkMRMLModelNode* sourceModelNode=vtkMRMLModelNode::SafeDownCast(source);
    targetModelNode->SetAndObservePolyData(sourceModelNode->GetPolyData());
  }
  else
  {
    target->CopyWithSingleModifiedEvent(source);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerMultidimDataBrowserLogic::GetCompatibleNodesFromScene(vtkCollection* compatibleNodes, vtkMRMLMultidimDataNode* multidimDataRootNode)
{
  if (compatibleNodes==NULL)
  {
    vtkErrorMacro("vtkSlicerMultidimDataBrowserLogic::GetCompatibleNodesFromScene failed: compatibleNodes is invalid");
    return;
  }
  compatibleNodes->RemoveAllItems();
  if (multidimDataRootNode==NULL)
  {
    // if root node is invalid then there is no compatible node
    return;
  }
  if (this->GetMRMLScene()==NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
  if (multidimDataRootNode->GetDimensionName()==NULL)
  {
    vtkErrorMacro("vtkSlicerMultidimDataBrowserLogic::GetCompatibleNodesFromScene failed: root node dimension name is invalid");
    return;
  }
  std::string masterRootNodeDimension=multidimDataRootNode->GetDimensionName();
  vtkSmartPointer<vtkCollection> multidimNodes = vtkSmartPointer<vtkCollection>::Take(this->GetMRMLScene()->GetNodesByClass("vtkMRMLMultidimDataNode"));
  vtkObject* nextObject = NULL;
  for (multidimNodes->InitTraversal(); (nextObject = multidimNodes->GetNextItemAsObject()); )
  {
    vtkMRMLMultidimDataNode* multidimNode = vtkMRMLMultidimDataNode::SafeDownCast(nextObject);
    if (multidimNode==NULL)
    {
      continue;
    }
    if (multidimNode==multidimDataRootNode)
    {
      // do not add the master node itself to the list of compatible nodes
      continue;
    }
    if (multidimNode->GetDimensionName()==NULL)
    {
      vtkErrorMacro("vtkSlicerMultidimDataBrowserLogic::GetCompatibleNodesFromScene failed: potential compatible root node dimension name is invalid");
      continue;
    }
    if (masterRootNodeDimension.compare(multidimNode->GetDimensionName())==0)
    {
      // dimension name is matching, so we consider it compatible
      compatibleNodes->AddItem(multidimNode);
    }
  }
}
