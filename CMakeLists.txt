cmake_minimum_required (VERSION 3.13)

# common solution for multiple independent projects
project (occt-gtk)
set_property (GLOBAL PROPERTY USE_FOLDERS ON)

set (GTK_VERSION "GTK3" CACHE STRING "GTK major version to use")
set_property (CACHE GTK_VERSION PROPERTY STRINGS GTK3 GTK4 ALL)

add_subdirectory (occt-gtk-tools)
if ("${GTK_VERSION}" STREQUAL "GTK3" OR "${GTK_VERSION}" STREQUAL "ALL")
  add_subdirectory (occt-gtk3-glarea)
endif()
if ("${GTK_VERSION}" STREQUAL "GTK4" OR "${GTK_VERSION}" STREQUAL "ALL")
  add_subdirectory (occt-gtk4-glarea)
endif()
