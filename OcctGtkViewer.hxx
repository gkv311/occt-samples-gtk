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

#ifndef _OcctGtkViewer_HeaderFile
#define _OcctGtkViewer_HeaderFile

#include <gtkmm.h>

#include <vector>

//! GTK window widget with embedded OCCT Viewer.
class OcctGtkViewer : public Gtk::Window
{
public:

  //! Main constructor.
  OcctGtkViewer();

  //! Destructor.
  virtual ~OcctGtkViewer();

protected:

  //! Allocate OpenGL resources.
  void onGlAreaRealized();

  //! Release OpenGL resources
  void onGlAreaReleased();

  //! Redraw viewer content.
  bool onGlAreaRender (const Glib::RefPtr<Gdk::GLContext>& theGlCtx);

  //! Value changed event.
  void onValueChanged (const Glib::RefPtr<Gtk::Adjustment>& theAdj);

protected:

  Gtk::Box    myVBox;
  Gtk::GLArea myGLArea;
  Gtk::Box    myControls;
  Gtk::Button myQuitButton;
  float       myRotAngle;

};

#endif // _OcctGtkViewer_HeaderFile
