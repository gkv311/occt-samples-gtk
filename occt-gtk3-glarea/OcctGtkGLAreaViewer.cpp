// Copyright (c) 2023-2026 Kirill Gavrilov

#include "OcctGtkGLAreaViewer.h"

#include "../occt-gtk-tools/OcctGlTools.h"
#include "../occt-gtk-tools/OcctGtkTools.h"

#include <Message.hxx>
#include <OpenGl_Context.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <OpenGl_FrameBuffer.hxx>

#ifdef _WIN32
  //
#elif defined(HAVE_WAYLAND)
  #include <gdk/gdkwayland.h>
#else
  #include <gdk/gdkx.h>
  #include <X11/Xlib.h>
#endif

// ================================================================
// Function : OcctGtkGLAreaViewer
// ================================================================
OcctGtkGLAreaViewer::OcctGtkGLAreaViewer()
{
  // receive keyboard events when focused
  set_can_focus(true);
  set_focus_on_click(true);

  Handle(Aspect_DisplayConnection) aDisp;
#if !defined(__APPLE__) && !defined(_WIN32) && !defined(HAVE_WAYLAND)
  aDisp = new Xw_DisplayConnection();
#endif
  Handle(OpenGl_GraphicDriver) aDriver = new OpenGl_GraphicDriver(aDisp, false);
  // lets Gtk::GLArea to manage buffer swap
  aDriver->ChangeOptions().buffersNoSwap = true;
  // don't write into alpha channel
  aDriver->ChangeOptions().buffersOpaqueAlpha = true;
  // offscreen FBOs should be always used
  aDriver->ChangeOptions().useSystemBuffer = false;
  // GTK3 creates Core Profile when possible with no option to manage this behavior!
  aDriver->ChangeOptions().contextCompatible = false;

  // create viewer
  myViewer = new V3d_Viewer(aDriver);
  myViewer->SetDefaultBackgroundColor(Quantity_NOC_BLACK);
  myViewer->SetDefaultLights();
  myViewer->SetLightOn();
  myViewer->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines);

  // create AIS context
  myContext = new AIS_InteractiveContext(myViewer);

  myViewCube = new AIS_ViewCube();
  myViewCube->SetViewAnimation(myViewAnimation);
  myViewCube->SetFixedAnimationLoop(false);
  myViewCube->SetAutoStartAnimation(true);

  // note - window will be created later within onGlAreaRealized() callback!
  myView = myViewer->CreateView();
  myView->SetImmediateUpdate(false);
  myView->ChangeRenderingParams().ToShowStats = true;
  // NOLINTNEXTLINE
  myView->ChangeRenderingParams().CollectedStats = (Graphic3d_RenderingParams::PerfCounters)(
    Graphic3d_RenderingParams::PerfCounters_FrameRate | Graphic3d_RenderingParams::PerfCounters_Triangles);

#ifdef HAVE_GLES2
  set_use_es(true);
#else
  set_use_es(false);
#endif

  setupInputCallbacks();
}

// ================================================================
// Function : setupInputCallbacks
// ================================================================
void OcctGtkGLAreaViewer::setupInputCallbacks()
{
  // connect to Gtk::GLArea events
  signal_realize()  .connect(sigc::mem_fun(*this, &OcctGtkGLAreaViewer::onGlAreaRealized));
  // important that the unrealize signal calls our handler to clean up
  // GL resources _before_ the default unrealize handler is called (the "false")
  signal_unrealize().connect(sigc::mem_fun(*this, &OcctGtkGLAreaViewer::onGlAreaReleased), false);
  signal_render()   .connect(sigc::mem_fun(*this, &OcctGtkGLAreaViewer::onGlAreaRender), false);

  // multi-touch events can be enabled, but on Windows platform they are delivered
  // concurrently with emulated mouse cursor events, breaking Viewer logic;
  // might work as expected on other systems that don't generate emulated events
  const bool toEnableMultitouch = false;

  // connect to input events
  Gdk::EventMask anEventFilter = Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK
                               | Gdk::SMOOTH_SCROLL_MASK  | Gdk::FOCUS_CHANGE_MASK
                               | Gdk::KEY_PRESS_MASK | Gdk::KEY_RELEASE_MASK;
  if (toEnableMultitouch)
    anEventFilter |= Gdk::TOUCH_MASK;

  add_events(anEventFilter);

  signal_motion_notify_event().connect([this](GdkEventMotion* theEvent) -> bool
  {
    if (OcctGtkTools::gtkHandleMotionEvent(*this, myView, theEvent))
      queue_draw();

    return true;
  }, false);

  signal_button_press_event().connect([this](GdkEventButton* theEvent) -> bool
  {
    if (get_focus_on_click())
      grab_focus(); // grab keyboard input

    if (OcctGtkTools::gtkHandleButtonEvent(*this, myView, theEvent))
      queue_draw();

    return true;
  }, false);

  signal_button_release_event().connect([this](GdkEventButton* theEvent) -> bool
  {
    if (OcctGtkTools::gtkHandleButtonEvent(*this, myView, theEvent))
      queue_draw();

    return true;
  }, false);

  signal_scroll_event().connect([this](GdkEventScroll* theEvent) -> bool
  {
    if (OcctGtkTools::gtkHandleScrollEvent(*this, myView, theEvent))
      queue_draw();

    return true;
  }, false);

  signal_key_press_event().connect([this](GdkEventKey* theEvent) -> bool
  {
    return onKey(theEvent);
  }, false);

  signal_key_release_event().connect([this](GdkEventKey* theEvent) -> bool
  {
    return onKey(theEvent);
  }, false);

  signal_focus_in_event().connect([this](GdkEventFocus*) -> bool
  {
    AIS_ViewController::ProcessFocus(true);
    return false;
  }, false);

  signal_focus_out_event().connect([this](GdkEventFocus*) -> bool
  {
    AIS_ViewController::ProcessFocus(false);
    return false;
  }, false);

  if (toEnableMultitouch)
    signal_touch_event().connect(sigc::mem_fun(*this, &OcctGtkGLAreaViewer::onTouch), false);
}

// ================================================================
// Function : ~OcctGtkGLAreaViewer
// ================================================================
OcctGtkGLAreaViewer::~OcctGtkGLAreaViewer()
{
  //
}

// ================================================================
// Function : onTouch
// ================================================================
bool OcctGtkGLAreaViewer::onTouch(GdkEventTouch* theEvent)
{
  const Standard_Size   aTouchId = (Standard_Size)theEvent->sequence;
  const Graphic3d_Vec2d aNewPos2d =
    myView->Window()->ConvertPointToBacking(Graphic3d_Vec2d(theEvent->x, theEvent->y));

  bool hasUpdates = false;
  if (theEvent->state == GDK_TOUCH_BEGIN)
  {
    hasUpdates = true;
    AIS_ViewController::AddTouchPoint(aTouchId, aNewPos2d);
  }
  else if (theEvent->state == GDK_TOUCH_UPDATE
        && AIS_ViewController::TouchPoints().Contains(aTouchId))
  {
    hasUpdates = true;
    AIS_ViewController::UpdateTouchPoint(aTouchId, aNewPos2d);
  }
  else if ((theEvent->state == GDK_TOUCH_END || theEvent->state == GDK_TOUCH_CANCEL)
        && AIS_ViewController::RemoveTouchPoint(aTouchId))
  {
    hasUpdates = true;
  }
  else
  {
    return false;
  }

  if (hasUpdates)
    queue_draw();

  return true;
}

// ================================================================
// Function : onKey
// ================================================================
bool OcctGtkGLAreaViewer::onKey(GdkEventKey* theEvent)
{
  const Aspect_VKey aVKey = OcctGtkTools::gtkKey2VKey(theEvent->keyval, guint(-1));
  if (aVKey == Aspect_VKey_UNKNOWN)
    return false;

  const double aTimeStamp = AIS_ViewController::EventTime();
  if (theEvent->type == GDK_KEY_PRESS)
    AIS_ViewController::KeyDown(aVKey, aTimeStamp);
  else
    AIS_ViewController::KeyUp(aVKey, aTimeStamp);

  if (theEvent->type == GDK_KEY_PRESS)
    processKeyPress(aVKey);

  AIS_ViewController::ProcessInput();
  return true;
}

// ================================================================
// Function : processKeyPress
// ================================================================
void OcctGtkGLAreaViewer::processKeyPress(Aspect_VKey theKey)
{
  if (myView.IsNull())
    return;

  switch (theKey)
  {
    case Aspect_VKey_Escape:
    {
      std::exit(0);
      break;
    }
    case Aspect_VKey_F:
    {
      myView->FitAll(0.01, false);
      queue_draw();
      break;
    }
  }
}

// ================================================================
// Function : handleViewRedraw
// ================================================================
void OcctGtkGLAreaViewer::handleViewRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                           const Handle(V3d_View)& theView)
{
  AIS_ViewController::handleViewRedraw(theCtx, theView);
  if (myToAskNextFrame)
    queue_draw(); // ask more frames
}

// ================================================================
// Function : initPixelScaleRatio
// ================================================================
void OcctGtkGLAreaViewer::initPixelScaleRatio()
{
  AIS_ViewController::SetTouchToleranceScale(myDevicePixelRatio);
  myView->ChangeRenderingParams().Resolution = (unsigned int )(96.0 * myDevicePixelRatio + 0.5);
}

// ================================================================
// Function : dumpGlInfo
// ================================================================
void OcctGtkGLAreaViewer::dumpGlInfo(bool theIsBasic, bool theToPrint)
{
  TColStd_IndexedDataMapOfStringString aGlCapsDict;
  myView->DiagnosticInformation(aGlCapsDict,
                                theIsBasic ? Graphic3d_DiagnosticInfo_Basic : Graphic3d_DiagnosticInfo_Complete);
  TCollection_AsciiString anInfo;
  for (TColStd_IndexedDataMapOfStringString::Iterator aValueIter(aGlCapsDict); aValueIter.More(); aValueIter.Next())
  {
    if (!aValueIter.Value().IsEmpty())
    {
      if (!anInfo.IsEmpty())
        anInfo += "\n";

      anInfo += aValueIter.Key() + ": " + aValueIter.Value();
    }
  }
  myGlInfo = anInfo;

  if (theToPrint)
    Message::SendInfo(anInfo);
}

// ================================================================
// Function : onGlAreaRealized
// ================================================================
void OcctGtkGLAreaViewer::onGlAreaRealized()
{
  make_current();
  Graphic3d_Vec2i aLogicalSize(get_width(), get_height());
#ifdef HAVE_GLES2
  if (!get_context()->get_use_es())
  {
    Message::SendFail() << "Broken configuration: OpenGL ES expected, but dekstop OpenGL is given\n";
    return;
  }
#else
  if (get_context()->get_use_es())
  {
    Message::SendFail() << "Broken configuration: desktop OpenGL expected, but OpenGL ES is given\n";
    return;
  }
#endif

  // check Wayland vs. X11 backend
  Aspect_Drawable aNativeWin = 0;
#if defined(HAVE_WAYLAND)
  struct wl_surface* aWlSurf = gdk_wayland_window_get_wl_surface(gtk_widget_get_window((GtkWidget* )gobj()));
  if (aWlSurf == nullptr)
  {
    Message::SendFail() << "Broken configuration: Wayland surface is expected\n";
    return;
  }
#elif !defined(_WIN32) && !defined(__APPLE__)
  aNativeWin = gdk_x11_window_get_xid(gtk_widget_get_window((GtkWidget* )gobj()));
  if (aNativeWin == 0)
  {
    Message::SendFail() << "Broken configuration: X11 window is expected\n";
    return;
  }
#endif

  try
  {
    OCC_CATCH_SIGNALS
    throw_if_error();

    // FBO is not yet initialized?
    const Graphic3d_Vec2i aViewSize = aLogicalSize * get_scale_factor();
    myDevicePixelRatio = float(aViewSize.y()) / float(aLogicalSize.y());
    initPixelScaleRatio();

    const bool isFirstInit = myView->Window().IsNull();
    if (!OcctGlTools::InitializeGlWindow(myView, aNativeWin, aViewSize, myDevicePixelRatio))
    {
      Gtk::MessageDialog aMsg("Error: OpenGl_Context is unable to wrap OpenGL context", false, Gtk::MESSAGE_ERROR);
      aMsg.run();
    }
    make_current();

    dumpGlInfo(true, true);
    if (isFirstInit)
    {
      myContext->Display(myViewCube, 0, 0, false);
    }
  }
  catch (const Gdk::GLError& theGlErr)
  {
    Message::SendFail() << "An error occurred making the context current during realize:\n"
                        << theGlErr.domain() << "-" << theGlErr.code() << "-" << theGlErr.what();
  }
  catch (const Standard_Failure& theErr)
  {
    Message::SendFail() << "An error occurred making the context current during realize:\n"
                        << theErr;
  }
}

// ================================================================
// Function : onGlAreaReleased
// ================================================================
void OcctGtkGLAreaViewer::onGlAreaReleased()
{
  make_current();
  try
  {
    throw_if_error();

    // release OCCT viewer on application close
    Handle(Aspect_DisplayConnection) aDisp;
    if (!myView.IsNull())
    {
      aDisp = myViewer->Driver()->GetDisplayConnection();
      myView->Remove();
      myView.Nullify();
      myViewer.Nullify();
    }

    make_current();
    aDisp.Nullify();
  }
  catch (const Gdk::GLError& theGlErr)
  {
    Message::SendFail() << "An error occurred making the context current during unrealize\n"
                        << theGlErr.domain() << "-" << theGlErr.code() << "-" << theGlErr.what() << "\n";
  }
}

// ================================================================
// Function : onGlAreaRender
// ================================================================
bool OcctGtkGLAreaViewer::onGlAreaRender(const Glib::RefPtr<Gdk::GLContext>& theGlCtx)
{
  (void )theGlCtx;
  if (myView->Window().IsNull())
    return false;

  try
  {
    throw_if_error();

    // wrap FBO created by Gtk::GLArea
    if (!OcctGlTools::InitializeGlFbo(myView))
    {
      Gtk::MessageDialog aMsg("Default FBO wrapper creation failed", false, Gtk::MESSAGE_ERROR);
      aMsg.run();
      return false;
    }

    // calculate pixel ratio between OpenGL FBO viewport dimension and Gtk::GLArea logical size
    const Graphic3d_Vec2i aLogicalSize(get_width(), get_height());
    Graphic3d_Vec2i aViewSize; myView->Window()->Size(aViewSize.x(), aViewSize.y());

    const float aPixelRatio = float(aViewSize.y()) / float(aLogicalSize.y());
    if (myDevicePixelRatio != aPixelRatio)
    {
      myDevicePixelRatio = aPixelRatio;
      Handle(OcctGlTools::OcctNeutralWindow) aWindow = Handle(OcctGlTools::OcctNeutralWindow)::DownCast(myView->Window());
      aWindow->SetDevicePixelRatio(aPixelRatio);
      initPixelScaleRatio();
      dumpGlInfo(true, false);
    }

    // flush pending input events and redraw the viewer
    myView->InvalidateImmediate();
    AIS_ViewController::FlushViewEvents(myContext, myView, true);
    return true;
  }
  catch (const Gdk::GLError& theGlErr)
  {
    Message::SendFail() << "An error occurred in the render callback of the GLArea\n"
                        << theGlErr.domain() << "-" << theGlErr.code() << "-" << theGlErr.what() << "\n";
    return false;
  }
}
