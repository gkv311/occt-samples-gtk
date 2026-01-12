// Copyright (c) 2023 Kirill Gavrilov

#include "OcctGtkGLAreaViewer.h"

#include "../occt-gtk-tools/OcctGlTools.h"
#include "../occt-gtk-tools/OcctGtkTools.h"

#include <Message.hxx>
#include <OpenGl_Context.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <OpenGl_FrameBuffer.hxx>

#if !defined(__APPLE__) && !defined(_WIN32) && defined(__has_include)
  #if __has_include(<Xw_DisplayConnection.hxx>)
    #include <Xw_DisplayConnection.hxx>
    #define USE_XW_DISPLAY
  #endif
#endif
#ifndef USE_XW_DISPLAY
typedef Aspect_DisplayConnection Xw_DisplayConnection;
#endif

#ifdef _WIN32
  //
#else
  #include <gdk/x11/gdkx.h>
#endif

// ================================================================
// Function : OcctGtkGLAreaViewer
// ================================================================
OcctGtkGLAreaViewer::OcctGtkGLAreaViewer()
{
  Handle(Aspect_DisplayConnection) aDisp = new Xw_DisplayConnection();
  Handle(OpenGl_GraphicDriver) aDriver = new OpenGl_GraphicDriver(aDisp, false);
  // lets Gtk::GLArea to manage buffer swap
  aDriver->ChangeOptions().buffersNoSwap = true;
  // don't write into alpha channel
  aDriver->ChangeOptions().buffersOpaqueAlpha = true;
  // offscreen FBOs should be always used
  aDriver->ChangeOptions().useSystemBuffer = false;
  // GTK3/GTK4 creates Core Profile when possible
  // (Compatible Profile created only as a fallback)
  // with no option to manage this behavior!
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
#endif

  // connect to Gtk::GLArea events
  signal_realize()  .connect(sigc::mem_fun(*this, &OcctGtkGLAreaViewer::onGlAreaRealized));
  // important that the unrealize signal calls our handler to clean up
  // GL resources _before_ the default unrealize handler is called (the "false")
  signal_unrealize().connect(sigc::mem_fun(*this, &OcctGtkGLAreaViewer::onGlAreaReleased), false);
  signal_render()   .connect(sigc::mem_fun(*this, &OcctGtkGLAreaViewer::onGlAreaRender), false);

  // connect to input events using raw controller
  Glib::RefPtr<Gtk::EventControllerLegacy> anEventCtrl = Gtk::EventControllerLegacy::create();
  anEventCtrl->signal_event().connect(sigc::mem_fun(*this, &OcctGtkGLAreaViewer::onRawEvent), false);
  add_controller(anEventCtrl);

  // connect to input events using 'modern' controllers (but it is buggy)
  /*Glib::RefPtr<Gtk::EventControllerMotion> anEventCtrlMotion = Gtk::EventControllerMotion::create();
  anEventCtrlMotion->signal_motion().connect(sigc::mem_fun(*this, &OcctGtkGLAreaViewer::onMotionMove), false);
  add_controller(anEventCtrlMotion);

  Glib::RefPtr<Gtk::EventControllerScroll> anEventCtrlScroll = Gtk::EventControllerScroll::create();
  anEventCtrlScroll->set_flags(Gtk::EventControllerScroll::Flags::VERTICAL);
  anEventCtrlScroll->signal_scroll().connect(sigc::mem_fun(*this, &OcctGtkGLAreaViewer::onMouseScroll), false);
  add_controller(anEventCtrlScroll);

  myEventCtrlClick = Gtk::GestureClick::create();
  myEventCtrlClick->set_button(0); // listen to all buttons
  myEventCtrlClick->signal_pressed().connect(sigc::mem_fun(*this, &OcctGtkGLAreaViewer::onMouseButtonPressed), false);
  myEventCtrlClick->signal_released().connect(sigc::mem_fun(*this, &OcctGtkGLAreaViewer::onMouseButtonReleased), false);
  add_controller(myEventCtrlClick);

  /// TODO why this doesn't work without forwarding from Window? events do not reset modifiers...
  myEventCtrlKey = Gtk::EventControllerKey::create();
  myEventCtrlKey->signal_modifiers().connect(sigc::mem_fun(*this, &OcctGtkGLAreaViewer::onModifiersChanged), false);
  myEventCtrlKey->signal_key_pressed().connect(sigc::mem_fun(*this, &OcctGtkGLAreaViewer::onKeyPressed), false);
  myEventCtrlKey->signal_key_released().connect(sigc::mem_fun(*this, &OcctGtkGLAreaViewer::onKeyReleased), false);
  add_controller(myEventCtrlKey);*/
}

// ================================================================
// Function : ~OcctGtkGLAreaViewer
// ================================================================
OcctGtkGLAreaViewer::~OcctGtkGLAreaViewer()
{
  //
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
      //const bool isEmulated = theEvent->get_pointer_emulated();
      //if (isEmulated)
      //  return false;

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
      //const bool isEmulated = theEvent->get_pointer_emulated();
      //if (isEmulated)
      //  return false;

      Graphic3d_Vec2d aPos;
      theEvent->get_position(aPos.x(), aPos.y());
      const Aspect_VKeyFlags aFlags = OcctGtkTools::gtkModifiers2VKeys(theEvent->get_modifier_state());
      if (theEvent->get_event_type() == Gdk::Event::Type::BUTTON_PRESS
        ? OcctGtkTools::gtkHandleButtonPressedEvent(*this, myView, aPos, theEvent->get_button(), aFlags)
        : OcctGtkTools::gtkHandleButtonReleasedEvent(*this, myView, aPos, theEvent->get_button(), aFlags))
      {
        queue_draw();
      }
      return true;
    }
    case Gdk::Event::Type::SCROLL:
    {
      Graphic3d_Vec2d aDelta;
      theEvent->get_deltas(aDelta.x(), aDelta.y());

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
    {
      const Aspect_VKey aVKey = OcctGtkTools::gtkKey2VKey(theEvent->get_keyval(), theEvent->get_keycode());
      if (aVKey == Aspect_VKey_UNKNOWN)
        return false;

      const double aTimeStamp = AIS_ViewController::EventTime();
      if (theEvent->get_event_type() == Gdk::Event::Type::KEY_PRESS)
        AIS_ViewController::KeyDown(aVKey, aTimeStamp);
      else
        AIS_ViewController::KeyUp(aVKey, aTimeStamp);

      if (theEvent->get_event_type() == Gdk::Event::Type::KEY_PRESS)
        processKeyPress(aVKey);

      AIS_ViewController::ProcessInput();
      return true;
    }
    default:
      break;
  }

  return false;
}

// ================================================================
// Function : onModifiersChanged
// ================================================================
/*bool OcctGtkGLAreaViewer::onModifiersChanged(Gdk::ModifierType theType)
{
  /// TODO this doesn't work as expected
  (void)theType;
  //const Aspect_VKeyFlags anOldFlags = myKeyModifiers;
  //myKeyModifiers = OcctGtkTools::gtkModifiers2VKeys(theType);
  //return myKeyModifiers != anOldFlags;
  return false;
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
// Function : onKeyPressed
// ================================================================
bool OcctGtkGLAreaViewer::onKeyPressed(guint theKeyVal, guint theKeyCode, Gdk::ModifierType )
{
  const Aspect_VKey aVKey = OcctGtkTools::gtkKey2VKey(theKeyVal, theKeyCode);
  if (aVKey == Aspect_VKey_UNKNOWN)
    return false;

  const double aTimeStamp = AIS_ViewController::EventTime();
  AIS_ViewController::KeyDown(aVKey, aTimeStamp);
  updateModifiers();

  AIS_ViewController::ProcessInput();
  return true;
}

// ================================================================
// Function : onKeyReleased
// ================================================================
void OcctGtkGLAreaViewer::onKeyReleased(guint theKeyVal, guint theKeyCode, Gdk::ModifierType )
{
  const Aspect_VKey aVKey = OcctGtkTools::gtkKey2VKey(theKeyVal, theKeyCode);
  if (aVKey == Aspect_VKey_UNKNOWN)
    return;

  const double aTimeStamp = AIS_ViewController::EventTime();
  AIS_ViewController::KeyUp(aVKey, aTimeStamp);
  updateModifiers();

  AIS_ViewController::ProcessInput();
}

// ================================================================
// Function : onMotionMove
// ================================================================
void OcctGtkGLAreaViewer::onMotionMove(double theX, double theY)
{
  if (OcctGtkTools::gtkHandleMotionEvent(*this, myView, Graphic3d_Vec2d(theX, theY), myKeyModifiers))
    queue_draw();
}

// ================================================================
// Function : onMouseButtonPressed
// ================================================================
void OcctGtkGLAreaViewer::onMouseButtonPressed(int , double theX, double theY)
{
  if (OcctGtkTools::gtkHandleButtonPressedEvent(*this, myView, Graphic3d_Vec2d(theX, theY), myEventCtrlClick->get_current_button(), myKeyModifiers))
    queue_draw();
}

// ================================================================
// Function : onMouseButtonReleased
// ================================================================
void OcctGtkGLAreaViewer::onMouseButtonReleased(int , double theX, double theY)
{
  if (OcctGtkTools::gtkHandleButtonReleasedEvent(*this, myView, Graphic3d_Vec2d(theX, theY), myEventCtrlClick->get_current_button(), myKeyModifiers))
    queue_draw();
}

// ================================================================
// Function : onMouseScroll
// ================================================================
bool OcctGtkGLAreaViewer::onMouseScroll(double theDeltaX, double theDeltaY)
{
  if (OcctGtkTools::gtkHandleScrollEvent(*this, myView, Graphic3d_Vec2d(theDeltaX, theDeltaY)))
    queue_draw();

  return true;
}*/

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
  if (aLogicalSize.x() == 0 || aLogicalSize.y() == 0)
    return;

  try
  {
    OCC_CATCH_SIGNALS
    throw_if_error();

    Glib::RefPtr<Gdk::Surface> aGdkSurf = this->get_native()->get_surface();
#if ((GTKMM_MAJOR_VERSION == 4 && GTKMM_MINOR_VERSION >= 12) || GTKMM_MAJOR_VERSION >= 5)
    // fraction scale factor introduced by gtkmm 4.12
    myDevicePixelRatio = aGdkSurf->get_scale();
#else
    myDevicePixelRatio = get_scale_factor();
#endif
    initPixelScaleRatio();

    const bool isFirstInit = myView->Window().IsNull();
    Aspect_Drawable aNativeWin = 0;
#if defined(_WIN32)
    //
#else
    // Gtk::GLArea creates GLX drawable from Window, so that aGlCtx->Window() is not a Window
    //aNativeWin = aGlCtx->Window();
    aNativeWin = gdk_x11_surface_get_xid(aGdkSurf->gobj());
#endif
    const Graphic3d_Vec2i aViewSize = Graphic3d_Vec2i(Graphic3d_Vec2d(aLogicalSize) * myDevicePixelRatio + Graphic3d_Vec2d(0.5));
    if (!OcctGlTools::InitializeGlWindow(myView, aNativeWin, aViewSize, myDevicePixelRatio))
    {
      Gtk::MessageDialog* aMsg = new Gtk::MessageDialog("OpenGl_Context is unable to wrap OpenGL context", false, Gtk::MessageType::ERROR);
      aMsg->set_title("Critical error: 3D Viewer initialization failure");
      aMsg->signal_response().connect([aMsg](int) { delete aMsg; });
      aMsg->show();
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
    return true;
  }
  catch (const Gdk::GLError& theGlErr)
  {
    Message::SendFail() << "An error occurred in the render callback of the GLArea\n"
                        << theGlErr.domain() << "-" << theGlErr.code() << "-" << theGlErr.what() << "\n";
    return false;
  }
}
