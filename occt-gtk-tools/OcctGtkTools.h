// Copyright (c) 2025 Kirill Gavrilov

#ifndef _OcctGtkTools_HeaderFile
#define _OcctGtkTools_HeaderFile

#include <Aspect_WindowInputListener.hxx>
#include <Message_Gravity.hxx>
#include <Quantity_ColorRGBA.hxx>

#include <gtkmm.h>

class OpenGl_Caps;
class V3d_View;

//! Auxiliary tools between GTK and OCCT definitions.
class OcctGtkTools
{
public: //! @name GTK application pre-setup for OCCT 3D Viewer integration

  //! Perform global GTK platform setup - to be called before QApplication creation.
  //! Defines platform plugin to load (e.g. xcb on Linux)
  //! and graphic driver (e.g. desktop OpenGL with desired profile/surface).
  static void gtkGlPlatformSetup();

public: //! @name methods for wrapping GTK input events into Aspect_WindowInputListener events

  //! Queue GTK mouse motion event to OCCT listener.
  static bool gtkHandleMotionEvent(Aspect_WindowInputListener& theListener,
                                   const Handle(V3d_View)& theView,
                                   const GdkEventMotion* theEvent);

  //! Queue GTK mouse button pressed event to OCCT listener.
  static bool gtkHandleButtonPressedEvent(Aspect_WindowInputListener& theListener,
                                          const Handle(V3d_View)& theView,
                                          const GdkEventButton* theEvent);

  //! Queue GTK mouse button released event to OCCT listener.
  static bool gtkHandleButtonReleasedEvent(Aspect_WindowInputListener& theListener,
                                           const Handle(V3d_View)& theView,
                                           const GdkEventButton* theEvent);

  //! Queue GTK mouse wheel event to OCCT listener.
  static bool gtkHandleScrollEvent(Aspect_WindowInputListener& theListener,
                                   const Handle(V3d_View)& theView,
                                   const GdkEventScroll* theEvent);

  //! Map GTK event mouse button to Aspect_VKeyMouse.
  static Aspect_VKeyMouse gtkMouseButton2VKey(guint theButton);

  //! Map GTK event mouse flags to Aspect_VKeyFlags.
  static Aspect_VKeyFlags gtkMouseFlags2VKeys(guint theFlags);

};

#endif // _OcctGtkTools_HeaderFile
