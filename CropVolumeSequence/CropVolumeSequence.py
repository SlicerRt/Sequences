import os
import unittest
import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
import logging

#
# CropVolumeSequence
#

class CropVolumeSequence(ScriptedLoadableModule):
  """Uses ScriptedLoadableModule base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "Crop volume sequence"
    self.parent.categories = ["Sequences"]
    self.parent.dependencies = []
    self.parent.contributors = ["Andras Lasso (PerkLab, Queen's University)"]
    self.parent.helpText = """This module can crop and resample a volume sequence to reduce its size for faster rendering and processing."""
    self.parent.helpText += self.getDefaultModuleDocumentationLink()
    self.parent.acknowledgementText = """
This file was originally developed by Andras Lasso
"""

#
# CropVolumeSequenceWidget
#

class CropVolumeSequenceWidget(ScriptedLoadableModuleWidget):
  """Uses ScriptedLoadableModuleWidget base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def setup(self):
    ScriptedLoadableModuleWidget.setup(self)

    # Instantiate and connect widgets ...

    #
    # Parameters Area
    #
    parametersCollapsibleButton = ctk.ctkCollapsibleButton()
    parametersCollapsibleButton.text = "Parameters"
    self.layout.addWidget(parametersCollapsibleButton)

    # Layout within the dummy collapsible button
    parametersFormLayout = qt.QFormLayout(parametersCollapsibleButton)

    #
    # input volume selector
    #
    self.inputSelector = slicer.qMRMLNodeComboBox()
    self.inputSelector.nodeTypes = ["vtkMRMLSequenceNode"]
    self.inputSelector.addEnabled = False
    self.inputSelector.removeEnabled = False
    self.inputSelector.noneEnabled = False
    self.inputSelector.showHidden = False
    self.inputSelector.showChildNodeTypes = False
    self.inputSelector.setMRMLScene( slicer.mrmlScene )
    self.inputSelector.setToolTip( "Pick a sequence node of volumes that will be cropped and resampled." )
    parametersFormLayout.addRow("Input volume sequence: ", self.inputSelector)

    #
    # output volume selector
    #
    self.outputSelector = slicer.qMRMLNodeComboBox()
    self.outputSelector.nodeTypes = ["vtkMRMLSequenceNode"]
    self.outputSelector.selectNodeUponCreation = True
    self.outputSelector.addEnabled = True
    self.outputSelector.removeEnabled = True
    self.outputSelector.noneEnabled = True
    self.outputSelector.noneDisplay = "(Overwrite input)"
    self.outputSelector.showHidden = False
    self.outputSelector.showChildNodeTypes = False
    self.outputSelector.setMRMLScene( slicer.mrmlScene )
    self.outputSelector.setToolTip( "Pick a sequence node where the cropped and resampled volumes will be stored." )
    parametersFormLayout.addRow("Output volume sequence: ", self.outputSelector)

    #
    # Crop parameters selector
    #
    self.cropParametersSelector = slicer.qMRMLNodeComboBox()
    self.cropParametersSelector.nodeTypes = ["vtkMRMLCropVolumeParametersNode"]
    self.cropParametersSelector.selectNodeUponCreation = True
    self.cropParametersSelector.addEnabled = True
    self.cropParametersSelector.removeEnabled = True
    self.cropParametersSelector.renameEnabled = True
    self.cropParametersSelector.noneEnabled = False
    self.cropParametersSelector.showHidden = True
    self.cropParametersSelector.showChildNodeTypes = False
    self.cropParametersSelector.setMRMLScene( slicer.mrmlScene )
    self.cropParametersSelector.setToolTip("Select a crop volumes parameters.")

    self.editCropParametersButton = qt.QPushButton()
    self.editCropParametersButton.setIcon(qt.QIcon(':Icons/Go.png'))
    #self.editCropParametersButton.setMaximumWidth(60)
    self.editCropParametersButton.enabled = True
    self.editCropParametersButton.toolTip = "Go to Crop Volume module to edit cropping parameters."
    hbox = qt.QHBoxLayout()
    hbox.addWidget(self.cropParametersSelector)
    hbox.addWidget(self.editCropParametersButton)
    parametersFormLayout.addRow("Crop volume settings: ", hbox)

    #
    # Apply Button
    #
    self.applyButton = qt.QPushButton("Apply")
    self.applyButton.toolTip = "Run the algorithm."
    self.applyButton.enabled = False
    parametersFormLayout.addRow(self.applyButton)

    # connections
    self.applyButton.connect('clicked(bool)', self.onApplyButton)
    self.inputSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.onSelect)
    self.cropParametersSelector.connect("currentNodeChanged(vtkMRMLNode*)", self.onSelect)
    self.editCropParametersButton.connect("clicked()", self.onEditCropParameters)

    # Add vertical spacer
    self.layout.addStretch(1)

    # Refresh Apply button state
    self.onSelect()

  def cleanup(self):
    pass

  def onSelect(self):
    self.applyButton.enabled = (self.inputSelector.currentNode() and self.cropParametersSelector.currentNode())

  def onEditCropParameters(self):
    if not self.cropParametersSelector.currentNode():
      cropParametersNode = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLCropVolumeParametersNode")
      self.cropParametersSelector.setCurrentNode(cropParametersNode)
    if self.inputSelector.currentNode():
      inputVolSeq = self.inputSelector.currentNode()
      seqBrowser = slicer.modules.sequencebrowser.logic().GetFirstBrowserNodeForSequenceNode(inputVolSeq)
      inputVolume = seqBrowser.GetProxyNode(inputVolSeq)
      if inputVolume:
        self.cropParametersSelector.currentNode().SetInputVolumeNodeID(inputVolume.GetID())
    slicer.app.openNodeModule(self.cropParametersSelector.currentNode())

  def onApplyButton(self):
    logic = CropVolumeSequenceLogic()
    logic.run(self.inputSelector.currentNode(), self.outputSelector.currentNode(), self.cropParametersSelector.currentNode())

#
# CropVolumeSequenceLogic
#

class CropVolumeSequenceLogic(ScriptedLoadableModuleLogic):
  """This class should implement all the actual
  computation done by your module.  The interface
  should be such that other python code can import
  this class and make use of the functionality without
  requiring an instance of the Widget.
  Uses ScriptedLoadableModuleLogic base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def run(self, inputVolSeq, outputVolSeq, cropParameters):
    """
    Run the actual algorithm
    """

    logging.info('Processing started')

    seqBrowser = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLSequenceBrowserNode")
    seqBrowser.SetAndObserveMasterSequenceNodeID(inputVolSeq.GetID())
    seqBrowser.SetSaveChanges(inputVolSeq, True) # allow modifying node in the sequence

    seqBrowser.SetSelectedItemNumber(0)
    slicer.modules.sequencebrowser.logic().UpdateAllProxyNodes()
    slicer.app.processEvents()
    inputVolume = seqBrowser.GetProxyNode(inputVolSeq)

    cropParameters.SetInputVolumeNodeID(inputVolume.GetID())

    if outputVolSeq == inputVolSeq:
      outputVolSeq = None
    
    if outputVolSeq:
      # Initialize output sequence
      outputVolSeq.RemoveAllDataNodes()
      outputVolSeq.SetIndexType(inputVolSeq.GetIndexType())
      outputVolSeq.SetIndexName(inputVolSeq.GetIndexName())
      outputVolSeq.SetIndexUnit(inputVolSeq.GetIndexUnit())
      outputVolume = slicer.mrmlScene.AddNewNodeByClass(inputVolume.GetClassName())
      cropParameters.SetOutputVolumeNodeID(outputVolume.GetID())
    else:
      outputVolume = None
      cropParameters.SetOutputVolumeNodeID(inputVolume.GetID())
     
    try:
      qt.QApplication.setOverrideCursor(qt.Qt.WaitCursor)
      numberOfDataNodes = inputVolSeq.GetNumberOfDataNodes()
      for seqItemNumber in range(numberOfDataNodes):
        slicer.app.processEvents(qt.QEventLoop.ExcludeUserInputEvents)
        seqBrowser.SetSelectedItemNumber(seqItemNumber)
        slicer.modules.sequencebrowser.logic().UpdateProxyNodesFromSequences(seqBrowser)
        slicer.modules.cropvolume.logic().Apply(cropParameters)
        if outputVolSeq:
          # Saved cropped result
          outputVolSeq.SetDataNodeAtValue(outputVolume, inputVolSeq.GetNthIndexValue(seqItemNumber))

    finally:
      qt.QApplication.restoreOverrideCursor()

      # Temporary result node
      if outputVolume:
        slicer.mrmlScene.RemoveNode(outputVolume)
      # Temporary input browser node
      slicer.mrmlScene.RemoveNode(seqBrowser)
      # Temporary input volume proxy node
      slicer.mrmlScene.RemoveNode(inputVolume)

      # Move output sequence node in the same browser node as the input volume sequence
      # if not in a sequence browser node already.
      if outputVolSeq and (slicer.modules.sequencebrowser.logic().GetFirstBrowserNodeForSequenceNode(outputVolSeq) is None):

        # Add output sequence to a sequence browser
        seqBrowser = slicer.modules.sequencebrowser.logic().GetFirstBrowserNodeForSequenceNode(inputVolSeq)
        if seqBrowser:
          seqBrowser.AddSynchronizedSequenceNode(outputVolSeq)
        else:
          seqBrowser = slicer.mrmlScene.AddNewNodeByClass("vtkMRMLSequenceBrowserNode")
          seqBrowser.SetAndObserveMasterSequenceNodeID(outputVolSeq.GetID())          
        seqBrowser.SetOverwriteProxyName(outputVolSeq, True)

        # Show output in slice views
        slicer.modules.sequencebrowser.logic().UpdateAllProxyNodes()
        slicer.app.processEvents()
        outputVolume = seqBrowser.GetProxyNode(outputVolSeq)
        slicer.util.setSliceViewerLayers(background=outputVolume)
      
      elif not outputVolSeq:
        # Refresh proxy node
        seqBrowser = slicer.modules.sequencebrowser.logic().GetFirstBrowserNodeForSequenceNode(inputVolSeq)
        slicer.modules.sequencebrowser.logic().UpdateProxyNodesFromSequences(seqBrowser)

    logging.info('Processing completed')

class CropVolumeSequenceTest(ScriptedLoadableModuleTest):
  """
  This is the test case for your scripted module.
  Uses ScriptedLoadableModuleTest base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def setUp(self):
    """ Do whatever is needed to reset the state - typically a scene clear will be enough.
    """
    slicer.mrmlScene.Clear(0)

  def runTest(self):
    """Run as few or as many tests as needed here.
    """
    self.setUp()
    self.test_CropVolumeSequence1()

  def test_CropVolumeSequence1(self):
    """ Ideally you should have several levels of tests.  At the lowest level
    tests should exercise the functionality of the logic with different inputs
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
        logging.info('Requesting download %s from %s...\n' % (name, url))
        urllib.urlretrieve(url, filePath)
      if loader:
        logging.info('Loading %s...' % (name,))
        loader(filePath)
    self.delayDisplay('Finished with download and loading')

    volumeNode = slicer.util.getNode(pattern="FA")
    logic = CropVolumeSequenceLogic()
    self.assertIsNotNone( logic.hasImageData(volumeNode) )
    self.delayDisplay('Test passed!')
