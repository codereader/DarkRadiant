#pragma once

#include "imainframe.h"
#include <memory>

#include "wxutil/window/TransientWindow.h"

class wxGauge;
class wxStaticText;

namespace ui
{

class ScreenUpdateBlocker :
	public IScopedScreenUpdateBlocker,
	public wxutil::TransientWindow
{
private:
	std::unique_ptr<wxWindowDisabler> _disabler;

	wxStaticText* _message;
	wxGauge* _gauge;

	// Once we received a call to setProgress() further calls to pulse() are forbidden
	bool _pulseAllowed;

public:
	// Pass the window title and the text message to the constructor
	ScreenUpdateBlocker(const std::string& title, const std::string& message, bool forceDisplay = false);

	void pulse();
	void setProgress(float progress);
	void setMessage(const std::string& message);

	~ScreenUpdateBlocker();

private:
	// Called whenever the main window is changing its "active" state property.
	void onMainWindowFocus(wxFocusEvent& ev);
	void onCloseEvent(wxCloseEvent& ev);
};

} // namespace ui
