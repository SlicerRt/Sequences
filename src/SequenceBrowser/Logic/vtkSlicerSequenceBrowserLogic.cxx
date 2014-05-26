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

//#define ENABLE_PERFORMANCE_PROFILING

// Sequence Logic includes
#include "vtkSlicerSequenceBrowserLogic.h"
#include "vtkMRMLSequenceBrowserNode.h"
#include "vtkMRMLSequenceNode.h"

// MRML includes
#include "vtkMRMLCameraNode.h"
#include "vtkMRMLModelNode.h"
#include "vtkMRMLScalarVolumeNode.h"
#include "vtkMRMLScalarVolumeDisplayNode.h"
#include "vtkMRMLLinearTransformNode.h"
#include "vtkMRMLGridTransformNode.h"
#include "vtkMRMLBSplineTransformNode.h"
#include "vtkMRMLScene.h"

// VTK includes
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>

// STL includes
#include <algorithm>

#ifdef ENABLE_PERFORMANCE_PROFILING
#include "vtkTimerLog.h"
#endif 

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSlicerSequenceBrowserLogic);

//----------------------------------------------------------------------------
vtkSlicerSequenceBrowserLogic::vtkSlicerSequenceBrowserLogic()
: UpdateVirtualOutputNodesInProgress(false)
{
}

//----------------------------------------------------------------------------
vtkSlicerSequenceBrowserLogic::~vtkSlicerSequenceBrowserLogic()
{  
}

//----------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

}

//---------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::SetMRMLSceneInternal(vtkMRMLScene * newScene)
{
  vtkNew<vtkIntArray> events;
  events->InsertNextValue(vtkMRMLScene::NodeAddedEvent);
  events->InsertNextValue(vtkMRMLScene::NodeRemovedEvent);
  events->InsertNextValue(vtkMRMLScene::EndBatchProcessEvent);
  this->SetAndObserveMRMLSceneEventsInternal(newScene, events.GetPointer());
}

//-----------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::RegisterNodes()
{
  if (this->GetMRMLScene()==NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
  this->GetMRMLScene()->RegisterNodeClass(vtkSmartPointer<vtkMRMLSequenceBrowserNode>::New());
}

//---------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::UpdateFromMRMLScene()
{
  if (this->GetMRMLScene()==NULL)
  {
    vtkErrorMacro("Scene is invalid");
    return;
  }
  // TODO: probably we need to add observer to all vtkMRMLSequenceBrowserNode-type nodes
}

//---------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::OnMRMLSceneNodeAdded(vtkMRMLNode* node)
{
  if (node==NULL)
  {
    vtkErrorMacro("An invalid node is attempted to be added");
    return;
  }
  if (node->IsA("vtkMRMLSequenceBrowserNode"))
  {
    vtkDebugMacro("OnMRMLSceneNodeAdded: Have a vtkMRMLSequenceBrowserNode node");
    vtkUnObserveMRMLNodeMacro(node); // remove any previous observation that might have been added
    vtkObserveMRMLNodeMacro(node);
  }
}

//---------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::OnMRMLSceneNodeRemoved(vtkMRMLNode* node)
{
  if (node==NULL)
  {
    vtkErrorMacro("An invalid node is attempted to be removed");
    return;
  }
  if (node->IsA("vtkMRMLSequenceBrowserNode"))
  {
    vtkDebugMacro("OnMRMLSceneNodeRemoved: Have a vtkMRMLSequenceBrowserNode node");
    vtkUnObserveMRMLNodeMacro(node);
  } 
}

//---------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::UpdateVirtualOutputNodes(vtkMRMLSequenceBrowserNode* browserNode)
{
#ifdef ENABLE_PERFORMANCE_PROFILING
  vtkSmartPointer<vtkTimerLog> timer=vtkSmartPointer<vtkTimerLog>::New();      
  timer->StartTimer();  
#endif 

  if (this->UpdateVirtualOutputNodesInProgress)
  {
    // avoid infinitie loops (caused by triggering a node update during a node update)
    return;
  }
  if (browserNode==NULL)
  {
    vtkWarningMacro("vtkSlicerSequenceBrowserLogic::UpdateVirtualOutputNodes failed: browserNode is invalid");
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
    vtkErrorMacro("vtkSlicerSequenceBrowserLogic::UpdateVirtualOutputNodes failed: scene is invalid");
    return;
  }

  this->UpdateVirtualOutputNodesInProgress=true;
  
  int selectedItemNumber=browserNode->GetSelectedItemNumber();
  std::string indexValue;
  if (selectedItemNumber>=0)
  {
    indexValue=browserNode->GetRootNode()->GetNthIndexValue(selectedItemNumber);
  }

  std::vector< vtkMRMLSequenceNode* > synchronizedRootNodes;
  browserNode->GetSynchronizedRootNodes(synchronizedRootNodes, true);
  
  for (std::vector< vtkMRMLSequenceNode* >::iterator sourceRootNodeIt=synchronizedRootNodes.begin(); sourceRootNodeIt!=synchronizedRootNodes.end(); ++sourceRootNodeIt)
  {
    vtkMRMLSequenceNode* synchronizedRootNode=(*sourceRootNodeIt);
    if (synchronizedRootNode==NULL)
    {
      vtkErrorMacro("Synchronized root node is invalid");
      continue;
    }
    vtkMRMLNode* sourceNode=synchronizedRootNode->GetDataNodeAtValue(indexValue.c_str());
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
      synchronizedRootNode->GetDisplayNodesAtValue(sourceDisplayNodes, indexValue.c_str());

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
    this->ShallowCopy(targetOutputNode, sourceNode);

    // Generation of data node name: root node name (IndexName = IndexValue IndexUnit)
    const char* rootName=synchronizedRootNode->GetName();
    const char* indexName=synchronizedRootNode->GetIndexName();
    const char* unit=synchronizedRootNode->GetIndexUnit();
    std::string dataNodeName;
    dataNodeName+=(rootName?rootName:"?");
    dataNodeName+=" [";
    if (indexName)
    {
      dataNodeName+=indexName;
      dataNodeName+="=";
    }
    dataNodeName+=indexValue;
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

#ifdef ENABLE_PERFORMANCE_PROFILING
  timer->StopTimer();
  vtkWarningMacro("UpdateVirtualOutputNodes: " << timer->GetElapsedTime() << "sec\n");
#endif 
}

//---------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::ProcessMRMLNodesEvents(vtkObject *caller, unsigned long event, void *vtkNotUsed(callData))
{
  vtkMRMLSequenceBrowserNode *browserNode = vtkMRMLSequenceBrowserNode::SafeDownCast(caller);
  if (browserNode==NULL)
  {
    vtkErrorMacro("Expected a vtkMRMLSequenceBrowserNode");
    return;
  }

  this->UpdateVirtualOutputNodes(browserNode);
}

//---------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::ShallowCopy(vtkMRMLNode* target, vtkMRMLNode* source)
{
  int oldModified=target->StartModify();
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
  else if (target->IsA("vtkMRMLModelNode"))
  {
    vtkMRMLModelNode* targetModelNode=vtkMRMLModelNode::SafeDownCast(target);
    vtkMRMLModelNode* sourceModelNode=vtkMRMLModelNode::SafeDownCast(source);
    targetModelNode->SetAndObservePolyData(sourceModelNode->GetPolyData());
  }
  else if (target->IsA("vtkMRMLLinearTransformNode"))
  {
    vtkMRMLLinearTransformNode* targetLinearTransformNode=vtkMRMLLinearTransformNode::SafeDownCast(target);
    vtkMRMLLinearTransformNode* sourceLinearTransformNode=vtkMRMLLinearTransformNode::SafeDownCast(source);
    targetLinearTransformNode->SetMatrixTransformToParent(sourceLinearTransformNode->GetMatrixTransformToParent());
    targetLinearTransformNode->SetMatrixTransformFromParent(sourceLinearTransformNode->GetMatrixTransformFromParent());
  }
  else if (target->IsA("vtkMRMLBSplineTransformNode"))
  {
    vtkMRMLBSplineTransformNode* targetBSplineTransformNode=vtkMRMLBSplineTransformNode::SafeDownCast(target);
    vtkMRMLBSplineTransformNode* sourceBSplineTransformNode=vtkMRMLBSplineTransformNode::SafeDownCast(source);
    targetBSplineTransformNode->SetAndObserveTransformToParent(sourceBSplineTransformNode->GetTransformToParent());
    targetBSplineTransformNode->SetAndObserveTransformFromParent(sourceBSplineTransformNode->GetTransformFromParent());
  }
  else if (target->IsA("vtkMRMLGridTransformNode"))
  {
    vtkMRMLGridTransformNode* targetGridTransformNode=vtkMRMLGridTransformNode::SafeDownCast(target);
    vtkMRMLGridTransformNode* sourceGridTransformNode=vtkMRMLGridTransformNode::SafeDownCast(source);
    targetGridTransformNode->SetAndObserveTransformToParent(sourceGridTransformNode->GetTransformToParent());
    targetGridTransformNode->SetAndObserveTransformFromParent(sourceGridTransformNode->GetTransformFromParent());
  }
  else if (target->IsA("vtkMRMLCameraNode"))
  {
    vtkMRMLCameraNode* targetCameraNode=vtkMRMLCameraNode::SafeDownCast(target);
    vtkMRMLCameraNode* sourceCameraNode=vtkMRMLCameraNode::SafeDownCast(source);
    int disabledModify = targetCameraNode->StartModify();
    targetCameraNode->SetPosition(sourceCameraNode->GetPosition());
    targetCameraNode->SetFocalPoint(sourceCameraNode->GetFocalPoint());
    targetCameraNode->SetViewUp(sourceCameraNode->GetViewUp());
    targetCameraNode->SetParallelProjection(sourceCameraNode->GetParallelProjection());
    targetCameraNode->SetParallelScale(sourceCameraNode->GetParallelScale());
    // Maybe the new position and focalpoint combo doesn't fit the existing
    // clipping range
    targetCameraNode->ResetClippingRange();
    targetCameraNode->EndModify(disabledModify);
  }
  else
  {
    target->CopyWithSingleModifiedEvent(source);
  }
  target->EndModify(oldModified);
}

//---------------------------------------------------------------------------
void vtkSlicerSequenceBrowserLogic::GetCompatibleNodesFromScene(vtkCollection* compatibleNodes, vtkMRMLSequenceNode* multidimDataRootNode)
{
  if (compatibleNodes==NULL)
  {
    vtkErrorMacro("vtkSlicerSequenceBrowserLogic::GetCompatibleNodesFromScene failed: compatibleNodes is invalid");
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
  if (multidimDataRootNode->GetIndexName()==NULL)
  {
    vtkErrorMacro("vtkSlicerSequenceBrowserLogic::GetCompatibleNodesFromScene failed: root node index name is invalid");
    return;
  }
  std::string masterRootNodeIndexName=multidimDataRootNode->GetIndexName();
  vtkSmartPointer<vtkCollection> multidimNodes = vtkSmartPointer<vtkCollection>::Take(this->GetMRMLScene()->GetNodesByClass("vtkMRMLSequenceNode"));
  vtkObject* nextObject = NULL;
  for (multidimNodes->InitTraversal(); (nextObject = multidimNodes->GetNextItemAsObject()); )
  {
    vtkMRMLSequenceNode* multidimNode = vtkMRMLSequenceNode::SafeDownCast(nextObject);
    if (multidimNode==NULL)
    {
      continue;
    }
    if (multidimNode==multidimDataRootNode)
    {
      // do not add the master node itself to the list of compatible nodes
      continue;
    }
    if (multidimNode->GetIndexName()==NULL)
    {
      vtkErrorMacro("vtkSlicerSequenceBrowserLogic::GetCompatibleNodesFromScene failed: potential compatible root node index name is invalid");
      continue;
    }
    if (masterRootNodeIndexName.compare(multidimNode->GetIndexName())==0)
    {
      // index name is matching, so we consider it compatible
      compatibleNodes->AddItem(multidimNode);
    }
  }
}
