// Copyright (c) 2023 Kirill Gavrilov

#include "OcctGtkWindowSample.h"

#include "../occt-gtk-tools/OcctGtkTools.h"

#include <Message.hxx>
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
    Gtk::Application::create("org.opencascade.samples.gtkmm.glarea",
                             Gio::Application::Flags::NON_UNIQUE);

  return aGtkApp->make_window_and_run<OcctGtkWindowSample>(theNbArgs, theArgVec);
}
