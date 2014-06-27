import os
import unittest
from __main__ import vtk, qt, ctk, slicer

#
# SequenceRegistration
#

class SequenceRegistration:
  def __init__(self, parent):
    parent.title = "Sequence Registration" 
    parent.categories = ["Multidimensional data"]
    parent.dependencies = []
    parent.contributors = ["Kevin Wang (PMH)"] # replace with "Firstname Lastname (Org)"
    parent.helpText = """
    This is a development module for dynamic object tracking.
    """
    parent.acknowledgementText = """
    This file was originally developed by Kevin Wang and was funded by PMH and OCAIRO.
""" # replace with organization, grant and thanks.
    self.parent = parent
    try:
      slicer.selfTests
    except AttributeError:
      slicer.selfTests = {}
    slicer.selfTests['SequenceRegistration'] = self.runTest

  def runTest(self):
    tester = SequenceRegistrationTest()
    tester.runTest()

class SequenceRegistrationWidget:
  def __init__(self, parent = None):
    if not parent:
      self.parent = slicer.qMRMLWidget()
      self.parent.setLayout(qt.QVBoxLayout())
      self.parent.setMRMLScene(slicer.mrmlScene)
    else:
      self.parent = parent
    self.layout = self.parent.layout()
    if not parent:
      self.setup()
      self.parent.show()
    
    self.maskVolumeNode = None
    self.InputReady = None
    self.OutputReady = None

  def setup(self):
    # Instantiate and connect widgets ...

    w = qt.QWidget()
    layout = qt.QGridLayout()
    w.setLayout(layout)
    self.layout.addWidget(w)
    w.show()
    #self.layout = layout

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
    self.reloadButton.name = "SequenceRegistration Reload"
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
    # Input Area
    #
    parametersCollapsibleButton1 = ctk.ctkCollapsibleButton()
    parametersCollapsibleButton1.text = "Inputs"
    self.layout.addWidget(parametersCollapsibleButton1)

    # Layout within the dummy collapsible button
    parametersFormLayout = qt.QFormLayout(parametersCollapsibleButton1)

    #
    # Input fixed image
    #
    self.fixedImageSelector = slicer.qMRMLNodeComboBox()
    self.fixedImageSelector.nodeTypes = ( ("vtkMRMLScalarVolumeNode"), "" )
    self.fixedImageSelector.addAttribute( "vtkMRMLScalarVolumeNode", "LabelMap", 0 )
    self.fixedImageSelector.selectNodeUponCreation = True
    self.fixedImageSelector.addEnabled = False
    self.fixedImageSelector.removeEnabled = False
    self.fixedImageSelector.noneEnabled = False
    self.fixedImageSelector.showHidden = False
    self.fixedImageSelector.showChildNodeTypes = False
    self.fixedImageSelector.setMRMLScene( slicer.mrmlScene )
    self.fixedImageSelector.setToolTip( "Please select the fixed image node for tracking." )
    parametersFormLayout.addRow("Reference sequence/image node: ", self.fixedImageSelector)

    #
    # Input moving sequence image
    #
    self.movingImageSequenceSelector = slicer.qMRMLNodeComboBox()
    self.movingImageSequenceSelector.nodeTypes = ( ("vtkMRMLSequenceNode"), "" )
    # self.movingImageSequenceSelector.addAttribute( "vtkMRMLScalarVolumeNode", "LabelMap",0 )
    self.movingImageSequenceSelector.selectNodeUponCreation = True
    self.movingImageSequenceSelector.addEnabled = False
    self.movingImageSequenceSelector.removeEnabled = False
    self.movingImageSequenceSelector.noneEnabled = False
    self.movingImageSequenceSelector.showHidden = False
    self.movingImageSequenceSelector.showChildNodeTypes = False
    self.movingImageSequenceSelector.setMRMLScene( slicer.mrmlScene )
    self.movingImageSequenceSelector.setToolTip( "Please select the moving sequence image node for tracking." )
    parametersFormLayout.addRow("Moving sequence/image node: ", self.movingImageSequenceSelector)

    #
    # Input rigid transform
    #
    self.linearTransformSelector = slicer.qMRMLNodeComboBox()
    self.linearTransformSelector.nodeTypes = ( ("vtkMRMLLinearTransformNode"), "" )
    self.linearTransformSelector.selectNodeUponCreation = True
    self.linearTransformSelector.addEnabled = True
    self.linearTransformSelector.removeEnabled = True
    self.linearTransformSelector.noneEnabled = True
    self.linearTransformSelector.showHidden = False
    self.linearTransformSelector.showChildNodeTypes = False
    self.linearTransformSelector.setMRMLScene( slicer.mrmlScene )
    self.linearTransformSelector.setToolTip( "Please select a transform." )
    parametersFormLayout.addRow("Initial transform node: ", self.linearTransformSelector)

    #
    # ROI
    #
    self.InputROISelector = slicer.qMRMLNodeComboBox()
    self.InputROISelector.nodeTypes = ( ("vtkMRMLAnnotationROINode"), "" )
    self.InputROISelector.selectNodeUponCreation = True
    self.InputROISelector.addEnabled = True
    self.InputROISelector.removeEnabled = True
    self.InputROISelector.noneEnabled = True
    self.InputROISelector.showHidden = False
    self.InputROISelector.showChildNodeTypes = False
    self.InputROISelector.setMRMLScene( slicer.mrmlScene )
    parametersFormLayout.addRow("ROI: ", self.InputROISelector)

    self.ROIVisibilityButton = qt.QPushButton("Off")
    self.ROIVisibilityButton.checkable = True
    self.ROIVisibilityButton.checked = False
    self.ROIVisibilityButton.enabled = False
    parametersFormLayout.addRow("ROI Visibility: ", self.ROIVisibilityButton)

    self.InitializeTransformModeOptions = ("Off","useGeometryAlign","useMomentsAlign")
    #
    #Initialization
    #
    self.groupBox = qt.QGroupBox("Initialize Transform Mode")
    self.groupBoxLayout = qt.QHBoxLayout(self.groupBox)
    parametersFormLayout.addRow(self.groupBox)

    #
    # layout selection
    #
    for initializeTransformModeOption in self.InitializeTransformModeOptions:
      initializeTransformModeButton = qt.QRadioButton(initializeTransformModeOption)
      initializeTransformModeButton.connect('clicked()', lambda itmo=initializeTransformModeOption: self.selectInitializeTransformMode(itmo))
      self.groupBoxLayout.addWidget(initializeTransformModeButton)
    #self.groupBoxLayout.addRow("Layout", layoutHolder)

    #
    # Register Button
    #
    self.registerButton = qt.QPushButton("Register")
    self.registerButton.toolTip = "Run the algorithm."
    self.registerButton.enabled = False
    parametersFormLayout.addRow(self.registerButton)

    # Connections
    self.fixedImageSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.onInputSelect)
    self.movingImageSequenceSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.onInputSelect)
    self.InputROISelector.connect("currentNodeChanged(vtkMRMLNode*)", self.onInputSelect)
    self.ROIVisibilityButton.connect('clicked(bool)', self.onROIVisible)
    self.registerButton.connect('clicked(bool)', self.onApply)
    
    #
    # Output Area
    #
    outputsCollapsibleButton1 = ctk.ctkCollapsibleButton()
    outputsCollapsibleButton1.text = "Outputs (at least one output must be specified)"
    self.layout.addWidget(outputsCollapsibleButton1)

    # Layout within the dummy collapsible button
    outputsFormLayout = qt.QFormLayout(outputsCollapsibleButton1)

    #
    # Output rigid transform
    #
    self.outputTransformSequenceSelector = slicer.qMRMLNodeComboBox()
    self.outputTransformSequenceSelector.nodeTypes = ( ("vtkMRMLSequenceNode"), "" )
    self.outputTransformSequenceSelector.selectNodeUponCreation = True
    self.outputTransformSequenceSelector.addEnabled = True
    self.outputTransformSequenceSelector.removeEnabled = True
    self.outputTransformSequenceSelector.noneEnabled = True
    self.outputTransformSequenceSelector.showHidden = False
    self.outputTransformSequenceSelector.showChildNodeTypes = False
    self.outputTransformSequenceSelector.setMRMLScene( slicer.mrmlScene )
    self.outputTransformSequenceSelector.setToolTip( "Please select a sequence transform node." )
    outputsFormLayout.addRow("Output sequence transform node: ", self.outputTransformSequenceSelector)

    #
    # Output images
    #
    self.outputImageSequenceSelector = slicer.qMRMLNodeComboBox()
    self.outputImageSequenceSelector.nodeTypes = ( ("vtkMRMLSequenceNode"), "" )
    self.outputImageSequenceSelector.selectNodeUponCreation = True
    self.outputImageSequenceSelector.addEnabled = True
    self.outputImageSequenceSelector.removeEnabled = True
    self.outputImageSequenceSelector.noneEnabled = True
    self.outputImageSequenceSelector.showHidden = False
    self.outputImageSequenceSelector.showChildNodeTypes = False
    self.outputImageSequenceSelector.setMRMLScene( slicer.mrmlScene )
    self.outputImageSequenceSelector.setToolTip( "Please select a sequence image node." )
    outputsFormLayout.addRow("Output sequence image node: ", self.outputImageSequenceSelector)

	# Comment out the following code as this functionality is not moved to Sequence Browser module
    # plottingCollapsibleButton = ctk.ctkCollapsibleButton()
    # plottingCollapsibleButton.text = "Plotting"
    # plottingCollapsibleButton.collapsed = 0
    # self.layout.addWidget(plottingCollapsibleButton)
    
    # plottingFrameLayout = qt.QGridLayout(plottingCollapsibleButton)

    # self.ChartingDisplayOptions = ("LR","AP","SI")
    # self.chartingDisplayOption = 'LR'
    # #
    # #Initialization
    # #
    # self.chartGroupBox = qt.QGroupBox("Display options:")
    # self.chartGroupBoxLayout = qt.QHBoxLayout(self.chartGroupBox)
    # plottingFrameLayout.addWidget(self.chartGroupBox,0,0,1,3)

    # #
    # # layout selection
    # #
    # for chartingDisplayOption in self.ChartingDisplayOptions:
      # chartingDisplayOptionButton = qt.QRadioButton(chartingDisplayOption)
      # chartingDisplayOptionButton.connect('clicked()', lambda cdo=chartingDisplayOption: self.selectChartingDisplayOption(cdo))
      # self.chartGroupBoxLayout.addWidget(chartingDisplayOptionButton)
    # #self.groupBoxLayout.addRow("Layout", layoutHolder)

    # # add chart container widget
    # self.__chartView = ctk.ctkVTKChartView(w)
    # plottingFrameLayout.addWidget(self.__chartView,1,0,1,3)

    # self.__chart = self.__chartView.chart()
    # self.__chartTable = vtk.vtkTable()
    # self.__xArray = vtk.vtkFloatArray()
    # self.__yArray = vtk.vtkFloatArray()
    # self.__zArray = vtk.vtkFloatArray()
    # self.__mArray = vtk.vtkFloatArray()
    # # will crash if there is no name
    # self.__xArray.SetName('')
    # self.__yArray.SetName('signal intensity')
    # self.__zArray.SetName('')
    # self.__mArray.SetName('signal intensity')
    # self.__chartTable.AddColumn(self.__xArray)
    # self.__chartTable.AddColumn(self.__yArray)
    # self.__chartTable.AddColumn(self.__zArray)
    # self.__chartTable.AddColumn(self.__mArray)

    self.outputTransformSequenceSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.onOutputSelect)
    self.outputImageSequenceSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.onOutputSelect)

    # Add vertical spacer
    self.layout.addStretch(1)

  def onROIVisible(self):
    roi = self.InputROISelector.currentNode()
    if roi:
      visibilityButton = self.ROIVisibilityButton
      if visibilityButton.isChecked():
        roi.SetDisplayVisibility(True)
        visibilityButton.setText("On")
      else:
        roi.SetDisplayVisibility(False)
        visibilityButton.setText("Off")
        
    else:
      pass

  def onInputSelect(self):
    self.InputReady = self.fixedImageSelector.currentNode() and self.movingImageSequenceSelector.currentNode() 
    print "input ready", self.InputReady
    self.registerButton.enabled = self.InputReady and self.OutputReady
    if self.InputROISelector.currentNode():
      self.ROIVisibilityButton.enabled = True
    else:
      self.ROIVisibilityButton.enabled = False
      self.ROIVisibilityButton.checked = False
    if self.ROIVisibilityButton.isChecked():
      self.ROIVisibilityButton.setText("On")
    else:
      self.ROIVisibilityButton.setText("Off")
      
  def onOutputSelect(self):
    self.OutputReady = self.outputTransformSequenceSelector.currentNode() or self.outputImageSequenceSelector.currentNode() 
    self.registerButton.enabled = self.InputReady and self.OutputReady
    print "output ready", self.OutputReady
    # if self.outputTransformSequenceSelector.currentNode():
      # self.updateChart()
    
  def updateChart(self):
    outputTransformSequenceNode = self.outputTransformSequenceSelector.currentNode()
    
    self.__chart.RemovePlot(0)

    if outputTransformSequenceNode == None:
      return
      
    if outputTransformSequenceNode.GetDataNodeClassName() != "vtkMRMLLinearTransformNode":
      return
      
    numOfDataNodes = outputTransformSequenceNode.GetNumberOfDataNodes()
    indexName = outputTransformSequenceNode.GetIndexName()
    self.__xArray.SetNumberOfTuples(numOfDataNodes)
    self.__xArray.SetNumberOfComponents(1)
    self.__xArray.Allocate(numOfDataNodes)
    self.__xArray.SetName(indexName)
    
    self.__yArray.SetNumberOfTuples(numOfDataNodes)
    self.__yArray.SetNumberOfComponents(1)
    self.__yArray.Allocate(numOfDataNodes)
    self.__yArray.SetName('DisplacementX')

    self.__zArray.SetNumberOfTuples(numOfDataNodes)
    self.__zArray.SetNumberOfComponents(1)
    self.__zArray.Allocate(numOfDataNodes)
    self.__zArray.SetName('DisplacementY')

    self.__mArray.SetNumberOfTuples(numOfDataNodes)
    self.__mArray.SetNumberOfComponents(1)
    self.__mArray.Allocate(numOfDataNodes)
    self.__mArray.SetName('DisplacementZ')

    self.__chartTable = vtk.vtkTable()
    self.__chartTable.AddColumn(self.__xArray)
    self.__chartTable.AddColumn(self.__yArray)
    self.__chartTable.AddColumn(self.__zArray)
    self.__chartTable.AddColumn(self.__mArray)
    self.__chartTable.SetNumberOfRows(numOfDataNodes)
    
    for i in range(numOfDataNodes):
      self.__chartTable.SetValue(i, 0, float(outputTransformSequenceNode.GetNthIndexValue(i)))
      linearTransformNode = outputTransformSequenceNode.GetNthDataNode(i)
      vtkMatrix = linearTransformNode.GetTransformToParent().GetMatrix()
      self.__chartTable.SetValue(i, 1, vtkMatrix.GetElement(0,3))
      self.__chartTable.SetValue(i, 2, vtkMatrix.GetElement(1,3))
      self.__chartTable.SetValue(i, 3, vtkMatrix.GetElement(2,3))
      
    self.__chart.GetAxis(0).SetTitle('displacement(mm)')
    self.__chart.GetAxis(1).SetTitle('time(s)')
    plot = self.__chart.AddPlot(0)
    if self.chartingDisplayOption == 'LR':
      plot.SetInput(self.__chartTable, 0, 1)
    if self.chartingDisplayOption == 'AP':
      plot.SetInput(self.__chartTable, 0, 2)
    if self.chartingDisplayOption == 'SI':
      plot.SetInput(self.__chartTable, 0, 3)

  def selectChartingDisplayOption(self,chartingDisplayOption):
    """Keep track of the currently selected layout and trigger an update"""
    print chartingDisplayOption
    self.chartingDisplayOption = chartingDisplayOption
    self.updateChart()
    
  def selectInitializeTransformMode(self,initializeTransformModeOption):
    """Keep track of the currently selected layout and trigger an update"""
    print initializeTransformModeOption
    self.initializeTransformModeOption = initializeTransformModeOption
    
  def onApply(self):
    fixedVolumeNode = self.fixedImageSelector.currentNode()
    movingVolumeSequenceNode = self.movingImageSequenceSelector.currentNode()
    maskROINode = self.InputROISelector.currentNode()
    initialTransformNode = self.linearTransformSelector.currentNode()
    initializeTransformModeOption = self.initializeTransformModeOption
    
    outputTransformSequenceNode = self.outputTransformSequenceSelector.currentNode()
    outputVolumeSequenceNode = self.outputImageSequenceSelector.currentNode()

    logic = SequenceRegistrationLogic()

    if maskROINode: 
      print("create the mask from ROI")
      if self.maskVolumeNode == None:
        self.maskVolumeNode = slicer.vtkMRMLScalarVolumeNode()
        slicer.mrmlScene.AddNode(self.maskVolumeNode)
      logic.makeMaskVolumeFromROI(fixedVolumeNode, self.maskVolumeNode, maskROINode)
    else:
      slicer.mrmlScene.RemoveNode(self.maskVolumeNode)
      self.maskVolumeNode = None
    
    print("Start the registration")
    logic.RegisterImageSequence(fixedVolumeNode, movingVolumeSequenceNode, outputTransformSequenceNode, outputVolumeSequenceNode, initializeTransformModeOption, initialTransformNode, self.maskVolumeNode)
    print("finish the registration")
    
    # self.updateChart()
    
  def onReload(self,moduleName="SequenceRegistration"):
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

  def onReloadAndTest(self,moduleName="SequenceRegistration"):
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
# SequenceRegistrationLogic
#

class SequenceRegistrationLogic:
  """This class should implement all the actual 
  computation done by your module.  The interface 
  should be such that other python code can import
  this class and make use of the functionality without
  requiring an instance of the Widget
  """
  def __init__(self):
    pass

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

  def makeMaskVolumeFromROI(self, refVolumeNode, maskVolumeNode, roiNode):
    # Create a box implicit function that will be used as a stencil to fill the volume

    roiBox = vtk.vtkBox()
    roiCenter = [0, 0, 0]
    roiNode.GetXYZ( roiCenter )
    roiRadius = [0, 0, 0]
    roiNode.GetRadiusXYZ( roiRadius )
    roiBox.SetBounds(roiCenter[0] - roiRadius[0], roiCenter[0] + roiRadius[0], roiCenter[1] - roiRadius[1], roiCenter[1] + roiRadius[1], roiCenter[2] - roiRadius[2], roiCenter[2] + roiRadius[2])

    # Determine the transform between the box and the image IJK coordinate systems

    rasToBox = vtk.vtkMatrix4x4()
    if roiNode.GetTransformNodeID() != None:
      roiBoxTransformNode = slicer.mrmlScene.GetNodeByID(roiNode.GetTransformNodeID())
      boxToRas = vtk.vtkMatrix4x4()
      roiBoxTransformNode.GetMatrixTransformToWorld(boxToRas)
      rasToBox.DeepCopy(boxToRas)
      rasToBox.Invert()

    ijkToRas = vtk.vtkMatrix4x4()
    refVolumeNode.GetIJKToRASMatrix( ijkToRas )

    ijkToBox = vtk.vtkMatrix4x4()
    vtk.vtkMatrix4x4.Multiply4x4(rasToBox,ijkToRas,ijkToBox)
    ijkToBoxTransform = vtk.vtkTransform()
    ijkToBoxTransform.SetMatrix(ijkToBox)
    roiBox.SetTransform(ijkToBoxTransform)

    # Use the stencil to fill the volume

    imageData = vtk.vtkImageData()
    refImageData = refVolumeNode.GetImageData()
    imageData.SetOrigin(refImageData.GetOrigin())
    imageData.SetSpacing(refImageData.GetSpacing())
    
    if vtk.VTK_MAJOR_VERSION <= 5:
      imageData.SetExtent(refImageData.GetWholeExtent())
      imageData.SetScalarTypeToUnsignedChar()
      imageData.AllocateScalars()
    else:
      imageData.SetExtent(refImageData.GetExtent())
      imageData.AllocateScalars(vtk.VTK_UNSIGNED_CHAR,1)

    # Convert the implicit function to a stencil
    functionToStencil = vtk.vtkImplicitFunctionToImageStencil()
    functionToStencil.SetInput(roiBox)
    functionToStencil.SetOutputOrigin(refImageData.GetOrigin())
    functionToStencil.SetOutputSpacing(refImageData.GetSpacing())    
    if vtk.VTK_MAJOR_VERSION <= 5:
      functionToStencil.SetOutputWholeExtent(refImageData.GetWholeExtent())
    else:
      functionToStencil.SetOutputWholeExtent(refImageData.GetExtent())
    functionToStencil.Update()

    # Apply the stencil to the volume
    stencilToImage=vtk.vtkImageStencil()
    if vtk.VTK_MAJOR_VERSION <= 5:
      stencilToImage.SetInput(imageData)
      stencilToImage.SetStencil(functionToStencil.GetOutput())
    else:
      stencilToImage.SetInputData(imageData)    
      stencilToImage.SetStencilData(functionToStencil.GetOutput())
    stencilToImage.ReverseStencilOn()
    stencilToImage.SetBackgroundValue(1)
    stencilToImage.Update()

    # Update the volume with the stencil operation result
    maskVolumeNode.SetAndObserveImageData(stencilToImage.GetOutput())
    maskVolumeNode.CopyOrientation(refVolumeNode)
    
  def RegisterImageSequence(self, fixedVolumeNode, movingVolumeSequenceNode, linearTransformSequenceNode, outputVolumeSequenceNode, initializeTransformMode='useGeometryAlign', initialTransformNode=None, maskVolumeNode=None):
    if linearTransformSequenceNode:
      linearTransformSequenceNode.RemoveAllDataNodes()
    if outputVolumeSequenceNode:
      outputVolumeSequenceNode.RemoveAllDataNodes()
  
    numOfImageNodes = movingVolumeSequenceNode.GetNumberOfDataNodes()
    lastTransformNode = None
    for i in range(numOfImageNodes):
      movingVolumeNode = movingVolumeSequenceNode.GetNthDataNode(i)
      movingVolumeIndexValue = movingVolumeSequenceNode.GetNthIndexValue(i)
      slicer.mrmlScene.AddNode(movingVolumeNode)

      outputTransformNode = slicer.vtkMRMLLinearTransformNode()
      #outputTransformNode = slicer.vtkMRMLBSplineTransformNode()
      slicer.mrmlScene.AddNode(outputTransformNode)
    
      outputVolumeNode = None	
      if outputVolumeSequenceNode:
        outputVolumeNode = slicer.vtkMRMLScalarVolumeNode()
        slicer.mrmlScene.AddNode(outputVolumeNode)

      initialTransformNode = linearTransformSequenceNode.GetNthDataNode(i-1)
      if initialTransformNode:
        slicer.mrmlScene.AddNode(initialTransformNode)
      else:
        initialTransformNode = None

      self.RegisterImage(fixedVolumeNode, movingVolumeNode, outputTransformNode, outputVolumeNode, initializeTransformMode, initialTransformNode, maskVolumeNode)
      if linearTransformSequenceNode:
        linearTransformSequenceNode.SetDataNodeAtValue(outputTransformNode, movingVolumeIndexValue)
      if outputVolumeSequenceNode:
        outputVolumeSequenceNode.SetDataNodeAtValue(outputVolumeNode, movingVolumeIndexValue)
     
      if initialTransformNode:
        slicer.mrmlScene.RemoveNode(initialTransformNode)
      slicer.mrmlScene.RemoveNode(movingVolumeNode)
      if outputVolumeNode:
        slicer.mrmlScene.RemoveNode(outputVolumeNode)
      # Initialize the lastTransform so the next registration can start with this transform
      lastTransform = outputTransformNode
      slicer.mrmlScene.RemoveNode(outputTransformNode)
    # This is required as a temp workaround to use the transform sequence to map baseline ROI to sequence ROI
    
    if linearTransformSequenceNode:  
      for i in range(numOfImageNodes):
        transformNode = linearTransformSequenceNode.GetNthDataNode(i)
        transformNode.Inverse()
    

  def RegisterImage(self, fixedVolumeNode, movingVolumeNode, linearTransformNode, outputVolumeNode=None, initializeTransformMode='useGeometryAlign', initialTransformNode=None, maskVolumeNode=None):
    try:
      rigidReg = slicer.modules.brainsfit

      """ this is for running the legacy rigid registration cli module
      parametersRigid = {}
      parametersRigid["FixedImageFileName"] = fixedVolumeNode.GetID()
      parametersRigid["MovingImageFileName"] = movingVolumeNode.GetID()
      if maskVolumeNode:
        parametersRigid["MaskImageFileName"] = maskVolumeNode.GetID()
      parametersRigid["InitialTransform"] = linearTransformNode.GetID()
      parametersRigid["OutputTransform"] = linearTransformNode.GetID()
      if outputVolumeNode:
        parametersRigid["OutputImagefileName"] = outputTransformNode.GetID()
      parametersRigid["Iterations"] = 100
      # parametersRigid["LearningRate"] = 0.005
      """
      
      parametersRigid = {}
      parametersRigid["fixedVolume"] = fixedVolumeNode.GetID()
      parametersRigid["movingVolume"] = movingVolumeNode.GetID()
  
      if initialTransformNode:
        parametersRigid["initialTransform"] = initialTransformNode.GetID()
      parametersRigid["initializeTransformMode"] = initializeTransformMode
      
      if maskVolumeNode:
        parametersRigid["maskProcessingMode"] = 'ROI'
        parametersRigid["fixedBinaryVolume"] = maskVolumeNode.GetID()
        parametersRigid["movingBinaryVolume"] = maskVolumeNode.GetID()
      else:
        parametersRigid["maskProcessingMode"] = 'NOMASK'

      if outputVolumeNode:
        parametersRigid["outputVolume"] = outputVolumeNode.GetID()

      # parameters needed for 4D target tracking 
      # MSE seems to be working better than MMI
      # needs to inhibit the rotation for 4D tracking
      parametersRigid["costMetric"] = 'MSE'
      parametersRigid["translationScale"] = 100000.0
      parametersRigid["numberOfIterations"] = 100
      


      parametersRigid["useRigid"] = True
      parametersRigid["linearTransform"] = linearTransformNode.GetID()
      
      # # parametersRigid["useBSpline"] = True
      # # parametersRigid["bsplineTransform"] = linearTransformNode.GetID()
      # # parametersRigid["splineGridSize"] = '10,10,6'

      self.cliROIRegistrationNode = None
      self.cliROIRegistrationNode = slicer.cli.run(rigidReg, self.cliROIRegistrationNode, parametersRigid)
      waitCount = 0

      while self.cliROIRegistrationNode.GetStatusString() != 'Completed' and waitCount < 100:
        print(self.cliROIRegistrationNode.GetStatusString())
        self.delayDisplay( "Register moving image to fixed image using rigid registration... %d" % waitCount )
        waitCount += 1
      self.delayDisplay("Register gMR to rMR using rigid registration finished",1)
      
      # self.assertTrue( self.cliROIRegistrationNode.GetStatusString() == 'Completed' )
      
    except Exception, e:
      import traceback
      traceback.print_exc()
      self.delayDisplay('Registration caused exception!\n' + str(e),1)
      
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

class SequenceRegistrationTest(unittest.TestCase):
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
    self.test_SequenceRegistration1()

  def test_SequenceRegistration1(self):
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
    logic = SequenceRegistrationLogic()
    self.assertTrue( logic.hasImageData(volumeNode) )
    self.delayDisplay('Test passed!')
