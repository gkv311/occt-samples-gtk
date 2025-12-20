// Copyright (c) 2023 Kirill Gavrilov

#ifndef _OcctGtkGLAreaViewer_HeaderFile
#define _OcctGtkGLAreaViewer_HeaderFile

#include <AIS_InteractiveContext.hxx>
#include <AIS_ViewController.hxx>
#include <AIS_ViewCube.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>

#include <gtkmm.h>

//! GTK GLArea widget with embedded OCCT Viewer.
class OcctGtkGLAreaViewer : public Gtk::GLArea, public AIS_ViewController
{
public:

  //! Main constructor.
  OcctGtkGLAreaViewer();

  //! Destructor.
  virtual ~OcctGtkGLAreaViewer();

  //! Return Viewer.
  const Handle(V3d_Viewer)& Viewer() const { return myViewer; }

  //! Return View.
  const Handle(V3d_View)& View() const { return myView; }

  //! Return AIS context.
  const Handle(AIS_InteractiveContext)& Context() const { return myContext; }

  //! Return GL info.
  const TCollection_AsciiString& GetGlInfo() const { return myGlInfo; }

protected:

  //! Update modifiers.
  void updateModifiers();

  //! Handle modifiers changed event.
  bool onModifiersChanged(Gdk::ModifierType theType);

  //! Handle key pressed event.
  bool onKeyPressed(guint theKeyVal, guint theKeyCode, Gdk::ModifierType theType);

  //! Handle key released event.
  void onKeyReleased(guint theKeyVal, guint theKeyCode, Gdk::ModifierType theType);

  //! Handle mouse movement event.
  void onMotionMove(double theX, double theY);

  //! Handle mouse button press event.
  void onMouseButtonPressed(int theNbPressed, double theX, double theY);

  //! Handle mouse button released event.
  void onMouseButtonReleased(int theNbPressed, double theX, double theY);

  //! Handle mouse scroll event.
  bool onMouseScroll(double theDeltaX, double theDeltaY);

protected:

  //! Allocate OpenGL resources.
  void onGlAreaRealized();

  //! Release OpenGL resources
  void onGlAreaReleased();

  //! Redraw viewer content.
  bool onGlAreaRender(const Glib::RefPtr<Gdk::GLContext>& theGlCtx);

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
  TCollection_AsciiString        myGlInfo;

  Glib::RefPtr<Gtk::EventControllerKey> myEventCtrlKey;
  Glib::RefPtr<Gtk::GestureClick>       myEventCtrlClick;

  Aspect_VKeyFlags myKeyModifiers = Aspect_VKeyFlags_NONE;

};

#endif // _OcctGtkGLAreaViewer_HeaderFile
