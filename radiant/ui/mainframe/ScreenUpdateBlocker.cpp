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
	TransientWindow(title, GlobalMainFrame().getWxTopLevelWindow())
{
	SetWindowStyleFlag(GetWindowStyleFlag() & ~(wxRESIZE_BORDER|wxCLOSE_BOX|wxMINIMIZE_BOX));

	wxPanel* panel = new wxPanel(this, wxID_ANY);
	panel->SetSizer(new wxBoxSizer(wxVERTICAL));

	wxStaticText* label = new wxStaticText(panel, wxID_ANY, message);

	panel->GetSizer()->Add(label, 1, wxEXPAND | wxALL, 12);

	SetMinSize(wxSize(200, 40));
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
	while (wxTheApp->HasPendingEvents())
	{
		wxTheApp->Dispatch();
	}

	// Register for the "is-active" changed event, to display this dialog
	// as soon as Radiant is getting the focus again
	GlobalMainFrame().getWxTopLevelWindow()->Connect(
		wxEVT_SET_FOCUS, wxFocusEventHandler(ScreenUpdateBlocker::onMainWindowFocus), NULL, this);
}

ScreenUpdateBlocker::~ScreenUpdateBlocker()
{
	GlobalMainFrame().getWxTopLevelWindow()->Disconnect(
		wxEVT_SET_FOCUS, wxFocusEventHandler(ScreenUpdateBlocker::onMainWindowFocus), NULL, this);

	// Process pending events to flush keystroke buffer etc.
	while (wxTheApp->HasPendingEvents())
	{
		wxTheApp->Dispatch();
	} 

	// Remove the event blocker, if appropriate
	_disabler.reset();

	// Re-enable screen updates
	GlobalMainFrame().enableScreenUpdates();

	// Start the autosaver again
	map::AutoSaver().startTimer();
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

} // namespace ui
