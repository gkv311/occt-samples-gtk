// Copyright (c) 2025 Kirill Gavrilov

#ifndef _OcctGtkTools_HeaderFile
#define _OcctGtkTools_HeaderFile

// workaround macros clash with Gtk::Collation::UNICODE
// (UNICODE is a standard macros used by WinAPI)
#ifdef UNICODE
#undef UNICODE
#endif
#include <gtkmm.h>

#include <Aspect_WindowInputListener.hxx>
#include <Message_Gravity.hxx>
#include <Quantity_ColorRGBA.hxx>

class OpenGl_Caps;
class V3d_View;

//! Auxiliary tools between GTK and OCCT definitions.
class OcctGtkTools
{
public: //! @name GTK application pre-setup for OCCT 3D Viewer integration

  //! Perform global GTK platform setup - to be called before Gtk::Application creation.
  //! Defines platform plugin to load (e.g. xcb on Linux)
  //! and graphic driver (e.g. desktop OpenGL with desired profile/surface).
  //!
  //! Environment variables GDK_BACKEND, GDK_DISABLE, GDK_DEBUG, GTK_CSD are used,
  //! as GTK intentionally doesn't provide C API for these.
  static void gtkGlPlatformSetup();

public: //! @name methods for wrapping GTK input events into Aspect_WindowInputListener events

  //! Map GTK event mouse button to Aspect_VKeyMouse.
  static Aspect_VKeyMouse gtkMouseButton2VKey(guint theButton);

  //! Map GTK key to virtual key.
  static Aspect_VKey gtkKey2VKey(guint theKeyVal, guint theKeyCode);

#if (GTK_MAJOR_VERSION >= 4)
  //! Map GTK modifiers to Aspect_VKeyFlags.
  static Aspect_VKeyFlags gtkModifiers2VKeys(Gdk::ModifierType theType);

  //! Queue GTK mouse motion event to OCCT listener.
  static bool gtkHandleMotionEvent(Aspect_WindowInputListener& theListener,
                                   const Handle(V3d_View)& theView,
                                   const Graphic3d_Vec2d& thePnt,
                                   const Aspect_VKeyFlags theFlags);

  //! Queue GTK mouse button pressed/released event to OCCT listener.
  static bool gtkHandleButtonEvent(Aspect_WindowInputListener& theListener,
                                   const Handle(V3d_View)& theView,
                                   const Graphic3d_Vec2d& thePnt,
                                   const unsigned int theBtn,
                                   const Aspect_VKeyFlags theFlags,
                                   const bool theIsPressed);

  //! Queue GTK mouse wheel event to OCCT listener.
  static bool gtkHandleScrollEvent(Aspect_WindowInputListener& theListener,
                                   const Handle(V3d_View)& theView,
                                   const Graphic3d_Vec2d& theDelta);
#else
  //! Map GTK event mouse flags to Aspect_VKeyFlags.
  static Aspect_VKeyFlags gtkMouseFlags2VKeys(guint theFlags);

  //! Queue GTK mouse motion event to OCCT listener.
  static bool gtkHandleMotionEvent(Aspect_WindowInputListener& theListener,
                                   const Handle(V3d_View)& theView,
                                   const GdkEventMotion* theEvent);

  //! Queue GTK mouse button pressed/released event to OCCT listener.
  static bool gtkHandleButtonEvent(Aspect_WindowInputListener& theListener,
                                   const Handle(V3d_View)& theView,
                                   const GdkEventButton* theEvent);

  //! Queue GTK mouse wheel event to OCCT listener.
  static bool gtkHandleScrollEvent(Aspect_WindowInputListener& theListener,
                                   const Handle(V3d_View)& theView,
                                   const GdkEventScroll* theEvent);
#endif
};

#endif // _OcctGtkTools_HeaderFile
