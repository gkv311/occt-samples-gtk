// Copyright (c) 2023-2026 Kirill Gavrilov

#include "OcctGtkWindowSample.h"

#include <AIS_Shape.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <Message.hxx>
#include <Standard_Version.hxx>

// ================================================================
// Function : OcctGtkWindowSample
// ================================================================
OcctGtkWindowSample::OcctGtkWindowSample()
: myVBox(Gtk::Orientation::ORIENTATION_VERTICAL),
  myAboutButton("About"),
  myQuitButton("Quit")
{
  set_title("OCCT Gtk3::GLArea Viewer sample");
  set_default_size(720, 480);

  myVBox.set_spacing(6);
  add(myVBox);

  myViewer.set_hexpand(true);
  myViewer.set_vexpand(true);
  myViewer.set_size_request(100, 200);
  myVBox.add(myViewer);
  myVBox.add(myControls);
  myControls.set_hexpand(true);
  {
    Gtk::Box* aSliderBox = Gtk::manage(new Gtk::Box());

    Gtk::Label* aLabel = Gtk::manage(new Gtk::Label("Background"));
    aSliderBox->add(*aLabel);
    aLabel->show();

    Glib::RefPtr<Gtk::Adjustment> anAdj = Gtk::Adjustment::create(0.0, 0.0, 360.0, 1.0, 12.0, 0.0);
    anAdj->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &OcctGtkWindowSample::onValueChanged), anAdj));

    Gtk::Scale* aSlider = Gtk::manage(new Gtk::Scale(anAdj));
    aSliderBox->add(*aSlider);
    aSlider->set_hexpand(true);
    aSlider->show();

    aSliderBox->show();
    myControls.add(*aSliderBox);

    myAboutButton.set_size_request(70);
    myAboutButton.signal_clicked().connect(sigc::mem_fun(*this, &OcctGtkWindowSample::onAboutClicked));
    myControls.add(myAboutButton);

    myQuitButton.set_size_request(70);
    myQuitButton.signal_clicked().connect(sigc::mem_fun(*this, &Gtk::Window::close));
    myControls.add(myQuitButton);
  }

  // display dummy shape for testing
  {
    TopoDS_Shape aBox = BRepPrimAPI_MakeBox(100.0, 50.0, 90.0).Shape();
    Handle(AIS_Shape) aShape = new AIS_Shape(aBox);
    myViewer.Context()->Display(aShape, AIS_Shaded, 0, false);
  }
}

// ================================================================
// Function : ~OcctGtkWindowSample
// ================================================================
OcctGtkWindowSample::~OcctGtkWindowSample()
{
  //
}

// ================================================================
// Function : onAboutClicked
// ================================================================
void OcctGtkWindowSample::onAboutClicked()
{
  std::stringstream anAbout;
  anAbout << "OCCT 3D Viewer sample embedded into Qt Widgets.\n"
          << "Open CASCADE Technology v." OCC_VERSION_STRING_EXT "\n"
          << "Gtkmm v." << GTKMM_MAJOR_VERSION << "." << GTKMM_MINOR_VERSION << "." << GTKMM_MICRO_VERSION << "\n"
          << "GTK v." << gtk_get_major_version() << "." << gtk_get_minor_version() << "." << gtk_get_micro_version();

  TCollection_AsciiString aGlInfo = TCollection_AsciiString() + "OpenGL info\n" + myViewer.GetGlInfo();

  Gtk::MessageDialog aMsg(anAbout.str().c_str(), false, Gtk::MESSAGE_INFO);
  aMsg.set_title("About Sample");
  aMsg.set_secondary_text(aGlInfo.ToCString());
  aMsg.run();
}

// ================================================================
// Function : onValueChanged
// ================================================================
void OcctGtkWindowSample::onValueChanged(const Glib::RefPtr<Gtk::Adjustment>& theAdj)
{
  float aVal = theAdj->get_value() / 360.0f;
  const Quantity_Color aColor(aVal, aVal, aVal, Quantity_TOC_sRGB);
  myViewer.View()->SetBgGradientColors(aColor, Quantity_NOC_BLACK, Aspect_GradientFillMethod_Elliptical);
  myViewer.View()->Invalidate();
  myViewer.queue_draw();
}
