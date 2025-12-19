// Copyright (c) 2023 Kirill Gavrilov

#include "OcctGtkViewer.h"

#include "../occt-gtk-tools/OcctGlTools.h"
#include "../occt-gtk-tools/OcctGtkTools.h"

#include <AIS_Shape.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
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
  #include <gdk/gdkx.h>
  #include <X11/Xlib.h>
#endif

// ================================================================
// Function : OcctGtkViewer
// ================================================================
OcctGtkViewer::OcctGtkViewer()
: myVBox(Gtk::Orientation::ORIENTATION_VERTICAL),
  myQuitButton("Quit")
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

  // widgets
  set_title("OCCT gtkmm Viewer sample");
  set_default_size(720, 480);

  myVBox.set_spacing(6);
  add(myVBox);

#ifdef HAVE_GLES2
  myGLArea.set_use_es(true);
#endif
  myGLArea.set_hexpand(true);
  myGLArea.set_vexpand(true);
  myGLArea.set_size_request(100, 200);
  myVBox.add(myGLArea);

  // connect to Gtk::GLArea events
  myGLArea.signal_realize()  .connect(sigc::mem_fun(*this, &OcctGtkViewer::onGlAreaRealized));
  // important that the unrealize signal calls our handler to clean up
  // GL resources _before_ the default unrealize handler is called (the "false")
  myGLArea.signal_unrealize().connect(sigc::mem_fun(*this, &OcctGtkViewer::onGlAreaReleased), false);
  myGLArea.signal_render()   .connect(sigc::mem_fun(*this, &OcctGtkViewer::onGlAreaRender), false);

  // connect to mouse input events
  myGLArea.add_events(Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK
                    | Gdk::SMOOTH_SCROLL_MASK  | Gdk::FOCUS_CHANGE_MASK);
  myGLArea.signal_motion_notify_event() .connect(sigc::mem_fun(*this, &OcctGtkViewer::onMouseMotion), false);
  myGLArea.signal_button_press_event()  .connect(sigc::mem_fun(*this, &OcctGtkViewer::onMouseButtonPressed), false);
  myGLArea.signal_button_release_event().connect(sigc::mem_fun(*this, &OcctGtkViewer::onMouseButtonReleased), false);
  myGLArea.signal_scroll_event()        .connect(sigc::mem_fun(*this, &OcctGtkViewer::onMouseScroll), false);

  // add dummy controls
  myVBox.add(myControls);
  myControls.set_hexpand(true);

  {
    Gtk::Box* aSliderBox = Gtk::manage(new Gtk::Box());

    Gtk::Label* aLabel = Gtk::manage(new Gtk::Label("Background"));
    aSliderBox->add(*aLabel);
    aLabel->show();

    Glib::RefPtr<Gtk::Adjustment> anAdj = Gtk::Adjustment::create(0.0, 0.0, 360.0, 1.0, 12.0, 0.0);
    anAdj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &OcctGtkViewer::onValueChanged), anAdj));

    Gtk::Scale* aSlider = Gtk::manage(new Gtk::Scale(anAdj));
    aSliderBox->add(*aSlider);
    aSlider->set_hexpand(true);
    aSlider->show();

    aSliderBox->show();
    myControls.add(*aSliderBox);
  }

  myQuitButton.set_hexpand(true);
  myQuitButton.signal_clicked().connect(sigc::mem_fun(*this, &Gtk::Window::close));
  myVBox.add(myQuitButton);
}

// ================================================================
// Function : ~OcctGtkViewer
// ================================================================
OcctGtkViewer::~OcctGtkViewer()
{
  //
}

// ================================================================
// Function : onValueChanged
// ================================================================
void OcctGtkViewer::onValueChanged(const Glib::RefPtr<Gtk::Adjustment>& theAdj)
{
  float aVal = theAdj->get_value() / 360.0f;
  myView->SetBackgroundColor(Quantity_Color(aVal, aVal, aVal, Quantity_TOC_sRGB));
  myView->Invalidate();
  myGLArea.queue_draw();
}

// ================================================================
// Function : onMouseMotion
// ================================================================
bool OcctGtkViewer::onMouseMotion(GdkEventMotion* theEvent)
{
  if (OcctGtkTools::gtkHandleMotionEvent(*this, myView, theEvent))
    myGLArea.queue_draw();

  return true;
}

// ================================================================
// Function : onMouseButtonPressed
// ================================================================
bool OcctGtkViewer::onMouseButtonPressed(GdkEventButton* theEvent)
{
  if (OcctGtkTools::gtkHandleButtonPressedEvent(*this, myView, theEvent))
    myGLArea.queue_draw();

  return true;
}

// ================================================================
// Function : onMouseButtonReleased
// ================================================================
bool OcctGtkViewer::onMouseButtonReleased(GdkEventButton* theEvent)
{
  if (OcctGtkTools::gtkHandleButtonReleasedEvent(*this, myView, theEvent))
    myGLArea.queue_draw();

  return true;
}

// ================================================================
// Function : onMouseScroll
// ================================================================
bool OcctGtkViewer::onMouseScroll(GdkEventScroll* theEvent)
{
  if (OcctGtkTools::gtkHandleScrollEvent(*this, myView, theEvent))
    myGLArea.queue_draw();

  return true;
}

// ================================================================
// Function : handleViewRedraw
// ================================================================
void OcctGtkViewer::handleViewRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                     const Handle(V3d_View)& theView)
{
  AIS_ViewController::handleViewRedraw(theCtx, theView);
  if (myToAskNextFrame)
    myGLArea.queue_draw(); // ask more frames
}

// ================================================================
// Function : initPixelScaleRatio
// ================================================================
void OcctGtkViewer::initPixelScaleRatio()
{
  SetTouchToleranceScale(myDevicePixelRatio);
  myView->ChangeRenderingParams().Resolution = (unsigned int )(96.0 * myDevicePixelRatio + 0.5);
}

// ================================================================
// Function : dumpGlInfo
// ================================================================
void OcctGtkViewer::dumpGlInfo(bool theIsBasic, bool theToPrint)
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

  if (theToPrint)
    Message::SendInfo(anInfo);
}

// ================================================================
// Function : onGlAreaRealized
// ================================================================
void OcctGtkViewer::onGlAreaRealized()
{
  myGLArea.make_current();
  Graphic3d_Vec2i aLogicalSize(myGLArea.get_width(), myGLArea.get_height());

  try
  {
    OCC_CATCH_SIGNALS
    myGLArea.throw_if_error();

    // FBO is not yet initialized?
    const Graphic3d_Vec2i aViewSize = aLogicalSize * myGLArea.get_scale_factor();
    myDevicePixelRatio = float(aViewSize.y()) / float(aLogicalSize.y());
    initPixelScaleRatio();

    const bool isFirstInit = myView->Window().IsNull();
    Aspect_Drawable aDrawable = 0;
#ifdef HAVE_GLES2
    //
#elif defined(_WIN32)
    //
#else
    // Gtk::GLArea creates GLX drawable from Window, so that aGlCtx->Window() is not a Window
    //aDrawable = aGlCtx->Window();
    aDrawable = gdk_x11_window_get_xid(gtk_widget_get_window((GtkWidget* )myGLArea.gobj()));
#endif
    if (!OcctGlTools::InitializeGlWindow(myView, aDrawable, aViewSize, myDevicePixelRatio))
    {
      Gtk::MessageDialog aMsg("Error: OpenGl_Context is unable to wrap OpenGL context", false, Gtk::MESSAGE_ERROR);
      aMsg.run();
    }
    myGLArea.make_current();

    dumpGlInfo(true, true);
    if (isFirstInit)
    {
      myContext->Display(myViewCube, 0, 0, false);

      // dummy shape for testing
      TopoDS_Shape aBox = BRepPrimAPI_MakeBox(100.0, 50.0, 90.0).Shape();
      Handle(AIS_Shape) aShape = new AIS_Shape(aBox);
      myContext->Display(aShape, AIS_Shaded, 0, false);
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
void OcctGtkViewer::onGlAreaReleased()
{
  myGLArea.make_current();
  try
  {
    myGLArea.throw_if_error();

    // release OCCT viewer on application close
    Handle(Aspect_DisplayConnection) aDisp;
    if (!myContext.IsNull())
    {
      aDisp = myViewer->Driver()->GetDisplayConnection();
      myContext->RemoveAll(false);
      myContext.Nullify();
      myView->Remove();
      myView.Nullify();
      myViewer.Nullify();
    }

    myGLArea.make_current();
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
bool OcctGtkViewer::onGlAreaRender(const Glib::RefPtr<Gdk::GLContext>& theGlCtx)
{
  (void )theGlCtx;
  if (myView->Window().IsNull())
    return false;

  try
  {
    myGLArea.throw_if_error();

    // wrap FBO created by Gtk::GLArea
    if (!OcctGlTools::InitializeGlFbo(myView))
    {
      Gtk::MessageDialog aMsg("Default FBO wrapper creation failed", false, Gtk::MESSAGE_ERROR);
      aMsg.run();
      return false;
    }

    // calculate pixel ratio between OpenGL FBO viewport dimension and Gtk::GLArea logical size
    const Graphic3d_Vec2i aLogicalSize(myGLArea.get_width(), myGLArea.get_height());
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
    FlushViewEvents(myContext, myView, true);
    return true;
  }
  catch (const Gdk::GLError& theGlErr)
  {
    Message::SendFail() << "An error occurred in the render callback of the GLArea\n"
                        << theGlErr.domain() << "-" << theGlErr.code() << "-" << theGlErr.what() << "\n";
    return false;
  }
}
