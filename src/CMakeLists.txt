cmake_minimum_required(VERSION 2.8.9)

project(MultidimData)

#-----------------------------------------------------------------------------
set(EXTENSION_HOMEPAGE "http://www.slicer.org/slicerWiki/index.php/Documentation/Nightly/Extensions/Sequence")
set(EXTENSION_CATEGORY "Multidimensional data")
set(EXTENSION_CONTRIBUTORS "Andras Lasso (PerkLab, Queen's), Matthew Holden (PerkLab, Queen's), Kevin Wang (PMH)")
set(EXTENSION_DESCRIPTION "A set of modules for generic multidimensional data management in Slicer")
set(EXTENSION_ICONURL "https://subversion.assembla.com/svn/slicerrt/trunk/Sequence/src/Sequences.png")
set(EXTENSION_SCREENSHOTURLS "http://wiki.slicer.org/slicerWiki/images/4/42/Slicer-r19441-MultiDimension-screenshot.png")

#-----------------------------------------------------------------------------
find_package(Slicer REQUIRED)
include(${Slicer_USE_FILE})

#-----------------------------------------------------------------------------
add_subdirectory(Sequences)
add_subdirectory(SequenceBrowser)
add_subdirectory(MetafileImporter)

#-----------------------------------------------------------------------------
include(${Slicer_EXTENSION_CPACK})
