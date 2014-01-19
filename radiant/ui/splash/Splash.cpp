#include "Splash.h"

#include "gtkutil/LeftAlignedLabel.h"

#include <gtkmm/progressbar.h>
#include <gtkmm/image.h>
#include <gtkmm/box.h>
#include <gtkmm/main.h>

#include <wx/wxprec.h>
#include <wx/dcbuffer.h>
#include <wx/splash.h>

#include "modulesystem/ModuleRegistry.h"

namespace ui
{

namespace
{
	const char* const SPLASH_FILENAME = "darksplash.png";
}

class wxImagePanel : 
	public wxPanel
{
    wxBitmap image;
 
public:
    wxImagePanel(wxFrame* parent, const wxString& file, wxBitmapType format);
 
    void paintEvent(wxPaintEvent & evt);
    void paintNow();
 
    void render(wxDC& dc);
 
    DECLARE_EVENT_TABLE()
};
 
BEGIN_EVENT_TABLE(wxImagePanel, wxPanel)
	// catch paint events
	EVT_PAINT(wxImagePanel::paintEvent)
END_EVENT_TABLE()
 
wxImagePanel::wxImagePanel(wxFrame* parent, const wxString& file, wxBitmapType format) :
	wxPanel(parent)
{
    // load the file... ideally add a check to see if loading was successful
    image.LoadFile(file, format);
	SetMinClientSize(wxSize(image.GetWidth(), image.GetHeight()));
	SetBackgroundStyle(wxBG_STYLE_PAINT);
}
 
void wxImagePanel::paintEvent(wxPaintEvent & evt)
{
    // depending on your system you may need to look at double-buffered dcs
    wxAutoBufferedPaintDC dc(this);
    render(dc);
}
 
void wxImagePanel::paintNow()
{
    // depending on your system you may need to look at double-buffered dcs
    wxClientDC dc(this);
    render(dc);
}
 
void wxImagePanel::render(wxDC&  dc)
{
    dc.DrawBitmap(image, 0, 0, false);
}

Splash::Splash() :
	wxFrame(NULL, wxID_ANY, wxT("DarkRadiant"), wxDefaultPosition, wxDefaultSize, wxSTAY_ON_TOP),
	_progressBar(NULL)
{
	/*set_decorated(false);
	set_resizable(false);
	set_modal(true);
	set_border_width(0);*/

	const ApplicationContext& ctx = module::getRegistry().getApplicationContext();
	std::string fullFileName(ctx.getBitmapsPath() + SPLASH_FILENAME);

	_sizer = new wxBoxSizer(wxVERTICAL);

	wxImagePanel* drawPane = new wxImagePanel(this, fullFileName, wxBITMAP_TYPE_ANY);
    _sizer->Add(drawPane, 1, wxEXPAND);

	_progressBar = new wxGauge(this, wxID_ANY, 100);
	_sizer->Add(_progressBar, 0, wxEXPAND);

	SetSizer(_sizer);

	Fit();
	Centre();
	Show();
}

bool Splash::isVisible()
{
	return InstancePtr() && InstancePtr()->IsVisible();
}

void Splash::setTopLevelWindow(const Glib::RefPtr<Gtk::Window>& window)
{
	if (!window) return;

	Gtk::Container* toplevel = window->get_toplevel();

	if (toplevel != NULL && toplevel->is_toplevel() &&
		dynamic_cast<Gtk::Window*>(toplevel) != NULL)
	{
		// wxTODO set_transient_for(*static_cast<Gtk::Window*>(toplevel));
	}
}

void Splash::setText(const std::string& text)
{
	//_progressBar->set_text(text);
	queueDraw();
}

void Splash::setProgress(float fraction)
{
	_progressBar->SetValue(static_cast<int>(fraction*100));
	queueDraw();
}

void Splash::setProgressAndText(const std::string& text, float fraction)
{
	setText(text);
	setProgress(fraction);
}

void Splash::show_all()
{
	queueDraw();
}

void Splash::queueDraw()
{
	// Trigger a (re)draw, just to make sure that it gets displayed
	Refresh(false);
	Update();

	while (wxTheApp->HasPendingEvents())
	{
		wxTheApp->Dispatch();
	}
}

void Splash::destroy()
{
	if (InstancePtr())
	{
		InstancePtr()->Destroy();
	}
}

SplashPtr& Splash::InstancePtr()
{
	static SplashPtr _instancePtr = NULL;
	return _instancePtr;
}

Splash& Splash::Instance()
{
	SplashPtr& instancePtr = InstancePtr();

	if (instancePtr == NULL)
	{
		instancePtr = new Splash;
	}

	return *instancePtr;
}

} // namespace ui
