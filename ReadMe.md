OCCT 3D Viewer setup within GTK+
================================

The samples in this repository demonstrate Open CASCADE Technology (OCCT) 3D Viewer setup
within GTK+ (gtkmm) GUI framework on Linux and Windows platforms.

![sample screenshot](/images/occt_gtkmm_sample.png)

## Building

Use *CMake* for building the project.
See `CMakeLists.txt` in the folder defining building rules.
Use `cmake-gui` or command-line interface for configuring project dependencies GTK+ (gtkmm-3.0/gtkmm-4.0), OpenGL and Open CASCADE Technology (OCCT) libraries.
Building has been checked within development snapshot of OCCT 7.6.0 on Xubuntu 21.04.

On Windows platform, building has been checked with MinGW builds of GTK (from MSYS2 packages).

The CMake option `USE_GLES2` could be used to switch from OpenGL desktop (`TKOpenGl`)
to OpenGL ES (`TKOpenGles`) implementation, which would require OCCT built with appropriate modules enabled.

## OCCT Gtk::GLArea sample

Projects within `occt-gtk3-glarea` (GTK3) and `occt-gtk4-glarea` (GTK4) subfolders
shows OCCT 3D viewer setup from *OpenGL* context created by `Gtk::GLArea` within GTK application.

## Troubleshooting

The samples relies on X11 backend (`GDK_BACKEND=x11`) with GLX (`GDK_DISABLE=egl`),
so that on Wayland session it will be running through Xwayland (which is expected to be done automatically).

Sample defines these environment variables within `OcctGtkTools::gtkGlPlatformSetup()`
method called from `main()` before GTK application initialization,
as GTK provides no public API for managing this
(these GTK variables are provided 'for debugging purposes' and their behavior can be changed in future).

Some bugs related to `Gtk::GLArea` have been observed with older GTK+ versions (Xubuntu 18.04).
