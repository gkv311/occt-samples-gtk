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

// ================================================================
// Function : OcctGtkViewer
// Purpose  :
// ================================================================
OcctGtkViewer::OcctGtkViewer()
: myVBox (Gtk::Orientation::ORIENTATION_VERTICAL),
  myQuitButton ("Quit"),
  myRotAngle (0.0f)
{
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
  myGlCtx->DiagnosticInformation (aGlCapsDict, theIsBasic ? Graphic3d_DiagnosticInfo_Basic : Graphic3d_DiagnosticInfo_Complete);
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
  myGlCtx = new OpenGl_Context();
  if (!myGlCtx->Init (true))
  {
    Message::SendFail() << "Error: Unable to wrap OpenGL context";
  }
  dumpGlInfo (false);
  try
  {
    myGLArea.throw_if_error();
  }
  catch (const Gdk::GLError& theGlErr)
  {
    Message::SendFail() << "An error occured making the context current during realize:\n"
                        << theGlErr.domain() << "-" << theGlErr.code() << "-" << theGlErr.what();
  }
}

// ================================================================
// Function : onGlAreaReleased
// Purpose  :
// ================================================================
void OcctGtkViewer::onGlAreaReleased()
{
  myGLArea.make_current();
  myGlCtx.Nullify();
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
    if (myGlCtx.IsNull())
    {
      return false;
    }

    float aVal = myRotAngle / 360.0f;
    myGlCtx->core11fwd->glClearColor (aVal, aVal, aVal, 1.0);
    myGlCtx->core11fwd->glClear (GL_COLOR_BUFFER_BIT);
    myGlCtx->core11fwd->glFlush();

    return true;
  }
  catch (const Gdk::GLError& theGlErr)
  {
    Message::SendFail() << "An error occurred in the render callback of the GLArea\n"
                        << theGlErr.domain() << "-" << theGlErr.code() << "-" << theGlErr.what() << "\n";
    return false;
  }
}
