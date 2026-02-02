// Copyright (c) 2023-2026 Kirill Gavrilov

#ifndef _OcctGtkGLAreaViewer_HeaderFile
#define _OcctGtkGLAreaViewer_HeaderFile

// workaround macros clash with Gtk::Collation::UNICODE (UNICODE is a standard macros used by WinAPI)
#ifdef UNICODE
#undef UNICODE
#endif
#include <gtkmm.h>
#include <gtkmm/eventcontrollerlegacy.h>

#include <AIS_InteractiveContext.hxx>
#include <AIS_ViewController.hxx>
#include <AIS_ViewCube.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>

//! GTK GLArea widget with embedded OCCT Viewer.
class OcctGtkGLAreaViewer : public Gtk::GLArea, public AIS_ViewController
{
public:
  //! Use 'modern' or 'legacy' (Gtk::EventControllerLegacy) input event controllers.
  //! Should be set before initialization.
  //!
  //! Gtk::EventControllerLegacy is much simpler to configure with the Viewer.
  //! Modern controllers do not provide raw multi-touch events (only controllers for dedicated gestures).
  //! - Bug in Gtk::EventControllerLegacy:
  //!   mouse events come with offset when client-side-decorations are enabled (GTK_CSD=1)
  //!   https://gitlab.gnome.org/GNOME/gtk/-/issues/7983
  static bool& ToUseModernInput();

public:

  //! Main constructor.
  explicit OcctGtkGLAreaViewer(bool theUseModernInput = ToUseModernInput());

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

protected: //! @name callbacks for modern-style controllers

  //! Connect to input events using 'modern' controllers (please check bugs).
  void addModernEventControllers();

  //! Update modifiers.
  void updateModifiers();

  //! Handle input key.
  void processKeyPress(Aspect_VKey theKey);

  //! Handle key pressed/released event from Gtk::EventControllerKey.
  bool onKey(guint theKeyVal, guint theKeyCode, Gdk::ModifierType theType, bool theIsPressed);

  //! Handle raw event.
  bool onRawEvent(const Glib::RefPtr<const Gdk::Event>& theEvent);

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
  guint                          myAnimationCallback = 0;

  Aspect_VKeyFlags myKeyModifiers = Aspect_VKeyFlags_NONE;

};

#endif // _OcctGtkGLAreaViewer_HeaderFile
