#include "ScreenUpdateBlocker.h"

#include "iradiant.h"
#include "map/AutoSaver.h"
#include "imainframe.h"

#include <wx/app.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/panel.h>

namespace ui {

ScreenUpdateBlocker::ScreenUpdateBlocker(const std::string& title, const std::string& message, bool forceDisplay) :
	TransientWindow(title, GlobalMainFrame().getWxTopLevelWindow()),
	_gauge(NULL)
{
	SetWindowStyleFlag(GetWindowStyleFlag() & ~(wxRESIZE_BORDER|wxCLOSE_BOX|wxMINIMIZE_BOX));

	wxPanel* panel = new wxPanel(this, wxID_ANY);

	panel->SetSizer(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	panel->GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 24);

	wxStaticText* label = new wxStaticText(panel, wxID_ANY, message);
	vbox->Add(label, 0, wxALIGN_CENTER | wxBOTTOM, 12);

	_gauge = new wxGauge(panel, wxID_ANY, 100);

	vbox->Add(_gauge, 1, wxEXPAND | wxALL);

	panel->Layout();
	panel->Fit();
	Layout();
	Fit();
	CenterOnParent();

	// Stop the autosaver
	map::AutoSaver().stopTimer();

	// Set the "screen updates disabled" flag
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
	wxTheApp->Yield();

	// Register for the "is-active" changed event, to display this dialog
	// as soon as Radiant is getting the focus again
	GlobalMainFrame().getWxTopLevelWindow()->Connect(
		wxEVT_SET_FOCUS, wxFocusEventHandler(ScreenUpdateBlocker::onMainWindowFocus), NULL, this);

	Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(ScreenUpdateBlocker::onCloseEvent), NULL, this);
}

ScreenUpdateBlocker::~ScreenUpdateBlocker()
{
	GlobalMainFrame().getWxTopLevelWindow()->Disconnect(
		wxEVT_SET_FOCUS, wxFocusEventHandler(ScreenUpdateBlocker::onMainWindowFocus), NULL, this);

	// Process pending events to flush keystroke buffer etc.
	wxTheApp->Yield();

	// Remove the event blocker, if appropriate
	_disabler.reset();

	// Re-enable screen updates
	GlobalMainFrame().enableScreenUpdates();

	// Start the autosaver again
	map::AutoSaver().startTimer();
}

void ScreenUpdateBlocker::pulse()
{
	_gauge->Pulse();
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
