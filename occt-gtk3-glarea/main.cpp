// Copyright (c) 2023 Kirill Gavrilov

#include "OcctGtkWindowSample.h"

#include "../occt-gtk-tools/OcctGtkTools.h"

#include <OSD.hxx>
#include <OSD_Environment.hxx>

#include <gtkmm.h>

int main(int theNbArgs, char* theArgVec[])
{
  OSD::SetSignal(false);
  //OSD::SetSignalStackTraceLength(10);

  // force X11 backend for OpenGL initialization using GLX
  // (should be done in sync with OCCT configuration)
  OcctGtkTools::gtkGlPlatformSetup();

  Glib::RefPtr<Gtk::Application> aGtkApp =
    Gtk::Application::create(theNbArgs, theArgVec,
                             "org.opencascade.samples.gtkmm.glarea",
                             Gio::APPLICATION_NON_UNIQUE);

  OcctGtkWindowSample aGtkWin;
  aGtkWin.show_all();
  return aGtkApp->run(aGtkWin);
}
