// Copyright (c) 2023 Kirill Gavrilov

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
  bool onMouseMotion(GdkEventMotion* theEvent);

  //! Handle mouse button press event.
  bool onMouseButtonPressed(GdkEventButton* theEvent);

  //! Handle mouse button released event.
  bool onMouseButtonReleased(GdkEventButton* theEvent);

  //! Handle mouse scroll event.
  bool onMouseScroll(GdkEventScroll* theEvent);

protected:

  //! Allocate OpenGL resources.
  void onGlAreaRealized();

  //! Release OpenGL resources
  void onGlAreaReleased();

  //! Redraw viewer content.
  bool onGlAreaRender(const Glib::RefPtr<Gdk::GLContext>& theGlCtx);

  //! Value changed event.
  void onValueChanged(const Glib::RefPtr<Gtk::Adjustment>& theAdj);

protected:

  //! Print OpenGL context info.
  void dumpGlInfo(bool theIsBasic, bool theToPrint);

  //! Initialize pixel scale ratio.
  void initPixelScaleRatio();

  //! Handle view redraw.
  virtual void handleViewRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                const Handle(V3d_View)& theView) override;

protected:

  Handle(V3d_Viewer)             myViewer;
  Handle(V3d_View)               myView;
  Handle(AIS_InteractiveContext) myContext;
  Handle(AIS_ViewCube)           myViewCube;  //!< view cube object
  float                          myDevicePixelRatio = 1.0f; //!< device pixel ratio for handling high DPI displays

  Gtk::Box    myVBox;
  Gtk::GLArea myGLArea;
  Gtk::Box    myControls;
  Gtk::Button myQuitButton;

};

#endif // _OcctGtkViewer_HeaderFile
