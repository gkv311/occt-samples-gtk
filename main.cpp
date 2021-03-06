// Copyright (c) 2021 OPEN CASCADE SAS
//
// This file is part of the examples of the Open CASCADE Technology software library.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE

#include "OcctGtkViewer.hxx"

#include <OSD.hxx>
#include <OSD_Environment.hxx>

#include <gtkmm.h>

int main (int theNbArgs, char* theArgVec[])
{
  OSD::SetSignal (false);
  OSD::SetSignalStackTraceLength (10);

  // force X11 backend for OpenGL initialization using GLX
  // (should be done in sync with OCCT configuration)
  OSD_Environment aBackend ("GDK_BACKEND");
  aBackend.SetValue ("x11");
  aBackend.Build();

  Glib::RefPtr<Gtk::Application> aGtkApp = Gtk::Application::create (theNbArgs, theArgVec, "org.opencascade.gtkexample");

  OcctGtkViewer aGtkWin;
  aGtkWin.show_all();
  return aGtkApp->run (aGtkWin);
}
