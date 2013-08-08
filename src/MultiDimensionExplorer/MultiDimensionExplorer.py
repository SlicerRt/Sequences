import os
import unittest
import string
from __main__ import vtk, qt, ctk, slicer

#
# MultiDimension Explorer module
#

class MultiDimensionExplorer:
  def __init__(self, parent):
    parent.title = "MultiDimension Explorer"
    parent.categories = ["MultiDimension"]
    parent.dependencies = []
    parent.contributors = ["Kevin Wang (Princess Margaret Cancer Centre)"]
    parent.helpText = string.Template("""
    Use this module to compose a MultiDimension MRML node using multiple ScalarVolumeNode. 
""").substitute({ 'a':parent.slicerWikiUrl, 'b':slicer.app.majorVersion, 'c':slicer.app.minorVersion })
    parent.acknowledgementText = """
    Supported by SparKit, OCAIRO and the Slicer Community. See http://www.slicerrt.org for details.
    """
    self.parent = parent

#
# Widget
#

class MultiDimensionExplorerWidget:

  def __init__(self, parent=None):
    if not parent:
      self.parent = slicer.qMRMLWidget()
      self.parent.setLayout(qt.QVBoxLayout())
      self.parent.setMRMLScene(slicer.mrmlScene)
    else:
      self.parent = parent
    self.logic = None
    self.layout = self.parent.layout()
    if not parent:
      self.setup()
      self.parent.show()
    
    self.logic = slicer.modules.multidimension.logic()

  def setup(self):
    # Instantiate and connect widgets ...

    #
    # Reload and Test area
    #
    reloadCollapsibleButton = ctk.ctkCollapsibleButton()
    reloadCollapsibleButton.text = "Reload && Test"
    self.layout.addWidget(reloadCollapsibleButton)
    reloadFormLayout = qt.QFormLayout(reloadCollapsibleButton)

    # reload button
    # (use this during development, but remove it when delivering
    #  your module to users)
    self.reloadButton = qt.QPushButton("Reload")
    self.reloadButton.toolTip = "Reload this module."
    self.reloadButton.name = "MultiDimensionExplorer Reload"
    reloadFormLayout.addWidget(self.reloadButton)
    self.reloadButton.connect('clicked()', self.onReload)

    # reload and test button
    # (use this during development, but remove it when delivering
    #  your module to users)
    self.reloadAndTestButton = qt.QPushButton("Reload and Test")
    self.reloadAndTestButton.toolTip = "Reload this module and then run the self tests."
    reloadFormLayout.addWidget(self.reloadAndTestButton)
    self.reloadAndTestButton.connect('clicked()', self.onReloadAndTest)


    # Collapsible button
    self.inputCollapsibleButton = ctk.ctkCollapsibleButton()
    self.inputCollapsibleButton.text = "Inputs"
    self.layout.addWidget(self.inputCollapsibleButton)

    # Layout within the collapsible button
    self.inputFormLayout = qt.QFormLayout(self.inputCollapsibleButton)

    # Dose Volume selector
    self.mdSelectorLabel = qt.QLabel('Input Multidimension Node:')
    self.mdSelector = slicer.qMRMLNodeComboBox()
    self.mdSelector.nodeTypes = ['vtkMRMLHierarchyNode']
    self.mdSelector.addAttribute('vtkMRMLHierarchyNode','MultiDimension.Name')
    self.mdSelector.noneEnabled = False
    self.mdSelector.addEnabled = False
    self.mdSelector.removeEnabled = False
    self.mdSelector.setMRMLScene( slicer.mrmlScene )
    self.mdSelector.setToolTip( "Set the input volume" )
    
    self.mdSelector.connect('mrmlSceneChanged(vtkMRMLScene*)', self.onVCMRMLSceneChanged)
    self.mdSelector.connect('currentNodeChanged(vtkMRMLNode*)', self.onInputMDChanged)

    self.inputFormLayout.addRow(self.mdSelectorLabel, self.mdSelector)

    # TODO: initialize the slider based on the contents of the labels array
    # slider to scroll over metadata stored in the vector container being explored
    self.mdSlider = ctk.ctkSliderWidget()
    label = qt.QLabel('Current frame number')

    self.inputFormLayout.addRow(label, self.mdSlider)
    
    self.mdSlider.connect('valueChanged(double)', self.onSliderChanged)

    # Add vertical spacer
    self.layout.addStretch(1)

  def onVCMRMLSceneChanged(self, mrmlScene):
    self.mdSelector.setMRMLScene(slicer.mrmlScene)
    self.onInputChanged()

  def onInputMDChanged(self):
    slicer.app.processEvents()
    self.mdNode = self.mdSelector.currentNode()
    print "inside"
    print self.mdNode.GetID()
    if self.mdNode != None:
      nFrames = self.logic.GetNumberOfChildrenNodes(self.mdNode)
      print "nframe", nFrames
      self.mdSlider.minimum = 0
      self.mdSlider.maximum = nFrames-1
      
  def onSliderChanged(self, newValue):
    slicer.app.processEvents()
    value = self.mdSlider.value
    print "value", value
    self.mdNode = self.mdSelector.currentNode()
    print self.mdNode.GetID()

    #if self.mdNode == None:
    #  return
    
    #self.mdNode = self.logic.SetMultiDimensionRootNode(self.mdNode)
    #tempNode = self.logic.FindChildNodeAtTimePoint(self.mdNode, str(int(value)))
    #tempVolumeNode = tempNode.GetAssociatedNode()
    
    #if tempVolumeNode == None:
    #  return

    # make the temp volume appear in all the slice views
    #selectionNode = slicer.app.applicationLogic().GetSelectionNode()
    #selectionNode.SetReferenceActiveVolumeID(tempVolumeNode.GetID())
    #slicer.app.applicationLogic().PropagateVolumeSelection(0)
    
    childNode = self.mdNode.GetNthChildNode( int( value ) )
    timePoint = childNode.GetAttribute( "MultiDimension.Value" )
    logic = MultiDimensionExplorerLogic()
    logic.DisplayNodes( self.logic.GetChildNodesAtTimePoint( self.mdNode,timePoint ) )
    
  def onReload(self,moduleName="MultiDimensionExplorer"):
    """Generic reload method for any scripted module.
    ModuleWizard will subsitute correct default moduleName.
    """
    import imp, sys, os, slicer

    widgetName = moduleName + "Widget"

    # reload the source code
    # - set source file path
    # - load the module to the global space
    filePath = eval('slicer.modules.%s.path' % moduleName.lower())
    p = os.path.dirname(filePath)
    if not sys.path.__contains__(p):
      sys.path.insert(0,p)
    fp = open(filePath, "r")
    globals()[moduleName] = imp.load_module(
        moduleName, fp, filePath, ('.py', 'r', imp.PY_SOURCE))
    fp.close()

    # rebuild the widget
    # - find and hide the existing widget
    # - create a new widget in the existing parent
    parent = slicer.util.findChildren(name='%s Reload' % moduleName)[0].parent().parent()
    for child in parent.children():
      try:
        child.hide()
      except AttributeError:
        pass
    # Remove spacer items
    item = parent.layout().itemAt(0)
    while item:
      parent.layout().removeItem(item)
      item = parent.layout().itemAt(0)
    # create new widget inside existing parent
    globals()[widgetName.lower()] = eval(
        'globals()["%s"].%s(parent)' % (moduleName, widgetName))
    globals()[widgetName.lower()].setup()

  def onReloadAndTest(self,moduleName="MultiDimensionExplorer"):
    try:
      self.onReload()
      evalString = 'globals()["%s"].%sTest()' % (moduleName, moduleName)
      tester = eval(evalString)
      tester.runTest()
    except Exception, e:
      import traceback
      traceback.print_exc()
      qt.QMessageBox.warning(slicer.util.mainWindow(), 
          "Reload and Test", 'Exception!\n\n' + str(e) + "\n\nSee Python Console for Stack Trace")


#
# Logic
#

class MultiDimensionExplorerLogic:
  """Implement the logic to create MultiDimension Node.
  Nodes are passed in as arguments.
  """
  def __init__(self):
    pass
    
  def DisplayNodes( self, nodeCollection ):
  
    # Set temporary nodes to have the selected slider values
    print nodeCollection.GetNumberOfItems()
    for i in range( 0, nodeCollection.GetNumberOfItems() ):
      selectedNode = nodeCollection.GetItemAsObject( i )
      node = slicer.mrmlScene.GetFirstNodeByName( "Virtual_" + selectedNode.GetName() )
      
      # Create a new virtual node if one doesn't already exist
      if ( node == None ):
        node = slicer.mrmlScene.CreateNodeByClass( selectedNode.GetClassName() )
      
      node.Copy( selectedNode )
      node.SetName( "Virtual_" + selectedNode.GetName() )
      print node.GetName()
      slicer.mrmlScene.AddNode( node )
      node.SetScene( slicer.mrmlScene )
      


