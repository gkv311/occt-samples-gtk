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

//! FBO wrapper.
class OcctFboWrapper : public OpenGl_FrameBuffer
{
  DEFINE_STANDARD_RTTI_INLINE(OcctFboWrapper, OpenGl_FrameBuffer)
public:
  //! Empty constructor.
  OcctFboWrapper() {}

  //! Initialize FBO.
  bool InitWrapperLazy (const Handle(OpenGl_Context)& theCtx, GLuint theFboId)
  {
    if (myGlFBufferId == theFboId)
    {
      return true;
    }

    return InitWrapper (theCtx);
  }
};

// ================================================================
// Function : OcctGtkViewer
// Purpose  :
// ================================================================
OcctGtkViewer::OcctGtkViewer()
: myVBox (Gtk::Orientation::ORIENTATION_VERTICAL),
  myQuitButton ("Quit"),
  myRotAngle (0.0f)
{
  Handle(Aspect_DisplayConnection) aDisp = new Aspect_DisplayConnection();
  Handle(OpenGl_GraphicDriver) aDriver = new OpenGl_GraphicDriver (aDisp, false);
  aDriver->ChangeOptions().buffersNoSwap = true;
  aDriver->ChangeOptions().buffersOpaqueAlpha = true;
  aDriver->ChangeOptions().useSystemBuffer = false;

  // create viewer
  myViewer = new V3d_Viewer (aDriver);
  myViewer->SetDefaultBackgroundColor (Quantity_NOC_BLACK);
  //myViewer->SetDefaultBackgroundColor (Quantity_NOC_RED);
  myViewer->SetDefaultLights();
  myViewer->SetLightOn();

  // create AIS context
  myContext = new AIS_InteractiveContext (myViewer);

  myViewCube = new AIS_ViewCube();
  myViewCube->SetViewAnimation (myViewAnimation);
  myViewCube->SetFixedAnimationLoop (false);
  myViewCube->SetAutoStartAnimation (true);

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

  // Connect gl area signals
  myGLArea.signal_realize()  .connect (sigc::mem_fun (*this, &OcctGtkViewer::onGlAreaRealized));
  // Important that the unrealize signal calls our handler to clean up
  // GL resources _before_ the default unrealize handler is called (the "false")
  myGLArea.signal_unrealize().connect (sigc::mem_fun (*this, &OcctGtkViewer::onGlAreaReleased), false);
  myGLArea.signal_render()   .connect (sigc::mem_fun (*this, &OcctGtkViewer::onGlAreaRender), false);

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
  myRotAngle = theAdj->get_value();
  myGLArea.queue_draw();
}

// ================================================================
// Function : onGlAreaRealized
// Purpose  :
// ================================================================
void OcctGtkViewer::onGlAreaRealized()
{
  myGLArea.make_current();
  Graphic3d_Vec2i aViewSize (myGLArea.get_width(), myGLArea.get_height());

  Handle(OpenGl_Context) aGlCtx = new OpenGl_Context();
  if (!aGlCtx->Init (true))
  {
    Message::SendFail() << "Error: Unable to wrap OpenGL context";
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
      myViewer->Driver()->GetDisplayConnection()->Init ((Aspect_XDisplay* )aGlCtx->GetDisplay());

//::Window www = gdk_x11_drawable_get_xid (gobj());
//::Window www = gdk_x11_window_get_xid (gobj());
//::Window www = gdk_x11_window_get_xid (gtk_widget_get_window (myGLArea.gobj()));
::Window www = gdk_x11_window_get_xid (gtk_widget_get_window ((GtkWidget* )myGLArea.gobj()));
//::Window www = aGlCtx->Window();

      aWindow = new Aspect_NeutralWindow();
      aWindow->SetNativeHandle (www);
      aWindow->SetSize (aViewSize.x(), aViewSize.y());



Message::SendWarning() << "Window: " << aGlCtx->Window() << "; www: " << www << "; Ctx: " << aGlCtx->RenderingContext() << "; Size: " << aViewSize.x() << "x" << aViewSize.y()
                       << "; XServerVendor= " << XServerVendor ((Display* )aGlCtx->GetDisplay());
//XWindowAttributes aWinAttribs;
//XGetWindowAttributes ((Display* )aGlCtx->GetDisplay(), (::Window )aGlCtx->Window(), &aWinAttribs);
//XGetWindowAttributes ((Display* )aGlCtx->GetDisplay(), www, &aWinAttribs);

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
  /// TODO
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
  try
  {
    myGLArea.throw_if_error();

    // wrap FBO created by Qt
    Handle(OpenGl_GraphicDriver) aDriver = Handle(OpenGl_GraphicDriver)::DownCast (myContext->CurrentViewer()->Driver());
    const Handle(OpenGl_Context)& aGlCtx = aDriver->GetSharedContext();
    Handle(OcctFboWrapper) aDefaultFbo = Handle(OcctFboWrapper)::DownCast (aGlCtx->DefaultFrameBuffer());
    if (aDefaultFbo.IsNull())
    {
      aDefaultFbo = new OcctFboWrapper();
      aGlCtx->SetDefaultFrameBuffer (aDefaultFbo);
    }
    //if (myToResize)
    {
      //aDefaultFbo->Release (aGlCtx.get());
    }
    if (!aDefaultFbo->InitWrapper (aGlCtx))
    ///if (!aDefaultFbo->InitWrapperLazy (aGlCtx, defaultFramebufferObject()))
    {
      aDefaultFbo.Nullify();
      Message::DefaultMessenger()->Send ("Default FBO wrapper creation failed\n", Message_Fail);
    }

    //if (myToResize)
    {
      //myToResize = false;
      //myView->MustBeResized();
    }

    float aVal = myRotAngle / 360.0f;
    //myGlCtx->core11fwd->glClearColor (aVal, aVal, aVal, 1.0);
    //myGlCtx->core11fwd->glClear (GL_COLOR_BUFFER_BIT);
    //myGlCtx->core11fwd->glFlush();
    myView->Redraw();
    return true;
  }
  catch (const Gdk::GLError& theGlErr)
  {
    Message::SendFail() << "An error occurred in the render callback of the GLArea\n"
                        << theGlErr.domain() << "-" << theGlErr.code() << "-" << theGlErr.what() << "\n";
    return false;
  }
}
