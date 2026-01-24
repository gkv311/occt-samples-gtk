// Copyright (c) 2023-2026 Kirill Gavrilov

#include "OcctGtkWindowSample.h"

#include "../occt-gtk-tools/OcctGtkTools.h"

#include <Message.hxx>
#include <OSD.hxx>
#include <OSD_Environment.hxx>

#include <gtkmm.h>

int main(int theNbArgs, char* theArgVec[])
{
  // remove parsed argument as Gtk::Application will complain on unknown arguments
  auto removeArgument = [&theNbArgs, &theArgVec](int& theArgIter)
  {
    if (theArgIter + 1 < theNbArgs)
      theArgVec[theArgIter] = theArgVec[theArgIter + 1];

    --theArgIter;
    --theNbArgs;
  };

  // handle application-specific arguments
  for (int anArgIter = 0; anArgIter < theNbArgs; ++anArgIter)
  {
    if (std::strcmp(theArgVec[anArgIter], "-v") == 0
     || std::strcmp(theArgVec[anArgIter], "--verbose") == 0)
    {
      // enable verbose messages from OCCT algorithms
      removeArgument(anArgIter);
      Message::DefaultMessenger()->Printers().First()->SetTraceLevel(Message_Trace);
    }
  }

  // guard signals to be thrown as OCCT C++ exceptions
  OSD::SetSignal(false); //OSD::SetSignalStackTraceLength(10);

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
