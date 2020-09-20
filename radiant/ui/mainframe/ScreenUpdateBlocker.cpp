#include "ScreenUpdateBlocker.h"

#include "iradiant.h"
#include "imainframe.h"

#include <wx/app.h>
#include <wx/frame.h>

namespace ui {

ScreenUpdateBlocker::ScreenUpdateBlocker(const std::string& title, const std::string& message, bool forceDisplay) :
	_dialog(nullptr),
	_pulseAllowed(true),
	_title(title)
{
	// Set the "screen updates disabled" flag (also disables autosaver)
	GlobalMainFrame().disableScreenUpdates();

	// Connect the realize signal to remove the window decorations
	if (GlobalMainFrame().isActiveApp() || forceDisplay)
	{
		showModalProgressDialog();
	}

	// Register for the "is-active" changed event, to display this dialog
	// as soon as Radiant is getting the focus again
	GlobalMainFrame().getWxTopLevelWindow()->Bind(wxEVT_SET_FOCUS, &ScreenUpdateBlocker::onMainWindowFocus, this);
}

void ScreenUpdateBlocker::showModalProgressDialog()
{
	_dialog = new wxutil::ModalProgressDialog(_title, GlobalMainFrame().getWxTopLevelWindow());

	_dialog->Bind(wxEVT_CLOSE_WINDOW, &ScreenUpdateBlocker::onCloseEvent, this);

	// Show this window immediately
	_dialog->Show();

	// Eat all window events
	_disabler.reset(new wxWindowDisabler(_dialog));

	// Process pending events to fully show the dialog
	wxTheApp->Yield(true);
}

ScreenUpdateBlocker::~ScreenUpdateBlocker()
{
	GlobalMainFrame().getWxTopLevelWindow()->Unbind(wxEVT_SET_FOCUS, &ScreenUpdateBlocker::onMainWindowFocus, this);

	// Process pending events to flush keystroke buffer etc.
	wxTheApp->Yield(true);

	if (_dialog != nullptr)
	{
		_dialog->Destroy();
	}

	// Remove the event blocker, if appropriate
	_disabler.reset();

	// Re-enable screen updates
	GlobalMainFrame().enableScreenUpdates();

    // Re-draw the scene to clear any artefacts in the buffer
    GlobalMainFrame().updateAllWindows();

	// Run one idle event loop, to allow wxWidgets to clean up the
	// modal progress dialog window
	wxTheApp->ProcessIdle();
}

void ScreenUpdateBlocker::pulse()
{
	if (_pulseAllowed && _dialog != nullptr)
	{
		_dialog->Pulse();
	}
}

void ScreenUpdateBlocker::doSetProgress(float progress)
{
	if (_dialog != nullptr)
	{
		_dialog->setFraction(progress);
	}

	_pulseAllowed = false;
}

void ScreenUpdateBlocker::setProgress(float progress)
{
	doSetProgress(progress);
}

void ScreenUpdateBlocker::doSetMessage(const std::string& message)
{
	if (_dialog != nullptr)
	{
		_dialog->setText(message);
	}
}

void ScreenUpdateBlocker::setMessage(const std::string& message)
{
	doSetMessage(message);

	if (_dialog != nullptr)
	{ 
		_dialog->Refresh();
		_dialog->Update();
	}
}

void ScreenUpdateBlocker::setMessageAndProgress(const std::string& message, float progress)
{
	if (_dialog != nullptr)
	{
		_dialog->setTextAndFraction(message, progress);
	}
}

void ScreenUpdateBlocker::onMainWindowFocus(wxFocusEvent& ev)
{
	// The Radiant main window has changed its active state, let's see if it became active
	// and if yes, show the blocker dialog.
	if (GlobalMainFrame().getWxTopLevelWindow()->IsActive() && _dialog == nullptr)
	{
		showModalProgressDialog();
	}
}

void ScreenUpdateBlocker::onCloseEvent(wxCloseEvent& ev)
{
	ev.Veto();
}

} // namespace ui
