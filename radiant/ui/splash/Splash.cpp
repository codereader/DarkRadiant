#include "Splash.h"

#include <wx/panel.h>
#include <wx/dcbuffer.h>
#include <wx/splash.h>
#include <wx/sizer.h>
#include <wx/app.h>

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
    void paintNow();

	void setText(const wxString& text);
 
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
    _image.LoadFile(file, format);
	SetMinClientSize(wxSize(_image.GetWidth(), _image.GetHeight()));
	SetBackgroundStyle(wxBG_STYLE_PAINT);
}

void wxImagePanel::setText(const wxString& text)
{
	_text = text;
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
    dc.DrawBitmap(_image, 0, 0, false);

	dc.SetTextForeground(wxColour(240, 240, 240));
	dc.DrawText(_text, wxPoint(15, _image.GetHeight() - wxNORMAL_FONT->GetPixelSize().GetHeight() - 15));
}

Splash::Splash() :
	wxFrame(NULL, wxID_ANY, wxT("DarkRadiant"), wxDefaultPosition, wxDefaultSize, wxCENTRE),
	_progressBar(NULL)
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
}

bool Splash::isVisible()
{
	return InstancePtr() && InstancePtr()->IsVisible();
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

void Splash::queueDraw()
{
	// Trigger a (re)draw, just to make sure that it gets displayed
	Refresh(false);
	Update();

	wxTheApp->Yield();
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
