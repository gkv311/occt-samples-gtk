OCCT 3D Viewer setup within GTK+
================================

The samples in this repository demonstrate Open CASCADE Technology (OCCT) 3D Viewer setup
within GTK+ (gtkmm) GUI framework on Linux platform.

![sample screenshot](/images/occt_gtkmm_sample.png)

## Building

Use *CMake* for building the project.
See `CMakeLists.txt` in the folder defining building rules.
Use `cmake-gui` or command-line interface for configuring project dependencies GTK+ (gtkmm-3.0/gtkmm-4.0), OpenGL and Open CASCADE Technology (OCCT) libraries.
Building has been checked within development snapshot of OCCT 7.6.0 on Xubuntu 21.04.

## OCCT Gtk::GLArea sample for GTK3

Project within `occt-gtk3-glarea` subfolder shows OCCT 3D viewer setup
from *OpenGL* context created by `Gtk::GLArea` within GTK3 application.

## OCCT Gtk::GLArea sample for GTK4

Project within `occt-gtk4-glarea` subfolder shows OCCT 3D viewer setup
from *OpenGL ES* context created by `Gtk::GLArea` within GTK4 application.

## Troubleshooting

The samples relies on X11 backend (`GDK_BACKEND=x11`), so that on Wayland session it will be running through Xwayland (which is expected to be done automatically).

Some bugs related to `Gtk::GLArea` have been observed with older GTK+ versions (Xubuntu 18.04).
