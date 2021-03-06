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

#include <AIS_InteractiveContext.hxx>
#include <AIS_ViewController.hxx>
#include <AIS_ViewCube.hxx>
#include <Aspect_NeutralWindow.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>

#include <gtkmm.h>

#include <vector>

class Aspect_NeutralWindow;
class OpenGl_Context;

//! GTK window widget with embedded OCCT Viewer.
class OcctGtkViewer : public Gtk::Window, public AIS_ViewController
{
public:

  //! Main constructor.
  OcctGtkViewer();

  //! Destructor.
  virtual ~OcctGtkViewer();

protected:

  //! Handle mouse movement event.
  bool onMouseMotion (GdkEventMotion* theEvent);

  //! Handle mouse button press event.
  bool onMouseButtonPressed (GdkEventButton* theEvent);

  //! Handle mouse button released event.
  bool onMouseButtonReleased (GdkEventButton* theEvent);

  //! Handle mouse scroll event.
  bool onMouseScroll (GdkEventScroll* theEvent);

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

  //! Print OpenGL context info.
  void dumpGlInfo (bool theIsBasic);

  //! Initialize pixel scale ratio.
  void initPixelScaleRatio();

  //! Handle view redraw.
  virtual void handleViewRedraw (const Handle(AIS_InteractiveContext)& theCtx,
                                 const Handle(V3d_View)& theView) override;

protected:

  Handle(V3d_Viewer)               myViewer;
  Handle(V3d_View)                 myView;
  Handle(AIS_InteractiveContext)   myContext;
  Handle(AIS_ViewCube)             myViewCube;  //!< view cube object
  float                            myDevicePixelRatio = 1.0f; //!< device pixel ratio for handling high DPI displays

  Gtk::Box    myVBox;
  Gtk::GLArea myGLArea;
  Gtk::Box    myControls;
  Gtk::Button myQuitButton;

};

#endif // _OcctGtkViewer_HeaderFile
