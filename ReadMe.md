OCCT GTK+ sample
==================

This sample demonstrates how to use Open CASCADE Technology (OCCT) 3D Viewer within a window created using GTK+ (gtkmm) on Linux platform.

![sample screenshot](/occt_gtkmm_sample.png)

OpenGL-based OCCT viewer is embedded into GTK+ application via `Gtk::GLArea` widget and X11 bindings.
For this reason, you may experience some bugs specific to `Gtk::GLArea` (not to OCCT) on older GTK+ versions (Xubuntu 18.04).
Compatibility with Wayland is not tested.

Use CMake for configuring the project and setting up paths to GTK+ (gtkmm-3.0/gtkmm-4.0), OpenGL and Open CASCADE Technology (OCCT) libraries.
Building has been checked within development snapshot of OCCT 7.6.0 on Xubuntu 21.04.
