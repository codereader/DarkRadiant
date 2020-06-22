#include "ScreenUpdateBlocker.h"

#include "iradiant.h"
#include "imainframe.h"

#include <wx/app.h>

namespace ui {

ScreenUpdateBlocker::ScreenUpdateBlocker(const std::string& title, const std::string& message, bool forceDisplay) :
	wxutil::ModalProgressDialog(title, GlobalMainFrame().getWxTopLevelWindow()),
	_pulseAllowed(true)
{
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
	GlobalMainFrame().getWxTopLevelWindow()->Bind(wxEVT_SET_FOCUS, &ScreenUpdateBlocker::onMainWindowFocus, this);

	Bind(wxEVT_CLOSE_WINDOW, &ScreenUpdateBlocker::onCloseEvent, this);
    Bind(wxEVT_IDLE, [&](wxIdleEvent&) { pulse(); });
}

ScreenUpdateBlocker::~ScreenUpdateBlocker()
{
	GlobalMainFrame().getWxTopLevelWindow()->Unbind(wxEVT_SET_FOCUS, &ScreenUpdateBlocker::onMainWindowFocus, this);

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
		ModalProgressDialog::Pulse();
	}
}

void ScreenUpdateBlocker::doSetProgress(float progress)
{
	ModalProgressDialog::setFraction(progress);

	_pulseAllowed = false;
}

void ScreenUpdateBlocker::setProgress(float progress)
{
	doSetProgress(progress);
}

void ScreenUpdateBlocker::doSetMessage(const std::string& message)
{
	ModalProgressDialog::setText(message);
}

void ScreenUpdateBlocker::setMessage(const std::string& message)
{
	doSetMessage(message);

	Refresh();
	Update();
}

void ScreenUpdateBlocker::setMessageAndProgress(const std::string& message, float progress)
{
	ModalProgressDialog::setTextAndFraction(message, progress);
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
