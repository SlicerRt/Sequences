cmake_minimum_required(VERSION 2.8.9)

project(MultiDimension)

#-----------------------------------------------------------------------------
set(EXTENSION_HOMEPAGE "http://www.slicer.org/slicerWiki/index.php/Documentation/Nightly/Extensions/MultiDimension")
set(EXTENSION_CATEGORY "MultiDimension")
set(EXTENSION_CONTRIBUTORS "Matthew Holden (Queen's), Kevin Wang (PMH)")
set(EXTENSION_DESCRIPTION "This is an example of extension bundling a loadable module")
set(EXTENSION_ICONURL "http://viewvc.slicer.org/viewvc.cgi/Slicer4/trunk/Extensions/Testing/MultiDimension/MultiDimension.png?revision=21746&view=co")
set(EXTENSION_SCREENSHOTURLS "http://wiki.slicer.org/slicerWiki/images/4/42/Slicer-r19441-MultiDimension-screenshot.png")

#-----------------------------------------------------------------------------
find_package(Slicer REQUIRED)
include(${Slicer_USE_FILE})

#-----------------------------------------------------------------------------
add_subdirectory(MultiDimension)
add_subdirectory(MultiDimensionBrowser)
add_subdirectory(MetafileImporter)

#-----------------------------------------------------------------------------
include(${Slicer_EXTENSION_CPACK})
