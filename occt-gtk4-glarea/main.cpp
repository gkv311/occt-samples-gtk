// Copyright (c) 2023 Kirill Gavrilov

#include "OcctGtkWindowSample.h"

#include "../occt-gtk-tools/OcctGtkTools.h"

#include <Message.hxx>
#include <Message.hxx>
#include <OSD.hxx>
#include <OSD_Environment.hxx>

#include <gtkmm.h>

int main(int theNbArgs, char* theArgVec[])
{
  OSD::SetSignal(false);
  //OSD::SetSignalStackTraceLength(10);

  for (int anArgIter = 0; anArgIter < theNbArgs; ++anArgIter)
  {
    // enable verbose messages from OCCT algorithms
    if (std::strcmp(theArgVec[anArgIter], "-v") == 0
     || std::strcmp(theArgVec[anArgIter], "--verbose") == 0)
    {
      if (anArgIter + 1 < theNbArgs)
        theArgVec[anArgIter] = theArgVec[anArgIter + 1];

      --anArgIter;
      --theNbArgs;
      Message::DefaultMessenger()->Printers().First()->SetTraceLevel(Message_Trace);
    }
  }

  // force X11 backend for OpenGL initialization using GLX
  // (should be done in sync with OCCT configuration)
  OcctGtkTools::gtkGlPlatformSetup();

  Glib::RefPtr<Gtk::Application> aGtkApp =
    Gtk::Application::create("org.opencascade.samples.gtkmm.glarea",
                             Gio::Application::Flags::NON_UNIQUE);

  return aGtkApp->make_window_and_run<OcctGtkWindowSample>(theNbArgs, theArgVec);
}
