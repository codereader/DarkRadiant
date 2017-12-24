#include "Splash.h"

#include <wx/panel.h>
#include <wx/dcbuffer.h>
#include <wx/splash.h>
#include <wx/sizer.h>
#include <wx/app.h>
#include <sigc++/retype_return.h>
#include <sigc++/functors/mem_fun.h>

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
private:
    wxBitmap _image;
	wxString _text;
 
public:
    wxImagePanel(wxFrame* parent, const wxString& file, wxBitmapType format);
 
    void paintEvent(wxPaintEvent & evt);

	void setText(const wxString& text);
 
    void render(wxDC& dc);
};

void wxImagePanel::setText(const wxString& text)
{
	_text = text;
}
 
wxImagePanel::wxImagePanel(wxFrame* parent, const wxString& file, wxBitmapType format) :
	wxPanel(parent)
{
    // load the file... ideally add a check to see if loading was successful
    _image.LoadFile(file, format);
	SetMinClientSize(wxSize(_image.GetWidth(), _image.GetHeight()));
	SetBackgroundStyle(wxBG_STYLE_PAINT);

    Bind(wxEVT_PAINT, [this](wxPaintEvent& ev) { paintEvent(ev); });
}

void wxImagePanel::paintEvent(wxPaintEvent & evt)
{
    // depending on your system you may need to look at double-buffered dcs
    wxAutoBufferedPaintDC dc(this);
    render(dc);
}
 
void wxImagePanel::render(wxDC&  dc)
{
    dc.DrawBitmap(_image, 0, 0, false);

	dc.SetTextForeground(wxColour(240, 240, 240));
	dc.DrawText(_text, wxPoint(15, _image.GetHeight() - wxNORMAL_FONT->GetPixelSize().GetHeight() - 15));
}

Splash::Splash() :
	wxFrame(nullptr, wxID_ANY, wxT("DarkRadiant"), wxDefaultPosition, wxDefaultSize, wxCENTRE),
	_progressBar(nullptr)
{
    const ApplicationContext& ctx = module::ModuleRegistry::Instance().getApplicationContext();
	std::string fullFileName(ctx.getBitmapsPath() + SPLASH_FILENAME);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);

	_imagePanel = new wxImagePanel(this, fullFileName, wxBITMAP_TYPE_ANY);
    sizer->Add(_imagePanel, 1, wxEXPAND);

	_progressBar = new wxGauge(this, wxID_ANY, 100);
	sizer->Add(_progressBar, 0, wxEXPAND);

	SetSizer(sizer);

	Fit();
	Centre();
	Show();

    // Subscribe to the post-module init event to destroy ourselves
    module::ModuleRegistry::Instance().signal_allModulesInitialised().connect(
        sigc::hide_return(sigc::mem_fun(this, &Splash::Destroy)));
}

void Splash::queueDraw()
{
	// Trigger a (re)draw, just to make sure that it gets displayed
	Refresh(false);
	Update();

	wxTheApp->Yield(true);
}

void Splash::setText(const std::string& text)
{
    _imagePanel->setText(text);
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

Splash& Splash::Instance()
{
	static Splash* instance = new Splash;
	return *instance;
}

void Splash::OnAppStartup()
{
	// Connect the module progress callback
	module::ModuleRegistry::Instance().signal_moduleInitialisationProgress().connect(
		sigc::mem_fun(Instance(), &Splash::setProgressAndText));
}

} // namespace ui
