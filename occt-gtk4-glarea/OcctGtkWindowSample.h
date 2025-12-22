// Copyright (c) 2023 Kirill Gavrilov

#ifndef _OcctGtkWindowSample_HeaderFile
#define _OcctGtkWindowSample_HeaderFile

#include "OcctGtkGLAreaViewer.h"

//! GTK window widget with embedded OCCT Viewer.
class OcctGtkWindowSample : public Gtk::Window
{
public:

  //! Main constructor.
  OcctGtkWindowSample();

  //! Destructor.
  virtual ~OcctGtkWindowSample();

protected:

  //! About button clicked event.
  void onAboutClicked();

  //! Value changed event.
  void onValueChanged(const Glib::RefPtr<Gtk::Adjustment>& theAdj);

  //! Forward modifiers change event to Viewer.
  bool onModifiersChanged(Gdk::ModifierType ) { return myEventCtrlKey->forward(myViewer); }

  //! Forward key pressed event to Viewer.
  bool onKeyPressed(guint , guint , Gdk::ModifierType ) { return myEventCtrlKey->forward(myViewer); }

  //! Forward key released event to Viewer.
  void onKeyReleased(guint , guint , Gdk::ModifierType ) { myEventCtrlKey->forward(myViewer); }

protected:

  Gtk::Box    myVBox;
  Gtk::Box    myControls;
  Gtk::Button myAboutButton;
  Gtk::Button myQuitButton;

  OcctGtkGLAreaViewer myViewer;

  Glib::RefPtr<Gtk::EventControllerKey> myEventCtrlKey;

};

#endif // _OcctGtkWindowSample_HeaderFile
