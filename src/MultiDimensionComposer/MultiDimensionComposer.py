import os
import unittest
import string
from __main__ import vtk, qt, ctk, slicer

#
# MultiDimension Composer module
#

class MultiDimensionComposer:
  def __init__(self, parent):
    parent.title = "MultiDimension Composer"
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

class MultiDimensionComposerWidget:

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
    self.reloadButton.name = "MultiDimensionComposer Reload"
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
    self.inputVolumeSelectorLabel = qt.QLabel('Input Volume:')
    self.inputVolumeSelector = slicer.qMRMLNodeComboBox()
    self.inputVolumeSelector.nodeTypes = ['vtkMRMLScalarVolumeNode']
    self.inputVolumeSelector.noneEnabled = False
    self.inputVolumeSelector.addEnabled = False
    self.inputVolumeSelector.removeEnabled = False
    self.inputVolumeSelector.setMRMLScene( slicer.mrmlScene )
    self.inputVolumeSelector.setToolTip( "Set the input volume" )
    self.inputFormLayout.addRow(self.inputVolumeSelectorLabel, self.inputVolumeSelector)

    # Add button
    self.addButton = qt.QPushButton("Add")
    self.addButton.toolTip = "Add volume as a frame in the MultiDimension."
    self.inputFormLayout.addWidget(self.addButton)
    self.addButton.connect('clicked(bool)', self.onAdd)

    # Remove button
    self.removeButton = qt.QPushButton("Remove")
    self.removeButton.toolTip = "Remove volume as a frame in the MultiDimension."
    self.inputFormLayout.addWidget(self.removeButton)
    self.removeButton.connect('clicked(bool)', self.onRemove)
    
    self.inputFormLayout.addRow(self.addButton, self.removeButton)
    
    self.volumeTableWidget = qt.QTableWidget(self.parent)
    self.items = []
    self.inputFormLayout.addWidget(self.volumeTableWidget)
    self.inputFormLayout.addRow(self.volumeTableWidget)
    self.volumeTableWidget.setRowCount(0)
    self.volumeTableWidget.setColumnCount(1)
    self.volumeTableWidget.setHorizontalHeaderLabels(['name'])
    self.volumeTableWidget.setColumnWidth(0,200)

   # Collapsible button
    self.outputCollapsibleButton = ctk.ctkCollapsibleButton()
    self.outputCollapsibleButton.text = "Output"
    self.layout.addWidget(self.outputCollapsibleButton)

    # Layout within the collapsible button
    self.outputFormLayout = qt.QFormLayout(self.outputCollapsibleButton)

    # Dose Volume selector
    self.mdSelectorLabel = qt.QLabel('MultiDimension :')
    self.mdSelector = slicer.qMRMLNodeComboBox()
    self.mdSelector.nodeTypes = ['vtkMRMLHierarchyNode']
    self.mdSelector.addAttribute('vtkMRMLHierarchyNode','MultiDimension.Name')
    self.mdSelector.noneEnabled = False
    self.mdSelector.addEnabled = True
    self.mdSelector.removeEnabled = False
    self.mdSelector.renameEnabled = True
    self.mdSelector.selectNodeUponCreation = True
    self.mdSelector.setMRMLScene( slicer.mrmlScene )
    self.mdSelector.setToolTip( 'Set the MultiDimension' )
    self.outputFormLayout.addRow(self.mdSelectorLabel, self.mdSelector)

    # Apply button
    self.createButton = qt.QPushButton("Create")
    self.createButton.toolTip = "Create a mulitvolume from a set of scalar volumes."
    self.layout.addWidget(self.createButton)
    self.createButton.connect('clicked(bool)', self.onCreate)

    # Add vertical spacer
    self.layout.addStretch(1)

    self.inputVolumeNodes = []
    self.inputVolumeNodesIDList = []

  def onAdd(self):
    #slicer.app.processEvents()
    inputVolumeNode = self.inputVolumeSelector.currentNode()
    inputVolumePair = [inputVolumeNode.GetName(), inputVolumeNode]
    self.inputVolumeNodes.append(inputVolumePair)
    
    numberOfInputVolumeNodes = len(self.inputVolumeNodes)
    self.volumeTableWidget.clearContents()
    self.volumeTableWidget.setRowCount(numberOfInputVolumeNodes)
    for i in range(numberOfInputVolumeNodes):
      print "item:", self.inputVolumeNodes[i][0]
      item = qt.QTableWidgetItem()
      item.setText(self.inputVolumeNodes[i][0])
      self.volumeTableWidget.setItem(i, 0, item )
      self.items.append(item)

  def onRemove(self):
    pass

  def onCreate(self):
    mvNode = self.mdSelector.currentNode()
    if mvNode == None:
      return

    slicer.app.processEvents()
    self.logic = MultiDimensionComposerLogic()
    self.logic.createMultiDimension(self.inputVolumeNodes, self.mdSelector.currentNode())

  def onReload(self,moduleName="MultiDimensionComposer"):
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

  def onReloadAndTest(self,moduleName="MultiDimensionComposer"):
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

class MultiDimensionComposerLogic:
  """Implement the logic to create MultiDimension Node.
  Nodes are passed in as arguments.
  """
  def __init__(self):
    pass

  def createMultiDimension(self, inputVolumeNodes, mvNode):
    nFrames = len(inputVolumeNodes)
    multiDimensionLogic = slicer.modules.multidimension.logic()
    rootNode = multiDimensionLogic.SetMultiDimensionRootNode(mvNode)
    for frameId in range(0,nFrames):
      childNode = inputVolumeNodes[frameId][1]
      multiDimensionLogic.AddChildNodeAtTimePoint(rootNode, childNode, str(frameId))

