#pragma once

#include "imainframe.h"
#include <memory>

#include "wxutil/window/TransientWindow.h"

class wxGauge;

namespace ui
{

class ScreenUpdateBlocker :
	public IScopedScreenUpdateBlocker,
	public wxutil::TransientWindow
{
private:
	std::unique_ptr<wxWindowDisabler> _disabler;

	wxGauge* _gauge;

public:
	// Pass the window title and the text message to the constructor
	ScreenUpdateBlocker(const std::string& title, const std::string& message, bool forceDisplay = false);

	void pulse();

	~ScreenUpdateBlocker();

private:
	// Called whenever the main window is changing its "active" state property.
	void onMainWindowFocus(wxFocusEvent& ev);
	void onCloseEvent(wxCloseEvent& ev);
};

} // namespace ui
