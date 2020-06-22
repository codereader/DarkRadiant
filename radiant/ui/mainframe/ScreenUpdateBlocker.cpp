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
	wxutil::ModalProgressDialog(title, GlobalMainFrame().getWxTopLevelWindow()),
#if 0
	_gauge(nullptr),
#endif
	_pulseAllowed(true)
{
#if 0
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
#endif
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
#if 0
		_gauge->Pulse();
#endif
	}
}

void ScreenUpdateBlocker::doSetProgress(float progress)
{
	ModalProgressDialog::setFraction(progress);

#if 0
	if (progress < 0.0f) progress = 0.0f;
	if (progress > 1.0f) progress = 1.0f;

	_gauge->SetValue(static_cast<int>(progress * 100));
#endif
	_pulseAllowed = false;
}

void ScreenUpdateBlocker::setProgress(float progress)
{
	doSetProgress(progress);
#if 0
	Refresh();
	Update();
#endif
}

void ScreenUpdateBlocker::doSetMessage(const std::string& message)
{
	ModalProgressDialog::setText(message);
#if 0
	std::size_t oldLength = _message->GetLabel().Length();

	_message->SetLabel(message);

	if (message.length() > oldLength)
	{
		Layout();
		Fit();
		CenterOnParent();
	}
#endif
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
#if 0
	doSetProgress(progress);
	doSetMessage(message);

	Refresh();
	Update();
#endif
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
