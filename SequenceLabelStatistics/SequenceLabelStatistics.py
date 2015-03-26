import os
import unittest
from __main__ import vtk, qt, ctk, slicer

#
# SequenceLabelStatistics
#

class SequenceLabelStatistics:
  def __init__(self, parent):
    import string
    parent.title = "Sequence Label Statistics"
    parent.categories = ["Sequences"]
    parent.dependencies = []
    parent.contributors = ["Kevin Wang (PMH)"] # replace with "Firstname Lastname (Org)"
    parent.helpText = """
    This is a development module for label statistics of sequence data.
    """
    parent.acknowledgementText = """
    This file was originally developed by Kevin Wang and was funded by PMH and OCAIRO.
""" # replace with organization, grant and thanks.
    self.parent = parent
    try:
      slicer.selfTests
    except AttributeError:
      slicer.selfTests = {}
    slicer.selfTests['SequenceLabelStatistics'] = self.runTest

  def runTest(self):
    tester = SequenceLabelStatisticsTest()
    tester.runTest()

#
# qSlicerPythonModuleExampleWidget
#

class SequenceLabelStatisticsWidget:
  def __init__(self, parent=None):
    settings = qt.QSettings()
    self.isDeveloperMode = settings.value('QtTesting/Enabled')
    self.chartOptions = ("Count", "Volume mm^3", "Volume cc", "Min", "Max", "Mean", "StdDev")
    if not parent:
      self.parent = slicer.qMRMLWidget()
      self.parent.setLayout(qt.QVBoxLayout())
      self.parent.setMRMLScene(slicer.mrmlScene)
    else:
      self.parent = parent
    self.layout = self.parent.layout()
    self.logic = None
    self.grayscaleNode = None
    self.labelNode = None
    self.fileName = None
    self.fileDialog = None
    if not parent:
      self.setup()
      self.grayscaleSelector.setMRMLScene(slicer.mrmlScene)
      self.labelSelector.setMRMLScene(slicer.mrmlScene)
      self.parent.show()

  def setup(self):
    # Instantiate and connect widgets ...

    w = qt.QWidget()
    layout = qt.QGridLayout()
    w.setLayout(layout)
    self.layout.addWidget(w)
    w.show()
    #self.layout = layout

    if self.isDeveloperMode:
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
      self.reloadButton.name = "SequenceLabelStatistics Reload"
      reloadFormLayout.addWidget(self.reloadButton)
      self.reloadButton.connect('clicked()', self.onReload)

      # reload and test button
      # (use this during development, but remove it when delivering
      #  your module to users)
      self.reloadAndTestButton = qt.QPushButton("Reload and Test")
      self.reloadAndTestButton.toolTip = "Reload this module and then run the self tests."
      reloadFormLayout.addWidget(self.reloadAndTestButton)
      self.reloadAndTestButton.connect('clicked()', self.onReloadAndTest)

    #
    # Parameter Area
    #
    parametersCollapsibleButton1 = ctk.ctkCollapsibleButton()
    parametersCollapsibleButton1.text = "Parameters"
    self.layout.addWidget(parametersCollapsibleButton1)

    # Layout within the dummy collapsible button
    parametersFormLayout = qt.QFormLayout(parametersCollapsibleButton1)

    #
    # the grayscale volume selector
    #
    self.grayscaleSelector = slicer.qMRMLNodeComboBox()
    self.grayscaleSelector.nodeTypes = ( ("vtkMRMLSequenceNode"), "" )
    # self.grayscaleSelector.addAttribute( "vtkMRMLSequenceNode", "LabelMap", 0 )
    self.grayscaleSelector.selectNodeUponCreation = False
    self.grayscaleSelector.addEnabled = False
    self.grayscaleSelector.removeEnabled = False
    self.grayscaleSelector.noneEnabled = False
    self.grayscaleSelector.showHidden = False
    self.grayscaleSelector.showChildNodeTypes = False
    self.grayscaleSelector.setMRMLScene( slicer.mrmlScene )
    self.grayscaleSelector.setToolTip( "Select the grayscale volume (background grayscale scalar volume node) for statistics calculations." )
    # TODO: need to add a QLabel
    # self.grayscaleSelector.SetLabelText( "Master Volume:" )
    parametersFormLayout.addRow("Grayscale Volume Sequence: ", self.grayscaleSelector)

    #
    # the label volume selector
    #
    self.labelSelector = slicer.qMRMLNodeComboBox()
    self.labelSelector.nodeTypes = ( "vtkMRMLSequenceNode", "vtkMRMLScalarVolumeNode" )
    self.labelSelector.addAttribute( "vtkMRMLScalarVolumeNode", "LabelMap", "1" )
    # todo addAttribute
    self.labelSelector.selectNodeUponCreation = False
    self.labelSelector.addEnabled = False
    self.labelSelector.noneEnabled = True
    self.labelSelector.removeEnabled = False
    self.labelSelector.showHidden = False
    self.labelSelector.showChildNodeTypes = False
    self.labelSelector.setMRMLScene( slicer.mrmlScene )
    self.labelSelector.setToolTip( "Pick the label map to edit" )
    parametersFormLayout.addRow("Labelmap/Sequence: ", self.labelSelector)

    # Apply button
    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.toolTip = "Calculate Statistics."
    self.applyButton.enabled = False
    parametersFormLayout.addRow(self.applyButton)

    # model and view for stats table
    self.view = qt.QTableView()
    self.view.sortingEnabled = True
    parametersFormLayout.addRow(self.view)

    # Chart button
    self.chartFrame = qt.QFrame()
    self.chartFrame.setLayout(qt.QHBoxLayout())
    parametersFormLayout.addRow(self.chartFrame)
    self.chartButton = qt.QPushButton("Chart")
    self.chartButton.toolTip = "Make a chart from the current statistics."
    self.chartFrame.layout().addWidget(self.chartButton)
    self.chartOption = qt.QComboBox()
    self.chartOption.addItems(self.chartOptions)
    self.chartFrame.layout().addWidget(self.chartOption)
    self.chartIgnoreZero = qt.QCheckBox()
    self.chartIgnoreZero.setText('Ignore Zero')
    self.chartIgnoreZero.checked = False
    self.chartIgnoreZero.setToolTip('Do not include the zero index in the chart to avoid dwarfing other bars')
    self.chartFrame.layout().addWidget(self.chartIgnoreZero)
    self.chartFrame.enabled = False


    # Save button
    self.saveButton = qt.QPushButton("Save")
    self.saveButton.toolTip = "Calculate Statistics."
    self.saveButton.enabled = False
    parametersFormLayout.addRow(self.saveButton)

    # Add vertical spacer
    self.layout.addStretch(1)

    # connections
    self.applyButton.connect('clicked()', self.onApply)
    self.chartButton.connect('clicked()', self.onChart)
    self.saveButton.connect('clicked()', self.onSave)
    self.grayscaleSelector.connect('currentNodeChanged(vtkMRMLNode*)', self.onGrayscaleSelect)
    self.labelSelector.connect('currentNodeChanged(vtkMRMLNode*)', self.onLabelSelect)

    # Set initial state
    self.grayscaleNode = self.grayscaleSelector.currentNode()
    self.labelNode = self.labelSelector.currentNode()
    self.applyButton.enabled = bool(self.grayscaleNode) and bool(self.labelNode)

  def onGrayscaleSelect(self, node):
    self.grayscaleNode = node
    self.applyButton.enabled = bool(self.grayscaleNode) and bool(self.labelNode)

  def onLabelSelect(self, node):
    self.labelNode = node
    self.applyButton.enabled = bool(self.grayscaleNode) and bool(self.labelNode)

  def volumesAreValid(self):
    """Verify that volumes are compatible with label calculation
    algorithm assumptions"""
    if not self.grayscaleNode or not self.labelNode:
      return False

    if not self.grayscaleNode.GetNumberOfDataNodes():
      return False
    if self.labelNode.IsA("vtkMRMLSequenceNode"):
      if not self.labelNode.GetNumberOfDataNodes():
        return False
    return True

  def onApply(self):
    """Calculate the label statistics
    """
    if not self.volumesAreValid():
      qt.QMessageBox.warning(slicer.util.mainWindow(),
          "Label Statistics", "Volumes do not have the same geometry.")
      return

    self.applyButton.text = "Working..."
    # TODO: why doesn't processEvents alone make the label text change?
    self.applyButton.repaint()
    slicer.app.processEvents()
    self.logic = SequenceLabelStatisticsLogic(self.grayscaleNode, self.labelNode)
    self.populateStats()
    self.chartFrame.enabled = True
    self.saveButton.enabled = True
    self.applyButton.text = "Apply"

  def onChart(self):
    """chart the label statistics
    """
    valueToPlot = self.chartOptions[self.chartOption.currentIndex]
    ignoreZero = self.chartIgnoreZero.checked
    self.logic.createStatsChart(self.labelNode,valueToPlot,ignoreZero)

  def onSave(self):
    """save the label statistics
    """
    if not self.fileDialog:
      self.fileDialog = qt.QFileDialog(self.parent)
      self.fileDialog.options = self.fileDialog.DontUseNativeDialog
      self.fileDialog.acceptMode = self.fileDialog.AcceptSave
      self.fileDialog.defaultSuffix = "csv"
      self.fileDialog.setNameFilter("Comma Separated Values (*.csv)")
      self.fileDialog.connect("fileSelected(QString)", self.onFileSelected)
    self.fileDialog.show()

  def onFileSelected(self,fileName):
    self.logic.saveStats(fileName)

  def populateStats(self):
    if not self.logic:
      return
    #displayNode = self.labelNode.GetNthDataNode(0).GetDisplayNode()
    #colorNode = displayNode.GetColorNode()
    #lut = colorNode.GetLookupTable()
    self.items = []
    self.model = qt.QStandardItemModel()
    self.view.setModel(self.model)
    self.view.verticalHeader().visible = False
    row = 0
    for i in self.logic.labelStats["Labels"]:
      color = qt.QColor()
      # rgb = lut.GetTableValue(i)
      color.setRgb(255,0,0)
      item = qt.QStandardItem()
      item.setData(color,1)
      # item.setToolTip(colorNode.GetColorName(i))
      self.model.setItem(row,0,item)
      self.items.append(item)
      col = 1
      for k in self.logic.keys:
        item = qt.QStandardItem()
        item.setText(str(self.logic.labelStats[i,k]))
        # item.setToolTip(colorNode.GetColorName(i))
        self.model.setItem(row,col,item)
        self.items.append(item)
        col += 1
      row += 1

    self.view.setColumnWidth(0,30)
    self.model.setHeaderData(0,1," ")
    col = 1
    for k in self.logic.keys:
      self.view.setColumnWidth(col,15*len(k))
      self.model.setHeaderData(col,1,k)
      col += 1

  def onReload(self,moduleName="SequenceLabelStatistics"):
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

  def onReloadAndTest(self,moduleName="SequenceLabelStatistics"):
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

class SequenceLabelStatisticsLogic:
  """Implement the logic to calculate label statistics.
  Nodes are passed in as arguments.
  Results are stored as 'statistics' instance variable.
  """

  def __init__(self, grayscaleNode, labelNode, fileName=None):
    #import numpy

    numOfDataNodes = grayscaleNode.GetNumberOfDataNodes()
    if labelNode.IsA("vtkMRMLSequenceNode"):
      numOfDataNodes2 = labelNode.GetNumberOfDataNodes()
      if numOfDataNodes != numOfDataNodes2:
        sys.stderr.write('Number of nodes in the grayscale volume sequence does not match the number of nodes in the labelmap volume sequence')
        return

    self.keys = ("Index", "Count", "Volume mm^3", "Volume cc", "Min", "Max", "Mean", "StdDev")
    if labelNode.IsA("vtkMRMLSequenceNode"):
      cubicMMPerVoxel = reduce(lambda x,y: x*y, labelNode.GetNthDataNode(0).GetSpacing())
    elif labelNode.IsA("vtkMRMLScalarVolumeNode"):
      cubicMMPerVoxel = reduce(lambda x,y: x*y, labelNode.GetSpacing())
    ccPerCubicMM = 0.001

    # TODO: progress and status updates
    # this->InvokeEvent(vtkSequenceLabelStatisticsLogic::StartLabelStats, (void*)"start label stats")

    self.labelStats = {}
    self.labelStats['Labels'] = []

    stataccum = vtk.vtkImageAccumulate()
    isLabelmapSequence = labelNode.IsA("vtkMRMLSequenceNode")
    if isLabelmapSequence:
      stataccumInput = labelNode.GetNthDataNode(0).GetImageData()
    else:
      stataccumInput = labelNode.GetImageData()

    if vtk.VTK_MAJOR_VERSION <= 5:
      stataccum.SetInput(stataccumInput)
    else:
      stataccum.SetInputData(stataccumInput)

    stataccum.Update()
    # lo = int(stataccum.GetMin()[0])
    hi = int(stataccum.GetMax()[0])

    # We select the highest label index for analysis
    selectedLabelIndex = hi

    for i in xrange(numOfDataNodes):

      # this->SetProgress((float)i/hi);
      # std::string event_message = "Label "; std::stringstream s; s << i; event_message.append(s.str());
      # this->InvokeEvent(vtkSequenceLabelStatisticsLogic::LabelStatsOuterLoop, (void*)event_message.c_str());

      # logic copied from slicer3 SequenceLabelStatistics
      # to create the binary volume of the label
      # //logic copied from slicer2 SequenceLabelStatistics MaskStat
      # // create the binary volume of the label
      if isLabelmapSequence:
        stataccumInput = labelNode.GetNthDataNode(0).GetImageData()
        if vtk.VTK_MAJOR_VERSION <= 5:
          stataccum.SetInput(stataccumInput)
        else:
          stataccum.SetInputData(stataccumInput)

      thresholder = vtk.vtkImageThreshold()
      if vtk.VTK_MAJOR_VERSION <= 5:
        thresholder.SetInput(stataccumInput)
      else:
        thresholder.SetInputData(stataccumInput)
      thresholder.SetInValue(1)
      thresholder.SetOutValue(0)
      thresholder.ReplaceOutOn()
      thresholder.ThresholdBetween(selectedLabelIndex,selectedLabelIndex)
      thresholder.SetOutputScalarType(grayscaleNode.GetNthDataNode(i).GetImageData().GetScalarType())
      thresholder.Update()

      # this.InvokeEvent(vtkSequenceLabelStatisticsLogic::LabelStatsInnerLoop, (void*)"0.25");

      #  use vtk's statistics class with the binary labelmap as a stencil
      stencil = vtk.vtkImageToImageStencil()
      if vtk.VTK_MAJOR_VERSION <= 5:
        stencil.SetInput(thresholder.GetOutput())
      else:
        stencil.SetInputData(thresholder.GetOutput())
      stencil.ThresholdBetween(1, 1)

      # this.InvokeEvent(vtkSequenceLabelStatisticsLogic::LabelStatsInnerLoop, (void*)"0.5")

      stat1 = vtk.vtkImageAccumulate()
      if vtk.VTK_MAJOR_VERSION <= 5:
        stat1.SetInput(grayscaleNode.GetNthDataNode(i).GetImageData())
        stat1.SetStencil(stencil.GetOutput())
      else:
        stat1.SetInputData(grayscaleNode.GetNthDataNode(i).GetImageData())
        stat1.SetStencilData(stencil.GetOutput())
      stat1.Update()

      # this.InvokeEvent(vtkSequenceLabelStatisticsLogic::LabelStatsInnerLoop, (void*)"0.75")

      if stat1.GetVoxelCount() > 0:
        # add an entry to the LabelStats list
        self.labelStats["Labels"].append(i)
        self.labelStats[i,"Index"] = i
        self.labelStats[i,"Count"] = stat1.GetVoxelCount()
        self.labelStats[i,"Volume mm^3"] = self.labelStats[i,"Count"] * cubicMMPerVoxel
        self.labelStats[i,"Volume cc"] = self.labelStats[i,"Volume mm^3"] * ccPerCubicMM
        self.labelStats[i,"Min"] = stat1.GetMin()[0]
        self.labelStats[i,"Max"] = stat1.GetMax()[0]
        self.labelStats[i,"Mean"] = stat1.GetMean()[0]
        self.labelStats[i,"StdDev"] = stat1.GetStandardDeviation()[0]

        # this.InvokeEvent(vtkSequenceLabelStatisticsLogic::LabelStatsInnerLoop, (void*)"1")

    # this.InvokeEvent(vtkSequenceLabelStatisticsLogic::EndLabelStats, (void*)"end label stats")

  def createStatsChart(self, labelNode, valueToPlot, ignoreZero=False):
    """Make a MRML chart of the current stats
    """
    layoutNodes = slicer.mrmlScene.GetNodesByClass('vtkMRMLLayoutNode')
    layoutNodes.SetReferenceCount(layoutNodes.GetReferenceCount()-1)
    layoutNodes.InitTraversal()
    layoutNode = layoutNodes.GetNextItemAsObject()
    layoutNode.SetViewArrangement(slicer.vtkMRMLLayoutNode.SlicerLayoutConventionalQuantitativeView)

    chartViewNodes = slicer.mrmlScene.GetNodesByClass('vtkMRMLChartViewNode')
    chartViewNodes.SetReferenceCount(chartViewNodes.GetReferenceCount()-1)
    chartViewNodes.InitTraversal()
    chartViewNode = chartViewNodes.GetNextItemAsObject()

    arrayNode = slicer.mrmlScene.AddNode(slicer.vtkMRMLDoubleArrayNode())
    array = arrayNode.GetArray()
    samples = len(self.labelStats["Labels"])
    tuples = samples
    if ignoreZero and self.labelStats["Labels"].__contains__(0):
      tuples -= 1
    array.SetNumberOfTuples(tuples)
    tuple = 0
    for i in xrange(samples):
        index = self.labelStats["Labels"][i]
        if not (ignoreZero and index == 0):
          array.SetComponent(tuple, 0, index+1)
          array.SetComponent(tuple, 1, self.labelStats[index,valueToPlot])
          array.SetComponent(tuple, 2, 0)
          tuple += 1

    chartNode = slicer.mrmlScene.AddNode(slicer.vtkMRMLChartNode())
    chartNode.AddArray(valueToPlot, arrayNode.GetID())

    chartViewNode.SetChartNodeID(chartNode.GetID())

    chartNode.SetProperty('default', 'title', 'Label Statistics')
    chartNode.SetProperty('default', 'xAxisLabel', 'Label')
    chartNode.SetProperty('default', 'yAxisLabel', valueToPlot)
    chartNode.SetProperty('default', 'type', 'Line');
    chartNode.SetProperty('default', 'xAxisType', 'categorical')
    chartNode.SetProperty('default', 'showLegend', 'off')

    # series level properties
    if labelNode.IsA("vtkMRMLSequenceNode"):
      if labelNode.GetNthDataNode(0).GetDisplayNode() != None and labelNode.GetNthDataNode(0).GetDisplayNode().GetColorNode() != None:
        chartNode.SetProperty(valueToPlot, 'lookupTable', labelNode.GetNthDataNode(0).GetDisplayNode().GetColorNodeID())
    else:
      chartNode.SetProperty(valueToPlot, 'lookupTable', slicer.mrmlScene.GetNodeByID('vtkMRMLColorTableNodeRed'))

  def statsAsCSV(self):
    """
    print comma separated value file with header keys in quotes
    """
    csv = ""
    header = ""
    for k in self.keys[:-1]:
      header += "\"%s\"" % k + ","
    header += "\"%s\"" % self.keys[-1] + "\n"
    csv = header
    for i in self.labelStats["Labels"]:
      line = ""
      for k in self.keys[:-1]:
        line += str(self.labelStats[i,k]) + ","
      line += str(self.labelStats[i,self.keys[-1]]) + "\n"
      csv += line
    return csv

  def saveStats(self,fileName):
    fp = open(fileName, "w")
    fp.write(self.statsAsCSV())
    fp.close()

  def hasImageData(self,volumeNode):
    """This is a dummy logic method that
    returns true if the passed in volume
    node has valid image data
    """
    if not volumeNode:
      print('no volume node')
      return False
    if volumeNode.GetImageData() == None:
      print('no image data')
      return False
    return True


class SequenceLabelStatisticsTest(unittest.TestCase):
  """
  This is the test case for your scripted module.
  """

  def delayDisplay(self,message,msec=1000):
    """This utility method displays a small dialog and waits.
    This does two things: 1) it lets the event loop catch up
    to the state of the test so that rendering and widget updates
    have all taken place before the test continues and 2) it
    shows the user/developer/tester the state of the test
    so that we'll know when it breaks.
    """
    print(message)
    self.info = qt.QDialog()
    self.infoLayout = qt.QVBoxLayout()
    self.info.setLayout(self.infoLayout)
    self.label = qt.QLabel(message,self.info)
    self.infoLayout.addWidget(self.label)
    qt.QTimer.singleShot(msec, self.info.close)
    self.info.exec_()

  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear(0)

  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()
    self.test_SequenceLabelStatisticsTest1()

  def test_SequenceLabelStatisticsTest1(self):
    """ Ideally you should have several levels of tests.  At the lowest level
    tests sould exercise the functionality of the logic with different inputs
    (both valid and invalid).  At higher levels your tests should emulate the
    way the user would interact with your code and confirm that it still works
    the way you intended.
    One of the most important features of the tests is that it should alert other
    developers when their changes will have an impact on the behavior of your
    module.  For example, if a developer removes a feature that you depend on,
    your test should break so they know that the feature is needed.
    """

    self.delayDisplay("Starting the test")
    #
    # first, get some data
    #
    import urllib
    downloads = (
        ('http://slicer.kitware.com/midas3/download?items=5767', 'FA.nrrd', slicer.util.loadVolume),
        )

    for url,name,loader in downloads:
      filePath = slicer.app.temporaryPath + '/' + name
      if not os.path.exists(filePath) or os.stat(filePath).st_size == 0:
        print('Requesting download %s from %s...\n' % (name, url))
        urllib.urlretrieve(url, filePath)
      if loader:
        print('Loading %s...\n' % (name,))
        loader(filePath)
    self.delayDisplay('Finished with download and loading\n')

    volumeNode = slicer.util.getNode(pattern="FA")
    logic = SequenceLabelStatisticsLogic()
    self.assertTrue( logic.hasImageData(volumeNode) )
    self.delayDisplay('Test passed!')
