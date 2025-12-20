// Copyright (c) 2025 Kirill Gavrilov

#include "OcctGtkTools.h"

#include <Aspect_ScrollDelta.hxx>
#include <Message.hxx>
#include <OpenGl_Caps.hxx>
#include <OSD_Environment.hxx>
#include <Standard_Version.hxx>
#include <V3d_View.hxx>

// ================================================================
// Function : gtkGlPlatformSetup
// ================================================================
void OcctGtkTools::gtkGlPlatformSetup()
{
#if defined(_WIN32)
  //
#elif defined(__APPLE__)
  //
#else
  // force X11 backend for OpenGL initialization using GLX
  // (should be done in sync with OCCT configuration)
  OSD_Environment aBackend("GDK_BACKEND");
  aBackend.SetValue("x11");
  aBackend.Build();
#endif
}

// ================================================================
// Function : gtkMouseButton2VKey
// ================================================================
Aspect_VKeyMouse OcctGtkTools::gtkMouseButton2VKey(guint theButton)
{
  switch (theButton)
  {
    case 1: return Aspect_VKeyMouse_LeftButton;
    case 2: return Aspect_VKeyMouse_MiddleButton;
    case 3: return Aspect_VKeyMouse_RightButton;
  }
  return Aspect_VKeyMouse_NONE;
}

// ================================================================
// Function : gtkKey2VKey
// ================================================================
Aspect_VKey OcctGtkTools::gtkKey2VKey(guint theKeyVal, guint )
{
  switch (theKeyVal)
  {
    case GDK_KEY_Shift_L:
    case GDK_KEY_Shift_R:
      return Aspect_VKey_Shift;
    case GDK_KEY_Control_L:
    case GDK_KEY_Control_R:
      return Aspect_VKey_Control;
    case GDK_KEY_Meta_L:
    case GDK_KEY_Meta_R:
      return Aspect_VKey_Meta;
    case GDK_KEY_Alt_L:
    case GDK_KEY_Alt_R:
      return Aspect_VKey_Alt;
  }
  return Aspect_VKey_UNKNOWN;
}

#if (GTK_MAJOR_VERSION >= 4)
// ================================================================
// Function : gtkModifiers2VKeys
// ================================================================
Aspect_VKeyFlags OcctGtkTools::gtkModifiers2VKeys(Gdk::ModifierType theType)
{
  Aspect_VKeyFlags aFlags = Aspect_VKeyFlags_NONE;
  if (int(theType & Gdk::ModifierType::SHIFT_MASK) != 0)
    aFlags |= Aspect_VKeyFlags_SHIFT;

  if (int(theType & Gdk::ModifierType::CONTROL_MASK) != 0)
    aFlags |= Aspect_VKeyFlags_CTRL;

  if (int(theType & Gdk::ModifierType::META_MASK) != 0)
    aFlags |= Aspect_VKeyFlags_META;

  if (int(theType & Gdk::ModifierType::ALT_MASK) != 0)
    aFlags |= Aspect_VKeyFlags_ALT;

  return aFlags;
}

// ================================================================
// Function : gtkHandleMotionEvent
// ================================================================
bool OcctGtkTools::gtkHandleMotionEvent(Aspect_WindowInputListener& theListener,
                                        const Handle(V3d_View)& theView,
                                        const Graphic3d_Vec2d& thePnt,
                                        const Aspect_VKeyFlags theFlags)
{
  if (theView->Window().IsNull())
    return false;

  const Graphic3d_Vec2i  aPnt2i(theView->Window()->ConvertPointToBacking(thePnt) + Graphic3d_Vec2d(0.5));
  const Aspect_VKeyMouse aButtons = theListener.PressedMouseButtons();
  return theListener.UpdateMousePosition(aPnt2i, aButtons, theFlags, false);
}

// ================================================================
// Function : gtkHandleButtonPressedEvent
// ================================================================
bool OcctGtkTools::gtkHandleButtonPressedEvent(Aspect_WindowInputListener& theListener,
                                               const Handle(V3d_View)& theView,
                                               const Graphic3d_Vec2d& thePnt,
                                               const unsigned int theBtn,
                                               const Aspect_VKeyFlags theFlags)
{
  if (theView->Window().IsNull())
    return false;

  const Aspect_VKeyMouse aButton = OcctGtkTools::gtkMouseButton2VKey(theBtn);
  if (aButton == Aspect_VKeyMouse_NONE)
    return false;

  const Graphic3d_Vec2i aPnt2i(theView->Window()->ConvertPointToBacking(thePnt) + Graphic3d_Vec2d(0.5));
  return theListener.PressMouseButton(aPnt2i, aButton, theFlags, false);
}

// ================================================================
// Function : gtkHandleButtonReleasedEvent
// ================================================================
bool OcctGtkTools::gtkHandleButtonReleasedEvent(Aspect_WindowInputListener& theListener,
                                                const Handle(V3d_View)& theView,
                                                const Graphic3d_Vec2d& thePnt,
                                                const unsigned int theBtn,
                                                const Aspect_VKeyFlags theFlags)
{
  if (theView->Window().IsNull())
    return false;

  const Aspect_VKeyMouse aButton = OcctGtkTools::gtkMouseButton2VKey(theBtn);
  if (aButton == Aspect_VKeyMouse_NONE)
    return false;

  const Graphic3d_Vec2i aPnt2i(theView->Window()->ConvertPointToBacking(thePnt) + Graphic3d_Vec2d(0.5));
  return theListener.ReleaseMouseButton(aPnt2i, aButton, theFlags, false);
}

// ================================================================
// Function : gtkHandleScrollEvent
// ================================================================
bool OcctGtkTools::gtkHandleScrollEvent(Aspect_WindowInputListener& theListener,
                                        const Handle(V3d_View)& theView,
                                        const Graphic3d_Vec2d& theDelta)
{
  if (theView->Window().IsNull())
    return false;

  const Graphic3d_Vec2i aPnt2i = theListener.LastMousePosition();
  return theListener.UpdateMouseScroll(Aspect_ScrollDelta(aPnt2i, -theDelta.y()));
}
#else
// ================================================================
// Function : gtkMouseFlags2VKeys
// ================================================================
Aspect_VKeyFlags OcctGtkTools::gtkMouseFlags2VKeys(guint theFlags)
{
  Aspect_VKeyFlags aFlags = Aspect_VKeyFlags_NONE;
  if ((theFlags & GDK_SHIFT_MASK) != 0)
    aFlags |= Aspect_VKeyFlags_SHIFT;

  if ((theFlags & GDK_CONTROL_MASK) != 0)
    aFlags |= Aspect_VKeyFlags_CTRL;

  if ((theFlags & GDK_META_MASK) != 0)
    aFlags |= Aspect_VKeyFlags_META;

#if (GTK_MAJOR_VERSION >= 4)
  if ((theFlags & GDK_ALT_MASK) != 0)
    aFlags |= Aspect_VKeyFlags_ALT;
#else
  if ((theFlags & GDK_MOD1_MASK) != 0)
    aFlags |= Aspect_VKeyFlags_ALT;
#endif
  return aFlags;
}

// ================================================================
// Function : gtkHandleMotionEvent
// ================================================================
bool OcctGtkTools::gtkHandleMotionEvent(Aspect_WindowInputListener& theListener,
                                        const Handle(V3d_View)& theView,
                                        const GdkEventMotion* theEvent)
{
  if (theView->Window().IsNull())
    return false;

  const Graphic3d_Vec2d  aPnt2d(theEvent->x, theEvent->y);
  const Graphic3d_Vec2i  aPnt2i(theView->Window()->ConvertPointToBacking(aPnt2d) + Graphic3d_Vec2d(0.5));
  const Aspect_VKeyMouse aButtons = theListener.PressedMouseButtons();
  const Aspect_VKeyFlags aFlags   = OcctGtkTools::gtkMouseFlags2VKeys(theEvent->state);
  return theListener.UpdateMousePosition(aPnt2i, aButtons, aFlags, false);
}

// ================================================================
// Function : gtkHandleButtonPressedEvent
// ================================================================
bool OcctGtkTools::gtkHandleButtonPressedEvent(Aspect_WindowInputListener& theListener,
                                               const Handle(V3d_View)& theView,
                                               const GdkEventButton* theEvent)
{
  if (theView->Window().IsNull())
    return false;

  const Graphic3d_Vec2d aPnt2d(theEvent->x, theEvent->y);
  const Graphic3d_Vec2i aPnt2i(theView->Window()->ConvertPointToBacking(aPnt2d) + Graphic3d_Vec2d(0.5));
  const Aspect_VKeyMouse aButton = OcctGtkTools::gtkMouseButton2VKey(theEvent->button);
  const Aspect_VKeyFlags aFlags  = OcctGtkTools::gtkMouseFlags2VKeys(theEvent->state);
  if (aButton == Aspect_VKeyMouse_NONE)
    return false;

  return theListener.PressMouseButton(aPnt2i, aButton, aFlags, false);
}

// ================================================================
// Function : gtkHandleButtonReleasedEvent
// ================================================================
bool OcctGtkTools::gtkHandleButtonReleasedEvent(Aspect_WindowInputListener& theListener,
                                                const Handle(V3d_View)& theView,
                                                const GdkEventButton* theEvent)
{
  if (theView->Window().IsNull())
    return false;

  const Graphic3d_Vec2d aPnt2d(theEvent->x, theEvent->y);
  const Graphic3d_Vec2i aPnt2i(theView->Window()->ConvertPointToBacking(aPnt2d) + Graphic3d_Vec2d(0.5));
  const Aspect_VKeyMouse aButton = OcctGtkTools::gtkMouseButton2VKey(theEvent->button);
  const Aspect_VKeyFlags aFlags  = OcctGtkTools::gtkMouseFlags2VKeys(theEvent->state);
  if (aButton == Aspect_VKeyMouse_NONE)
    return false;

  return theListener.ReleaseMouseButton(aPnt2i, aButton, aFlags, false);
}

// ================================================================
// Function : gtkHandleScrollEvent
// ================================================================
bool OcctGtkTools::gtkHandleScrollEvent(Aspect_WindowInputListener& theListener,
                                        const Handle(V3d_View)& theView,
                                        const GdkEventScroll* theEvent)
{
  if (theView->Window().IsNull())
    return false;

  const Graphic3d_Vec2d aPnt2d(theEvent->x, theEvent->y);
  const Graphic3d_Vec2i aPnt2i(theView->Window()->ConvertPointToBacking(aPnt2d) + Graphic3d_Vec2d(0.5));
  return theListener.UpdateMouseScroll(Aspect_ScrollDelta(aPnt2i, -theEvent->delta_y));
}
#endif
