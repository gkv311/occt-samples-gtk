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
  #include <gdk/wayland/gdkwayland.h>
#else
  #include <gdk/x11/gdkx.h>
#endif

// ================================================================
// Function : ToUseModernInput
// ================================================================
bool& OcctGtkGLAreaViewer::ToUseModernInput()
{
  static bool toUseModern = false;
  return toUseModern;
}

// ================================================================
// Function : OcctGtkGLAreaViewer
// ================================================================
OcctGtkGLAreaViewer::OcctGtkGLAreaViewer(bool theUseModernInput)
{
  // receive keyboard events when focused
  set_can_focus(true);
  set_focusable(true);
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
  // GTK4 creates Core Profile when possible with no option to manage this behavior!
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

  // connect to Gtk::GLArea events
  // GTK4 calls signal_realize() with 0x0 dimensions making 3D Viewer initialization impossible
  //signal_realize().connect(sigc::mem_fun(*this, &OcctGtkGLAreaViewer::onGlAreaRealized));
  // important that the unrealize signal calls our handler to clean up
  // GL resources _before_ the default unrealize handler is called (the "false")
  signal_unrealize().connect(sigc::mem_fun(*this, &OcctGtkGLAreaViewer::onGlAreaReleased), false);
  signal_render()   .connect(sigc::mem_fun(*this, &OcctGtkGLAreaViewer::onGlAreaRender), false);

  if (theUseModernInput)
  {
    addModernEventControllers();
  }
  else
  {
    // TODO mouse/touch events come with offset from Gtk::EventControllerLegacy
    // when client-side-decorations are enabled (GTK_CSD=1)
    // https://gitlab.gnome.org/GNOME/gtk/-/issues/7983
    Message::SendTrace() << "OcctGtkGLAreaViewer: using 'legacy' GTK4 input controllers API";
    Glib::RefPtr<Gtk::EventControllerLegacy> anEventCtrl = Gtk::EventControllerLegacy::create();
    anEventCtrl->signal_event().connect(sigc::mem_fun(*this, &OcctGtkGLAreaViewer::onRawEvent), false);
    add_controller(anEventCtrl);
  }
}

// ================================================================
// Function : addModernEventControllers
// ================================================================
void OcctGtkGLAreaViewer::addModernEventControllers()
{
  Message::SendTrace() << "OcctGtkGLAreaViewer: using 'modern' GTK4 input controllers API";

  Glib::RefPtr<Gtk::EventControllerMotion> anEventCtrlMotion = Gtk::EventControllerMotion::create();
  anEventCtrlMotion->signal_motion().connect([this](double theX, double theY)
  {
    if (OcctGtkTools::gtkHandleMotionEvent(*this, myView, Graphic3d_Vec2d(theX, theY), myKeyModifiers))
      queue_draw();
  }, false);
  add_controller(anEventCtrlMotion);

  Glib::RefPtr<Gtk::EventControllerScroll> anEventCtrlScroll = Gtk::EventControllerScroll::create();
  anEventCtrlScroll->set_flags(Gtk::EventControllerScroll::Flags::VERTICAL);
  const Gtk::EventControllerScroll* aScrollPtr = anEventCtrlScroll.get();
  anEventCtrlScroll->signal_scroll().connect([this, aScrollPtr](double theDeltaX, double theDeltaY) -> bool
  {
    Graphic3d_Vec2d aDelta(theDeltaX, theDeltaY);
    if (aScrollPtr->get_unit() == Gdk::ScrollUnit::SURFACE)
      aDelta = (aDelta * myDevicePixelRatio) / 123.0; // 123 logical pixels from GTK documentation

    if (OcctGtkTools::gtkHandleScrollEvent(*this, myView, aDelta))
      queue_draw();

    return true;
  }, false);
  add_controller(anEventCtrlScroll);

  Glib::RefPtr<Gtk::GestureClick> anEventCtrlClick = Gtk::GestureClick::create();
  anEventCtrlClick->set_button(0); // listen to all buttons
  const Gtk::GestureClick* aClickPtr = anEventCtrlClick.get();
  anEventCtrlClick->signal_pressed().connect([this, aClickPtr](int , double theX, double theY)
  {
    if (get_focus_on_click())
      grab_focus(); // grab keyboard input

    if (OcctGtkTools::gtkHandleButtonEvent(*this, myView, Graphic3d_Vec2d(theX, theY),
                                           aClickPtr->get_current_button(), myKeyModifiers, true))
      queue_draw();
  }, false);
  anEventCtrlClick->signal_released().connect([this, aClickPtr](int , double theX, double theY)
  {
    if (OcctGtkTools::gtkHandleButtonEvent(*this, myView, Graphic3d_Vec2d(theX, theY),
                                           aClickPtr->get_current_button(), myKeyModifiers, false))
      queue_draw();
  }, false);
  anEventCtrlClick->signal_cancel().connect([this, aClickPtr](Gdk::EventSequence* )
  {
    // when multiple mouse buttons are pressed, cancel event comes from Gtk::GestureClick instead of release
    const Aspect_VKeyMouse aButton = OcctGtkTools::gtkMouseButton2VKey(aClickPtr->get_current_button());
    if (AIS_ViewController::ReleaseMouseButton(AIS_ViewController::LastMousePosition(), aButton, myKeyModifiers, false))
      queue_draw();
  }, false);
  add_controller(anEventCtrlClick);

  // TODO there is no event from Gtk::EventControllerKey to reset modifiers...
  Glib::RefPtr<Gtk::EventControllerKey> anEventCtrlKey = Gtk::EventControllerKey::create();
  anEventCtrlKey->signal_modifiers().connect([this](Gdk::ModifierType theType) -> bool
  {
    (void)theType; //myKeyModifiers = OcctGtkTools::gtkModifiers2VKeys(theType);
    return false;  //return true;
  }, false);
  anEventCtrlKey->signal_key_pressed().connect([this](guint theKeyVal, guint theKeyCode, Gdk::ModifierType theType) -> bool
                                               { return onKey(theKeyVal, theKeyCode, theType, true); }, false);
  anEventCtrlKey->signal_key_released().connect([this](guint theKeyVal, guint theKeyCode, Gdk::ModifierType theType)
                                                { onKey(theKeyVal, theKeyCode, theType, false); }, false);
  add_controller(anEventCtrlKey);

  Glib::RefPtr<Gtk::EventControllerFocus> anEventCtrlFocus = Gtk::EventControllerFocus::create();
  anEventCtrlFocus->signal_enter().connect([this]() {
    AIS_ViewController::ProcessFocus(true);
  }, false);
  anEventCtrlFocus->signal_leave().connect([this]()
  {
    AIS_ViewController::ProcessFocus(false);
    myKeyModifiers = Aspect_VKeyFlags_NONE;
  }, false);
  add_controller(anEventCtrlFocus);
}

// ================================================================
// Function : ~OcctGtkGLAreaViewer
// ================================================================
OcctGtkGLAreaViewer::~OcctGtkGLAreaViewer()
{
  //
}

// ================================================================
// Function : onRawEvent
// ================================================================
bool OcctGtkGLAreaViewer::onRawEvent(const Glib::RefPtr<const Gdk::Event>& theEvent)
{
  if (myView->Window().IsNull())
    return false;

  switch (theEvent->get_event_type())
  {
    case Gdk::Event::Type::MOTION_NOTIFY:
    {
      //if (theEvent->get_pointer_emulated()) { return false; }

      Graphic3d_Vec2d aPos;
      theEvent->get_position(aPos.x(), aPos.y());
      const Aspect_VKeyFlags aFlags = OcctGtkTools::gtkModifiers2VKeys(theEvent->get_modifier_state());
      if (OcctGtkTools::gtkHandleMotionEvent(*this, myView, aPos, aFlags))
        queue_draw();

      return true;
    }
    case Gdk::Event::Type::BUTTON_PRESS:
    case Gdk::Event::Type::BUTTON_RELEASE:
    {
      if (get_focus_on_click())
        grab_focus(); // grab keyboard input

      //if (theEvent->get_pointer_emulated()) { return false; }

      Graphic3d_Vec2d aPos;
      theEvent->get_position(aPos.x(), aPos.y());
      const Aspect_VKeyFlags aFlags = OcctGtkTools::gtkModifiers2VKeys(theEvent->get_modifier_state());
      if (OcctGtkTools::gtkHandleButtonEvent(*this, myView,
                                             aPos, theEvent->get_button(), aFlags,
                                             theEvent->get_event_type() == Gdk::Event::Type::BUTTON_PRESS))
        queue_draw();

      return true;
    }
    case Gdk::Event::Type::SCROLL:
    {
      Graphic3d_Vec2d aDelta;
      theEvent->get_deltas(aDelta.x(), aDelta.y());
      if (theEvent->get_scroll_unit() == Gdk::ScrollUnit::SURFACE)
        aDelta = (aDelta * myDevicePixelRatio) / 123.0; // 123 logical pixels from GTK documentation

      Graphic3d_Vec2d aPos;
      theEvent->get_position(aPos.x(), aPos.y());
      const Graphic3d_Vec2i aPnt2i(myView->Window()->ConvertPointToBacking(aPos) + Graphic3d_Vec2d(0.5));
      if (AIS_ViewController::UpdateMouseScroll(Aspect_ScrollDelta(aPnt2i, -aDelta.y())))
        queue_draw();

      return true;
    }
    case Gdk::Event::Type::TOUCH_BEGIN:
    case Gdk::Event::Type::TOUCH_UPDATE:
    case Gdk::Event::Type::TOUCH_END:
    case Gdk::Event::Type::TOUCH_CANCEL:
    {
      Graphic3d_Vec2d aPos;
      theEvent->get_position(aPos.x(), aPos.y());

      const Standard_Size   aTouchId = (Standard_Size)theEvent->get_event_sequence();
      const Graphic3d_Vec2d aNewPos2d = myView->Window()->ConvertPointToBacking(aPos);

      bool hasUpdates = false;
      if (theEvent->get_event_type() == Gdk::Event::Type::TOUCH_BEGIN)
      {
        hasUpdates = true;
        AIS_ViewController::AddTouchPoint(aTouchId, aNewPos2d);
      }
      else if (theEvent->get_event_type() == Gdk::Event::Type::TOUCH_UPDATE
            && AIS_ViewController::TouchPoints().Contains(aTouchId))
      {
        hasUpdates = true;
        AIS_ViewController::UpdateTouchPoint(aTouchId, aNewPos2d);
      }
      else if ((theEvent->get_event_type() == Gdk::Event::Type::TOUCH_END
             || theEvent->get_event_type() == Gdk::Event::Type::TOUCH_CANCEL)
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
    case Gdk::Event::Type::KEY_PRESS:
    case Gdk::Event::Type::KEY_RELEASE:
      return onKey(theEvent->get_keyval(), theEvent->get_keycode(), theEvent->get_modifier_state(),
                   theEvent->get_event_type() == Gdk::Event::Type::KEY_PRESS);
    case Gdk::Event::Type::FOCUS_CHANGE:
      AIS_ViewController::ProcessFocus(theEvent->get_focus_in());
      return true;
    default:
      break;
  }

  return false;
}

// ================================================================
// Function : onKey
// ================================================================
bool OcctGtkGLAreaViewer::onKey(guint theKeyVal, guint theKeyCode, Gdk::ModifierType , bool theIsPressed)
{
  const Aspect_VKey aVKey = OcctGtkTools::gtkKey2VKey(theKeyVal, theKeyCode);
  if (aVKey == Aspect_VKey_UNKNOWN)
    return false;

  const double aTimeStamp = AIS_ViewController::EventTime();
  if (theIsPressed)
    AIS_ViewController::KeyDown(aVKey, aTimeStamp);
  else
    AIS_ViewController::KeyUp(aVKey, aTimeStamp);

  if (theIsPressed)
    processKeyPress(aVKey);

  updateModifiers();

  AIS_ViewController::ProcessInput();
  return true;
}

// ================================================================
// Function : updateModifiers
// ================================================================
void OcctGtkGLAreaViewer::updateModifiers()
{
  Aspect_VKeyFlags aFlags = Aspect_VKeyFlags_NONE;
  if (AIS_ViewController::Keys().IsKeyDown(Aspect_VKey_Shift))
    aFlags |= Aspect_VKeyFlags_SHIFT;

  if (AIS_ViewController::Keys().IsKeyDown(Aspect_VKey_Control))
    aFlags |= Aspect_VKeyFlags_CTRL;

  if (AIS_ViewController::Keys().IsKeyDown(Aspect_VKey_Meta))
    aFlags |= Aspect_VKeyFlags_META;

  if (AIS_ViewController::Keys().IsKeyDown(Aspect_VKey_Alt))
    aFlags |= Aspect_VKeyFlags_ALT;

  myKeyModifiers = aFlags;
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
  if (myToAskNextFrame != (myAnimationCallback != 0))
  {
    if (myToAskNextFrame)
    {
      // start animation
      myAnimationCallback = add_tick_callback([this](const Glib::RefPtr<Gdk::FrameClock>&) -> bool
      {
        queue_draw();
        return true;
      });
    }
    else
    {
      // stop animation
      remove_tick_callback(myAnimationCallback);
      myAnimationCallback = 0;
    }
  }
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
  if (aLogicalSize.x() == 0 || aLogicalSize.y() == 0)
    return;

  // check OpenGL vs. OpenGL ES context
  const Gdk::GLApi anApi = get_context()->get_api2();
#ifdef HAVE_GLES2
  if (anApi != Gdk::GLApi::GLES)
  {
    Message::SendFail() << "Broken configuration: OpenGL ES is expected, but dekstop OpenGL created\n";
    return;
  }
#else
  if (anApi != Gdk::GLApi::GL)
  {
    Message::SendFail() << "Broken configuration: desktop OpenGL is expected, but OpenGL ES created\n";
    return;
  }
#endif

  // check Wayland vs. X11 backend
  Glib::RefPtr<Gdk::Surface> aGdkSurf = this->get_native()->get_surface();
  Aspect_Drawable aNativeWin = 0;
#if defined(HAVE_WAYLAND)
  struct wl_surface* aWlSurf = gdk_wayland_surface_get_wl_surface(aGdkSurf->gobj());
  if (aWlSurf == nullptr)
  {
    Message::SendFail() << "Broken configuration: Wayland surface is expected\n";
    return;
  }
#elif !defined(_WIN32) && !defined(__APPLE__)
  aNativeWin = gdk_x11_surface_get_xid(aGdkSurf->gobj());
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
#if ((GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 12) || GTKMM_MAJOR_VERSION >= 5)
    // fractional scale factor introduced by gtkmm 4.12
    myDevicePixelRatio = aGdkSurf->get_scale();
#else
    myDevicePixelRatio = get_scale_factor();
#endif
    initPixelScaleRatio();

    const bool isFirstInit = myView->Window().IsNull();
    const Graphic3d_Vec2i aViewSize = Graphic3d_Vec2i(Graphic3d_Vec2d(aLogicalSize) * myDevicePixelRatio + Graphic3d_Vec2d(0.5));
    if (!OcctGlTools::InitializeGlWindow(myView, aNativeWin, aViewSize, myDevicePixelRatio))
    {
      Gtk::MessageDialog* aMsg = new Gtk::MessageDialog("OpenGl_Context is unable to wrap OpenGL context", false, Gtk::MessageType::ERROR);
      aMsg->set_title("Critical error: 3D Viewer initialization failure");
      aMsg->signal_response().connect([aMsg](int) { delete aMsg; });
      aMsg->show();
    }
    make_current();

    dumpGlInfo(true, false);
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
  const bool isFirstInit = myView->Window().IsNull();
  if (myView->Window().IsNull())
  {
    onGlAreaRealized();
    make_current();
    gtk_gl_area_attach_buffers(gobj()); // not wrapped by C++ gtkmm
  }

  if (myView->Window().IsNull())
    return false;

  try
  {
    throw_if_error();

    // wrap FBO created by Gtk::GLArea
    if (!OcctGlTools::InitializeGlFbo(myView))
    {
      Gtk::MessageDialog* aMsg = new Gtk::MessageDialog("Default FBO wrapper creation failed", false, Gtk::MessageType::ERROR);
      aMsg->set_title("Critical error: 3D Viewer initialization failure");
      aMsg->signal_response().connect([aMsg](int) { delete aMsg; });
      aMsg->show();
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
    if (isFirstInit)
      dumpGlInfo(true, true);

    return true;
  }
  catch (const Gdk::GLError& theGlErr)
  {
    Message::SendFail() << "An error occurred in the render callback of the GLArea\n"
                        << theGlErr.domain() << "-" << theGlErr.code() << "-" << theGlErr.what() << "\n";
    return false;
  }
}
