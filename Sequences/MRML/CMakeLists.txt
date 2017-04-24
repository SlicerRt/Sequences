project(vtkSlicer${MODULE_NAME}ModuleMRML)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "VTK_SLICER_${MODULE_NAME_UPPER}_MODULE_MRML_EXPORT")

include_directories(BEFORE ${vtkTeem_INCLUDE_DIRS})

set(${KIT}_INCLUDE_DIRECTORIES
  ${Slicer_Base_INCLUDE_DIRS}
  )

set(${KIT}_SRCS
  vtkMRMLLinearTransformSequenceStorageNode.cxx
  vtkMRMLLinearTransformSequenceStorageNode.h
  vtkMRMLNodeSequencer.cxx
  vtkMRMLNodeSequencer.h
  vtkMRMLSequenceNode.cxx
  vtkMRMLSequenceNode.h
  vtkMRMLSequenceStorageNode.cxx
  vtkMRMLSequenceStorageNode.h
  vtkMRMLVolumeSequenceStorageNode.cxx
  vtkMRMLVolumeSequenceStorageNode.h
  )

set(${KIT}_TARGET_LIBRARIES
  ${ITK_LIBRARIES}
  ${MRML_LIBRARIES}
  SlicerBaseLogic
  qSlicerBaseQTCLI
  vtkSlicerMarkupsModuleMRML
  )

#-----------------------------------------------------------------------------
SlicerMacroBuildModuleMRML(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  )
  
target_include_directories(${KIT} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
export(TARGETS ${KIT} ${KIT}Python ${KIT}PythonD FILE "${KIT}Targets.cmake")
