#pragma once

#include "imainframe.h"

#include "gtkutil/window/BlockingTransientWindow.h"
#include <sigc++/connection.h>

namespace ui {

class ScreenUpdateBlocker :
	public IScopedScreenUpdateBlocker,
	public gtkutil::TransientWindow
{
	bool _grabbedFocus;

	sigc::connection _focusHandler;
	sigc::connection _realizeHandler;

public:
	// Pass the window title and the text message to the constructor
	ScreenUpdateBlocker(const std::string& title, const std::string& message, bool forceDisplay = false);

	~ScreenUpdateBlocker();

private:
	// Called whenever the main window is changing its "active" state property.
	void onMainWindowFocus();

	void onRealize();
};

} // namespace ui
