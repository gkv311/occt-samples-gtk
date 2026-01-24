// Copyright (c) 2023-2026 Kirill Gavrilov

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

protected:

  Gtk::Box    myVBox;
  Gtk::Box    myControls;
  Gtk::Button myAboutButton;
  Gtk::Button myQuitButton;

  OcctGtkGLAreaViewer myViewer;

};

#endif // _OcctGtkWindowSample_HeaderFile
