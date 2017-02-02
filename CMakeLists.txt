cmake_minimum_required(VERSION 2.8.9)

project(Sequences)

#-----------------------------------------------------------------------------
set(EXTENSION_HOMEPAGE "http://www.slicer.org/slicerWiki/index.php/Documentation/Nightly/Extensions/Sequences")
set(EXTENSION_CATEGORY "Sequences")
set(EXTENSION_CONTRIBUTORS "Andras Lasso (PerkLab, Queen's), Matthew Holden (PerkLab, Queen's), Kevin Wang (PMH), Csaba Pinter (PerkLab, Queen's), Andriy Fedorov (BWH), Isaiah Norton (BWH)")
set(EXTENSION_DESCRIPTION "A set of modules for generic sequences (multidimensional data) management in Slicer (0.3.0)")
set(EXTENSION_ICONURL "http://www.slicer.org/slicerWiki/images/4/4d/SequencesLogo.png")
set(EXTENSION_SCREENSHOTURLS "http://www.slicer.org/slicerWiki/images/1/17/CardiacCtReplay.gif http://www.slicer.org/slicerWiki/images/0/0f/UltrasoundToolGuidance3dOnly.gif http://www.slicer.org/slicerWiki/images/e/ea/ShapeModelSequenceBrowsing3dOnly.gif http://www.slicer.org/slicerWiki/images/thumb/5/5b/SequenceEditor.png/504px-SequenceEditor.png http://www.slicer.org/slicerWiki/images/thumb/e/ef/SequenceBrowser.png/397px-SequenceBrowser.png")

#-----------------------------------------------------------------------------
find_package(Slicer REQUIRED)
include(${Slicer_USE_FILE})

#-----------------------------------------------------------------------------
OPTION(SEQUENCES_ENABLE_EXPERIMENTAL_MODULES "Enable the building of work-in-progress, experimental modules." OFF)

#-----------------------------------------------------------------------------
add_subdirectory(Sequences)
add_subdirectory(SequenceBrowser)
add_subdirectory(MetafileImporter)

if (SEQUENCES_ENABLE_EXPERIMENTAL_MODULES)
  # In development
  add_subdirectory(SequenceRegistration)
  add_subdirectory(SequenceLabelStatistics)
  add_subdirectory(SequenceResample)
  #add_subdirectory(SequenceGaussianBlurImageFilter)
endif()


#-----------------------------------------------------------------------------
include(${Slicer_EXTENSION_CPACK})

# Setup the configuration file
if(NOT Slicer_SOURCE_DIR)
  # Configure 'SlicerSequencesConfig.cmake'
  include(CMakePackageConfigHelpers)

  # For a build tree
  set(Sequences_SOURCE_DIR ${CMAKE_SOURCE_DIR})
  set(Sequences_BINARY_DIR ${CMAKE_BINARY_DIR})
  set(Sequences_LIBRARY_DIR ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
  configure_package_config_file(
      CMake/${PROJECT_NAME}Config.cmake.in
      "${CMAKE_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
      INSTALL_DESTINATION ${CMAKE_BINARY_DIR}
      PATH_VARS Sequences_SOURCE_DIR Sequences_BINARY_DIR Sequences_LIBRARY_DIR
      NO_CHECK_REQUIRED_COMPONENTS_MACRO
  )
endif()
