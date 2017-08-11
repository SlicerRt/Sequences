import os
import unittest
import vtk, qt, ctk, slicer
from slicer.ScriptedLoadableModule import *
import logging

#
# SequenceSampleData
#

class SequenceSampleData(ScriptedLoadableModule):
  """Uses ScriptedLoadableModule base class, available at:
  https://github.com/Slicer/Slicer/blob/master/Base/Python/slicer/ScriptedLoadableModule.py
  """

  def __init__(self, parent):
    ScriptedLoadableModule.__init__(self, parent)
    self.parent.title = "Sequence Sample Data"
    self.parent.categories = ["Sequences"]
    self.parent.dependencies = ["SampleData"]
    self.parent.contributors = ["Andras Lasso (PerkLab)"]
    self.parent.helpText = """This module adds sample sequences to SampleData module"""
    self.parent.acknowledgementText = """
This work was was funded by Cancer Care Ontario
and the Ontario Consortium for Adaptive Interventions in Radiation Oncology (OCAIRO)
"""

    # don't show this module - additional data will be shown in SampleData module
    parent.hidden = True 

    # Add data sets to SampleData module
    iconsPath = os.path.join(os.path.dirname(self.parent.path), 'Resources/Icons')
    import SampleData

    SampleData.SampleDataLogic.registerCustomSampleDataSource(
      category='Sequences',
      sampleName='CTPCardio',
      uris='http://slicer.kitware.com/midas3/download/bitstream/667462/CTP-cardio.seq.nrrd',
      fileNames='CTP-cardio.seq.nrrd',
      nodeNames='CTPCardio',
      thumbnailFileName=os.path.join(iconsPath, 'CTPCardio.png'),
      loadFileType='Volume Sequence'
      )

    SampleData.SampleDataLogic.registerCustomSampleDataSource(
      category='Sequences',
      sampleName='CTCardio',
      uris='http://slicer.kitware.com/midas3/download/bitstream/675175/CT-cardio.seq.nrrd',
      fileNames='CT-cardio.seq.nrrd',
      nodeNames='CTCardio',
      thumbnailFileName=os.path.join(iconsPath, 'CTCardio.png'),
      loadFileType='Volume Sequence'
      )
