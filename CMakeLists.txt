cmake_minimum_required (VERSION 2.8)

project (occt-gtkmm-sample)

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/adm/cmake" ${CMAKE_MODULE_PATH})

set (APP_VERSION_MAJOR 1)
set (APP_VERSION_MINOR 0)
set (APP_TARGET occt-gtkmm-sample)
set (CMAKE_CXX_STANDARD 11)

find_package (OpenGL    REQUIRED)
find_package (PkgConfig REQUIRED)
pkg_check_modules (GTKMM REQUIRED gtkmm-3.0)

find_package (OpenCASCADE REQUIRED)
if (NOT OpenCASCADE_FOUND)
  message (FATAL_ERROR "coult not find OpenCASCADE, please set OpenCASCADE_DIR variable" )
else()
  message (STATUS "Using OpenCASCADE from \"${OpenCASCADE_INSTALL_PREFIX}\"" )
  message (STATUS "OpenCASCADE_INCLUDE_DIR=${OpenCASCADE_INCLUDE_DIR}")
  message (STATUS "OpenCASCADE_LIBRARY_DIR=${OpenCASCADE_LIBRARY_DIR}")
endif()

# compiler flags
if (MSVC)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:precise /EHa /MP")
  string (REGEX REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  add_definitions (-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
else()
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -fPIC")
  add_definitions(-DOCC_CONVERT_SIGNALS)
endif()

add_executable (${APP_TARGET}
  main.cpp
  OcctGtkViewer.hxx
  OcctGtkViewer.cxx)

# extra search paths
include_directories(${OpenCASCADE_INCLUDE_DIR})
#link_directories   (${OpenCASCADE_LIBRARY_DIR})

include_directories(${GTKMM_INCLUDE_DIRS})
link_directories   (${GTKMM_LIBRARY_DIRS})

# define dependencies
set (anOcctLibs TKOpenGl TKXDEIGES TKIGES TKXDESTEP TKSTEP TKSTEPAttr TKSTEP209 TKSTEPBase TKXSBase TKBinXCAF TKXmlXCAF TKBin TKXml TKXmlL TKBinL TKXCAF TKVCAF TKCAF TKV3d TKMesh TKOffset TKFillet TKShHealing TKBool TKBO TKHLR TKPrim TKService TKTopAlgo TKGeomAlgo TKBRep TKGeomBase TKG3d TKG2d TKMath TKLCAF TKCDF TKernel)
target_link_libraries (${PROJECT_NAME} PRIVATE ${anOcctLibs})

target_link_libraries (${PROJECT_NAME} PRIVATE ${GTKMM_LIBRARIES} ${OPENGL_LIBRARIES})

if (UNIX)
  target_link_libraries (${PROJECT_NAME} PRIVATE EGL)
  target_link_libraries (${PROJECT_NAME} PRIVATE X11)
  target_link_libraries (${PROJECT_NAME} PRIVATE dl)
  target_link_libraries (${PROJECT_NAME} PRIVATE pthread)
endif()
