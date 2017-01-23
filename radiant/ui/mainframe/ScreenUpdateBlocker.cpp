#include "ScreenUpdateBlocker.h"

#include "iradiant.h"
#include "imainframe.h"

#include <wx/app.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/panel.h>
#include <wx/gauge.h>

namespace ui {

ScreenUpdateBlocker::ScreenUpdateBlocker(const std::string& title, const std::string& message, bool forceDisplay) :
	TransientWindow(title, GlobalMainFrame().getWxTopLevelWindow()),
	_gauge(NULL),
	_pulseAllowed(true)
{
	SetWindowStyleFlag(GetWindowStyleFlag() & ~(wxRESIZE_BORDER|wxCLOSE_BOX|wxMINIMIZE_BOX));

	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxPanel* panel = new wxPanel(this, wxID_ANY);
	GetSizer()->Add(panel, 1, wxEXPAND);

	panel->SetSizer(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	panel->GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 24);

	_message = new wxStaticText(panel, wxID_ANY, message);
	vbox->Add(_message, 0, wxALIGN_CENTER | wxBOTTOM, 12);

	_gauge = new wxGauge(panel, wxID_ANY, 100);

	vbox->Add(_gauge, 1, wxEXPAND | wxALL);

	panel->Layout();
	panel->Fit();
	Layout();
	Fit();
	CenterOnParent();

	// Set the "screen updates disabled" flag (also disables autosaver)
	GlobalMainFrame().disableScreenUpdates();

	// Connect the realize signal to remove the window decorations
	if (GlobalMainFrame().isActiveApp() || forceDisplay)
	{
		// Show this window immediately
		Show();

		// Eat all window events
		_disabler.reset(new wxWindowDisabler(this));
	}

	// Process pending events to fully show the dialog
	wxTheApp->Yield(true);

	// Register for the "is-active" changed event, to display this dialog
	// as soon as Radiant is getting the focus again
	GlobalMainFrame().getWxTopLevelWindow()->Connect(
		wxEVT_SET_FOCUS, wxFocusEventHandler(ScreenUpdateBlocker::onMainWindowFocus), NULL, this);

	Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(ScreenUpdateBlocker::onCloseEvent), NULL, this);

    Bind(wxEVT_IDLE, [&](wxIdleEvent&)
    {
        pulse();
    });
}

ScreenUpdateBlocker::~ScreenUpdateBlocker()
{
	GlobalMainFrame().getWxTopLevelWindow()->Disconnect(
		wxEVT_SET_FOCUS, wxFocusEventHandler(ScreenUpdateBlocker::onMainWindowFocus), NULL, this);

	// Process pending events to flush keystroke buffer etc.
	wxTheApp->Yield(true);

	// Remove the event blocker, if appropriate
	_disabler.reset();

	// Re-enable screen updates
	GlobalMainFrame().enableScreenUpdates();

    // Re-draw the scene to clear any artefacts in the buffer
    GlobalMainFrame().updateAllWindows();
}

void ScreenUpdateBlocker::pulse()
{
	if (_pulseAllowed)
	{
		_gauge->Pulse();
	}
}

void ScreenUpdateBlocker::setProgress(float progress)
{
	if (progress < 0.0f) progress = 0.0f;
	if (progress > 1.0f) progress = 1.0f;

	_gauge->SetValue(static_cast<int>(progress * 100));

	_pulseAllowed = false;

	Refresh();
	Update();
}

void ScreenUpdateBlocker::setMessage(const std::string& message)
{
	std::size_t oldLength = _message->GetLabel().Length();

	_message->SetLabel(message);

	if (message.length() > oldLength)
	{
		Layout();
		Fit();
		CenterOnParent();
	}

	Refresh();
	Update();
}

void ScreenUpdateBlocker::onMainWindowFocus(wxFocusEvent& ev)
{
	// The Radiant main window has changed its active state, let's see if it became active
	// and if yes, show this blocker dialog.
	if (GlobalMainFrame().getWxTopLevelWindow()->IsActive() && !IsShownOnScreen())
	{
		Show();
	}
}

void ScreenUpdateBlocker::onCloseEvent(wxCloseEvent& ev)
{
	ev.Veto();
}

} // namespace ui
