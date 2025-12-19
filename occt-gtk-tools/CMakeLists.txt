cmake_minimum_required (VERSION 3.13)

# dummy project - just to collect files in folder
project (occt-gtk-tools)
add_custom_target (${PROJECT_NAME} SOURCES
  OcctGtkTools.h
  OcctGtkTools.cpp
  OcctGlTools.h
  OcctGlTools.cpp
  ../ReadMe.md
)
set_target_properties (${PROJECT_NAME} PROPERTIES FOLDER "Tools")
