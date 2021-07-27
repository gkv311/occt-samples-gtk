cmake_minimum_required(VERSION 2.8)

project(occt-gtkmm-sample)

set(APP_VERSION_MAJOR 1)
set(APP_VERSION_MINOR 0)
set(APP_TARGET occt-gtkmm-sample)
set(CMAKE_CXX_STANDARD 11)

find_package(OpenGL    REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)

add_executable(${APP_TARGET}
  main.cpp
  OcctGtkViewer.hxx
  OcctGtkViewer.cxx)

target_include_directories (${APP_TARGET} PRIVATE ${GTKMM_INCLUDE_DIRS})
link_directories (${GTKMM_LIBRARY_DIRS})
target_link_libraries (${APP_TARGET} PRIVATE ${GTKMM_LIBRARIES} ${OPENGL_LIBRARIES})
target_link_libraries (${APP_TARGET} PRIVATE "epoxy")
