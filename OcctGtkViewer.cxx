// Copyright (c) 2021 OPEN CASCADE SAS
//
// This file is part of the examples of the Open CASCADE Technology software library.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE

#include "OcctGtkViewer.hxx"

#include <Message.hxx>
#include <OpenGl_Context.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <OpenGl_FrameBuffer.hxx>

#include <gdk/gdkx.h>
#include <X11/Xlib.h>

#include <EGL/egl.h>

// ================================================================
// Function : OcctGtkViewer
// Purpose  :
// ================================================================
OcctGtkViewer::OcctGtkViewer()
: myVBox (Gtk::Orientation::ORIENTATION_VERTICAL),
  myQuitButton ("Quit")
{
  Handle(Aspect_DisplayConnection) aDisp = new Aspect_DisplayConnection();
  Handle(OpenGl_GraphicDriver) aDriver = new OpenGl_GraphicDriver (aDisp, false);
  // lets Gtk::GLArea to manage buffer swap
  aDriver->ChangeOptions().buffersNoSwap = true;
  // don't write into alpha channel
  aDriver->ChangeOptions().buffersOpaqueAlpha = true;
  // offscreen FBOs should be always used
  aDriver->ChangeOptions().useSystemBuffer = false;

  // create viewer
  myViewer = new V3d_Viewer (aDriver);
  myViewer->SetDefaultBackgroundColor (Quantity_NOC_BLACK);
  myViewer->SetDefaultLights();
  myViewer->SetLightOn();
  myViewer->ActivateGrid (Aspect_GT_Rectangular, Aspect_GDM_Lines);

  // create AIS context
  myContext = new AIS_InteractiveContext (myViewer);

  myViewCube = new AIS_ViewCube();
  myViewCube->SetViewAnimation (myViewAnimation);
  myViewCube->SetFixedAnimationLoop (false);
  myViewCube->SetAutoStartAnimation (true);

  // note - window will be created later within onGlAreaRealized() callback!
  myView = myViewer->CreateView();
  myView->SetImmediateUpdate (false);
  myView->ChangeRenderingParams().ToShowStats = true;
  myView->ChangeRenderingParams().CollectedStats = (Graphic3d_RenderingParams::PerfCounters )
    (Graphic3d_RenderingParams::PerfCounters_FrameRate
   | Graphic3d_RenderingParams::PerfCounters_Triangles);

  // widgets
  set_title ("OCCT gtkmm Viewer sample");
  set_default_size (720, 480);

  myVBox.set_spacing (6);
  add (myVBox);

  myGLArea.set_hexpand (true);
  myGLArea.set_vexpand (true);
  myGLArea.set_size_request (100, 200);
  myVBox.add (myGLArea);

  // connect to Gtk::GLArea events
  myGLArea.signal_realize()  .connect (sigc::mem_fun (*this, &OcctGtkViewer::onGlAreaRealized));
  // important that the unrealize signal calls our handler to clean up
  // GL resources _before_ the default unrealize handler is called (the "false")
  myGLArea.signal_unrealize().connect (sigc::mem_fun (*this, &OcctGtkViewer::onGlAreaReleased), false);
  myGLArea.signal_render()   .connect (sigc::mem_fun (*this, &OcctGtkViewer::onGlAreaRender), false);

  // connect to mouse input events
  myGLArea.add_events (Gdk::POINTER_MOTION_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK
                     | Gdk::SMOOTH_SCROLL_MASK  | Gdk::FOCUS_CHANGE_MASK);
  myGLArea.signal_motion_notify_event() .connect (sigc::mem_fun (*this, &OcctGtkViewer::onMouseMotion), false);
  myGLArea.signal_button_press_event()  .connect (sigc::mem_fun (*this, &OcctGtkViewer::onMouseButtonPressed), false);
  myGLArea.signal_button_release_event().connect (sigc::mem_fun (*this, &OcctGtkViewer::onMouseButtonReleased), false);
  myGLArea.signal_scroll_event()        .connect (sigc::mem_fun (*this, &OcctGtkViewer::onMouseScroll), false);

  // add dummy controls
  myVBox.add (myControls);
  myControls.set_hexpand (true);

  {
    Gtk::Box* aSliderBox = Gtk::manage (new Gtk::Box());

    Gtk::Label* aLabel = Gtk::manage (new Gtk::Label ("Background"));
    aSliderBox->add (*aLabel);
    aLabel->show();

    Glib::RefPtr<Gtk::Adjustment> anAdj = Gtk::Adjustment::create (0.0, 0.0, 360.0, 1.0, 12.0, 0.0);
    anAdj->signal_value_changed().connect (sigc::bind (sigc::mem_fun (*this, &OcctGtkViewer::onValueChanged), anAdj));

    Gtk::Scale* aSlider = Gtk::manage (new Gtk::Scale (anAdj));
    aSliderBox->add (*aSlider);
    aSlider->set_hexpand (true);
    aSlider->show();

    aSliderBox->show();
    myControls.add (*aSliderBox);
  }

  myQuitButton.set_hexpand (true);
  myQuitButton.signal_clicked().connect (sigc::mem_fun (*this, &Gtk::Window::close));
  myVBox.add (myQuitButton);
}

// ================================================================
// Function : ~OcctGtkViewer
// Purpose  :
// ================================================================
OcctGtkViewer::~OcctGtkViewer()
{
  //
}

// ================================================================
// Function : dumpGlInfo
// Purpose  :
// ================================================================
void OcctGtkViewer::dumpGlInfo (bool theIsBasic)
{
  TColStd_IndexedDataMapOfStringString aGlCapsDict;
  myView->DiagnosticInformation (aGlCapsDict, theIsBasic ? Graphic3d_DiagnosticInfo_Basic : Graphic3d_DiagnosticInfo_Complete);
  if (theIsBasic)
  {
    TCollection_AsciiString aViewport;
    aGlCapsDict.FindFromKey ("Viewport", aViewport);
    aGlCapsDict.Clear();
    aGlCapsDict.Add ("Viewport", aViewport);
  }

  // beautify output
  {
    TCollection_AsciiString* aGlVer   = aGlCapsDict.ChangeSeek ("GLversion");
    TCollection_AsciiString* aGlslVer = aGlCapsDict.ChangeSeek ("GLSLversion");
    if (aGlVer   != NULL
     && aGlslVer != NULL)
    {
      *aGlVer = *aGlVer + " [GLSL: " + *aGlslVer + "]";
      aGlslVer->Clear();
    }
  }

  TCollection_AsciiString anInfo;
  for (TColStd_IndexedDataMapOfStringString::Iterator aValueIter (aGlCapsDict); aValueIter.More(); aValueIter.Next())
  {
    if (!aValueIter.Value().IsEmpty())
    {
      if (!anInfo.IsEmpty())
      {
        anInfo += "\n";
      }
      anInfo += aValueIter.Key() + ": " + aValueIter.Value();
    }
  }

  Message::SendWarning (anInfo);
}

// ================================================================
// Function : onValueChanged
// Purpose  :
// ================================================================
void OcctGtkViewer::onValueChanged (const Glib::RefPtr<Gtk::Adjustment>& theAdj)
{
  float aVal = theAdj->get_value() / 360.0f;
  myView->SetBackgroundColor (Quantity_Color (aVal, aVal, aVal, Quantity_TOC_sRGB));
  myView->Invalidate();
  myGLArea.queue_draw();
}

//! Convert Gtk event mouse button to Aspect_VKeyMouse.
static Aspect_VKeyMouse mouseButtonFromGtk (guint theButton)
{
  switch (theButton)
  {
    case 1: return Aspect_VKeyMouse_LeftButton;
    case 2: return Aspect_VKeyMouse_MiddleButton;
    case 3: return Aspect_VKeyMouse_RightButton;
  }
  return Aspect_VKeyMouse_NONE;
}

//! Convert Gtk event mouse flags to Aspect_VKeyFlags.
static Aspect_VKeyFlags mouseFlagsFromGtk (guint theFlags)
{
  Aspect_VKeyFlags aFlags = Aspect_VKeyFlags_NONE;
  if ((theFlags & GDK_SHIFT_MASK) != 0)
  {
    aFlags |= Aspect_VKeyFlags_SHIFT;
  }
  if ((theFlags & GDK_CONTROL_MASK) != 0)
  {
    aFlags |= Aspect_VKeyFlags_CTRL;
  }
  if ((theFlags & GDK_META_MASK) != 0)
  {
    aFlags |= Aspect_VKeyFlags_META;
  }
  if ((theFlags & GDK_MOD1_MASK) != 0)
  {
    aFlags |= Aspect_VKeyFlags_ALT;
  }
  return aFlags;
}

// ================================================================
// Function : onMouseMotion
// Purpose  :
// ================================================================
bool OcctGtkViewer::onMouseMotion (GdkEventMotion* theEvent)
{
  const bool isEmulated = false;
  const Aspect_VKeyMouse aButtons = PressedMouseButtons();
  const Graphic3d_Vec2d aNewPos2d (theEvent->x, theEvent->y);
  const Graphic3d_Vec2i aNewPos2i = Graphic3d_Vec2i (aNewPos2d + Graphic3d_Vec2d (0.5));
  const Aspect_VKeyFlags aFlags = mouseFlagsFromGtk (theEvent->state);
  if (UpdateMousePosition (aNewPos2i, aButtons, aFlags, isEmulated))
  {
    myGLArea.queue_draw();
  }
  return true;
}

// ================================================================
// Function : onMouseButtonPressed
// Purpose  :
// ================================================================
bool OcctGtkViewer::onMouseButtonPressed (GdkEventButton* theEvent)
{
  const bool isEmulated = false;
  const Aspect_VKeyMouse aButton = mouseButtonFromGtk (theEvent->button);
  if (aButton == Aspect_VKeyMouse_NONE)
  {
    return false;
  }
  const Graphic3d_Vec2d aNewPos2d (theEvent->x, theEvent->y);
  const Graphic3d_Vec2i aNewPos2i = Graphic3d_Vec2i (aNewPos2d + Graphic3d_Vec2d (0.5));
  const Aspect_VKeyFlags aFlags = mouseFlagsFromGtk (theEvent->state);
  if (PressMouseButton (aNewPos2i, aButton, aFlags, isEmulated))
  {
    myGLArea.queue_draw();
  }
  return true;
}

// ================================================================
// Function : onMouseButtonReleased
// Purpose  :
// ================================================================
bool OcctGtkViewer::onMouseButtonReleased (GdkEventButton* theEvent)
{
  const bool isEmulated = false;
  const Aspect_VKeyMouse aButton = mouseButtonFromGtk (theEvent->button);
  if (aButton == Aspect_VKeyMouse_NONE)
  {
    return false;
  }
  const Graphic3d_Vec2d aNewPos2d (theEvent->x, theEvent->y);
  const Graphic3d_Vec2i aNewPos2i = Graphic3d_Vec2i (aNewPos2d + Graphic3d_Vec2d (0.5));
  const Aspect_VKeyFlags aFlags = mouseFlagsFromGtk (theEvent->state);
  if (ReleaseMouseButton (aNewPos2i, aButton, aFlags, isEmulated))
  {
    myGLArea.queue_draw();
  }
  return true;
}

// ================================================================
// Function : onMouseScroll
// Purpose  :
// ================================================================
bool OcctGtkViewer::onMouseScroll (GdkEventScroll* theEvent)
{
  const Graphic3d_Vec2d aNewPos2d (theEvent->x, theEvent->y);
  const Aspect_ScrollDelta aScroll (Graphic3d_Vec2i (aNewPos2d + Graphic3d_Vec2d (0.5)), -theEvent->delta_y);
  if (UpdateMouseScroll (aScroll))
  {
    myGLArea.queue_draw();
  }
  return true;
}

// ================================================================
// Function : onGlAreaRealized
// Purpose  :
// ================================================================
void OcctGtkViewer::onGlAreaRealized()
{
  myGLArea.make_current();
  Graphic3d_Vec2i aViewSize (myGLArea.get_width(), myGLArea.get_height());

  void* aEglCtx = eglGetCurrentContext();
  if (aEglCtx != EGL_NO_CONTEXT)
  {
    Message::SendFail() << "Error: Wayland session (EGL context) is not unsuppored";
    Gtk::MessageDialog aMsg ("Error: Wayland session (EGL context) is not unsuppored", false, Gtk::MESSAGE_ERROR);
    aMsg.run();
    return;
  }

  Handle(OpenGl_Context) aGlCtx = new OpenGl_Context();
  if (!aGlCtx->Init (true))
  {
    Message::SendFail() << "Error: OpenGl_Context is unable to wrap OpenGL context";
    Gtk::MessageDialog aMsg ("Error: OpenGl_Context is unable to wrap OpenGL context", false, Gtk::MESSAGE_ERROR);
    aMsg.run();
    return;
  }

  try
  {
    OCC_CATCH_SIGNALS
    myGLArea.throw_if_error();

    Handle(Aspect_NeutralWindow) aWindow = Handle(Aspect_NeutralWindow)::DownCast (myView->Window());
    if (!aWindow.IsNull())
    {
      aWindow->SetSize (aViewSize.x(), aViewSize.y());
      myView->SetWindow (aWindow, aGlCtx->RenderingContext());
      dumpGlInfo (true);
    }
    else
    {
      // XDisplay could be wrapped, but OCCT may use a dedicated connection
      //myViewer->Driver()->GetDisplayConnection()->Init ((Aspect_XDisplay* )aGlCtx->GetDisplay());

      // Gtk::GLArea creates GLX drawable from Window, so that aGlCtx->Window() is not a Window
      //::Window anXWin = aGlCtx->Window();
      ::Window anXWin = gdk_x11_window_get_xid (gtk_widget_get_window ((GtkWidget* )myGLArea.gobj()));
      aWindow = new Aspect_NeutralWindow();
      aWindow->SetNativeHandle (anXWin);
      aWindow->SetSize (aViewSize.x(), aViewSize.y());
      myView->SetWindow (aWindow, aGlCtx->RenderingContext());
      dumpGlInfo (false);

      myContext->Display (myViewCube, 0, 0, false);
    }
  }
  catch (const Gdk::GLError& theGlErr)
  {
    Message::SendFail() << "An error occured making the context current during realize:\n"
                        << theGlErr.domain() << "-" << theGlErr.code() << "-" << theGlErr.what();
  }
  catch (const Standard_Failure& theErr)
  {
    Message::SendFail() << "An error occured making the context current during realize:\n"
                        << theErr;
  }
}

// ================================================================
// Function : onGlAreaReleased
// Purpose  :
// ================================================================
void OcctGtkViewer::onGlAreaReleased()
{
  myGLArea.make_current();
  /// TODO gracefully release OCCT viewer on application close
  /// myView.Nullify();
  try
  {
    myGLArea.throw_if_error();
  }
  catch (const Gdk::GLError& theGlErr)
  {
    Message::SendFail() << "An error occured making the context current during unrealize\n"
                        << theGlErr.domain() << "-" << theGlErr.code() << "-" << theGlErr.what() << "\n";
  }
}

// ================================================================
// Function : onGlAreaRender
// Purpose  :
// ================================================================
bool OcctGtkViewer::onGlAreaRender (const Glib::RefPtr<Gdk::GLContext>& theGlCtx)
{
  (void )theGlCtx;
  if (myView->Window().IsNull())
  {
    return false;
  }

  try
  {
    myGLArea.throw_if_error();

    // wrap FBO created by Gtk::GLArea
    Handle(OpenGl_GraphicDriver) aDriver = Handle(OpenGl_GraphicDriver)::DownCast (myContext->CurrentViewer()->Driver());
    const Handle(OpenGl_Context)& aGlCtx = aDriver->GetSharedContext();
    Handle(OpenGl_FrameBuffer) aDefaultFbo = aGlCtx->DefaultFrameBuffer();
    if (aDefaultFbo.IsNull())
    {
      aDefaultFbo = new OpenGl_FrameBuffer();
      aGlCtx->SetDefaultFrameBuffer (aDefaultFbo);
    }
    if (!aDefaultFbo->InitWrapper (aGlCtx))
    {
      aDefaultFbo.Nullify();
      Message::DefaultMessenger()->Send ("Default FBO wrapper creation failed\n", Message_Fail);
      return false;
    }

    Graphic3d_Vec2i aViewSizeOld;
    Graphic3d_Vec2i aViewSizeNew (myGLArea.get_width(), myGLArea.get_height());
    Handle(Aspect_NeutralWindow) aWindow = Handle(Aspect_NeutralWindow)::DownCast (myView->Window());
    aWindow->Size (aViewSizeOld.x(), aViewSizeOld.y());
    if (aViewSizeNew != aViewSizeOld)
    {
      aWindow->SetSize (aViewSizeNew.x(), aViewSizeNew.y());
      myView->MustBeResized();
      myView->Invalidate();
    }

    // flush pending input events and redraw the viewer
    FlushViewEvents (myContext, myView, true);
    return true;
  }
  catch (const Gdk::GLError& theGlErr)
  {
    Message::SendFail() << "An error occurred in the render callback of the GLArea\n"
                        << theGlErr.domain() << "-" << theGlErr.code() << "-" << theGlErr.what() << "\n";
    return false;
  }
}

// ================================================================
// Function : handleViewRedraw
// Purpose  :
// ================================================================
void OcctGtkViewer::handleViewRedraw (const Handle(AIS_InteractiveContext)& theCtx,
                                      const Handle(V3d_View)& theView)
{
  AIS_ViewController::handleViewRedraw (theCtx, theView);
  if (myToAskNextFrame)
  {
    // ask more frames
    myGLArea.queue_draw();
  }
}
