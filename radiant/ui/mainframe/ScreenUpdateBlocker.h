#pragma once

#include "imainframe.h"
#include <memory>

#include "wxutil/window/TransientWindow.h"

namespace ui
{

class ScreenUpdateBlocker :
	public IScopedScreenUpdateBlocker,
	public wxutil::TransientWindow
{
private:
	std::unique_ptr<wxWindowDisabler> _disabler;

public:
	// Pass the window title and the text message to the constructor
	ScreenUpdateBlocker(const std::string& title, const std::string& message, bool forceDisplay = false);

	~ScreenUpdateBlocker();

private:
	// Called whenever the main window is changing its "active" state property.
	void onMainWindowFocus(wxFocusEvent& ev);
	void onCloseEvent(wxCloseEvent& ev);
};

} // namespace ui
