// Copyright (c) 2023-2026 Kirill Gavrilov

#include "OcctGtkWindowSample.h"

#include "../occt-gtk-tools/OcctGtkTools.h"

#include <Message.hxx>
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
    else if (std::strcmp(theArgVec[anArgIter], "--modern") == 0
          || std::strcmp(theArgVec[anArgIter], "--moderninput") == 0)
    {
      removeArgument(anArgIter);
      OcctGtkGLAreaViewer::ToUseModernInput() = true;
    }
    else if (std::strcmp(theArgVec[anArgIter], "--legacy") == 0
          || std::strcmp(theArgVec[anArgIter], "--legacyinput") == 0)
    {
      removeArgument(anArgIter);
      OcctGtkGLAreaViewer::ToUseModernInput() = false;
    }
  }

  // guard signals to be thrown as OCCT C++ exceptions
  OSD::SetSignal(false); //OSD::SetSignalStackTraceLength(10);

  // force X11 backend (GDK_BACKEND=x11) for OpenGL initialization using GLX;
  // should be done in sync with OCCT configurations
  OcctGtkTools::gtkGlPlatformSetup();

  Glib::RefPtr<Gtk::Application> aGtkApp =
    Gtk::Application::create("org.opencascade.samples.gtkmm.glarea",
                             Gio::Application::Flags::NON_UNIQUE);

  return aGtkApp->make_window_and_run<OcctGtkWindowSample>(theNbArgs, theArgVec);
}
