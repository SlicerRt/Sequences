cmake_minimum_required(VERSION 3.5)

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
add_subdirectory(SequenceSampleData)

if (SEQUENCES_ENABLE_EXPERIMENTAL_MODULES)
  # In development
  add_subdirectory(SequenceRegistration)
  add_subdirectory(SequenceLabelStatistics)
  add_subdirectory(SequenceResample)
  #add_subdirectory(SequenceGaussianBlurImageFilter)
endif()

#-----------------------------------------------------------------------------
include(${Slicer_EXTENSION_GENERATE_CONFIG})
include(${Slicer_EXTENSION_CPACK})
