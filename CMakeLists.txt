cmake_minimum_required (VERSION 3.13)

# common solution for multiple independent projects
project (occt-gtk)
set_property (GLOBAL PROPERTY USE_FOLDERS ON)
add_subdirectory (occt-gtk-tools)
add_subdirectory (occt-gtk-glarea)
