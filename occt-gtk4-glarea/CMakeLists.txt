cmake_minimum_required (VERSION 3.13)

project (occt-gtk4-glarea)

set (CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../adm/cmake" ${CMAKE_MODULE_PATH})
set (APP_VERSION_MAJOR 1)
set (APP_VERSION_MINOR 0)

# compiler flags
set (CMAKE_CXX_STANDARD 17)
if (MSVC)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:precise /EHa /MP")
  string (REGEX REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  add_definitions (-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE -DUNICODE)
else()
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -fPIC")
  add_definitions (-DOCC_CONVERT_SIGNALS)
endif()

# increase compiler warnings level (-W3 for MSVC, -Wextra for GCC)
if (MSVC)
  if (CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string (REGEX REPLACE "/W[0-4]" "/W3" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
  endif()
elseif (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshorten-64-to-32")
  endif()
  if (BUILD_SHARED_LIBS)
    if (APPLE)
      set (CMAKE_SHARED_LINKER_FLAGS "-lm ${CMAKE_SHARED_LINKER_FLAGS}")
    elseif (NOT WIN32)
      set (CMAKE_SHARED_LINKER_FLAGS "-lm ${CMAKE_SHARED_LINKER_FLAGS}")
    endif()
  endif()
endif()

# Find dependencies
set (OpenCASCADE_DIR "" CACHE PATH "Path to Open CASCADE libraries.")
if (MSVC)
  set (3RDPARTY_DLL_DIRS "" CACHE STRING "Paths to external DLLs separated by semicolon (FreeImage, FreeType, etc.)")
endif()

# Find OpenGL
set (USE_GLES2 OFF CACHE BOOL "Use OpenGL ES instead of desktop OpenGL")
if (NOT WIN32 AND NOT APPLE)
  set (OpenGL_GL_PREFERENCE "GLVND")
endif()
if (USE_GLES2)
  add_definitions (-DHAVE_GLES2)
  #find_package (OpenGLES2 REQUIRED)
else()
  find_package (OpenGL REQUIRED)
endif()

# Find GTK
find_package (PkgConfig REQUIRED)
pkg_check_modules (GTKMM REQUIRED gtkmm-4.0)
message (STATUS "GTKMM_INCLUDE_DIRS=${GTKMM_INCLUDE_DIRS}")
message (STATUS "GTKMM_LIBRARY_DIRS=${GTKMM_LIBRARY_DIRS}")
include_directories(${GTKMM_INCLUDE_DIRS})
link_directories   (${GTKMM_LIBRARY_DIRS})

# Find Open CASCADE Technology
find_package (OpenCASCADE REQUIRED)
if (NOT OpenCASCADE_FOUND)
  message (FATAL_ERROR "could not find OpenCASCADE, please set OpenCASCADE_DIR variable" )
else()
  message (STATUS "Using OpenCASCADE from \"${OpenCASCADE_INSTALL_PREFIX}\"" )
  message (STATUS "OpenCASCADE_INCLUDE_DIR=${OpenCASCADE_INCLUDE_DIR}")
  message (STATUS "OpenCASCADE_LIBRARY_DIR=${OpenCASCADE_LIBRARY_DIR}")
  include_directories(${OpenCASCADE_INCLUDE_DIR})
  link_directories   (${OpenCASCADE_LIBRARY_DIR})
endif()
set (OpenCASCADE_LIBS TKV3d TKHLR TKMesh TKService TKShHealing TKPrim TKTopAlgo TKGeomAlgo TKBRep TKGeomBase TKG3d TKG2d TKMath TKernel)

# compiler flags
if (MSVC)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:precise /EHa /MP")
  string (REGEX REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  add_definitions (-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
else()
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions -fPIC")
  add_definitions(-DOCC_CONVERT_SIGNALS)
endif()

# main project target
add_executable (${PROJECT_NAME}
  ../occt-gtk-tools/OcctGlTools.h
  ../occt-gtk-tools/OcctGlTools.cpp
  ../occt-gtk-tools/OcctGtkTools.h
  ../occt-gtk-tools/OcctGtkTools.cpp
  main.cpp
  OcctGtkGLAreaViewer.h
  OcctGtkGLAreaViewer.cpp
  OcctGtkWindowSample.h
  OcctGtkWindowSample.cpp
)
target_link_libraries (${PROJECT_NAME} PRIVATE ${GTKMM_LIBRARIES} ${OpenCASCADE_LIBS})
if (USE_GLES2)
  target_link_libraries (${PROJECT_NAME} PRIVATE TKOpenGles GLESv2 EGL)
else()
  target_link_libraries (${PROJECT_NAME} PRIVATE TKOpenGl ${OPENGL_LIBRARIES})
endif()
if (UNIX)
  target_link_libraries (${PROJECT_NAME} PRIVATE EGL X11 dl pthread)
endif()

# install target
install (TARGETS "${PROJECT_NAME}"
         RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}"
         PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
