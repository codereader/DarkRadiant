#ifndef _SCREEN_UPDATE_BLOCKER_H_
#define _SCREEN_UPDATE_BLOCKER_H_

#include "gtkutil/window/BlockingTransientWindow.h"
#include <sigc++/connection.h>

namespace ui {

class ScreenUpdateBlocker :
	public gtkutil::TransientWindow
{
	bool _grabbedFocus;

	sigc::connection _focusHandler;
	sigc::connection _realizeHandler;

public:
	// Pass the window title and the text message to the constructor
	ScreenUpdateBlocker(const std::string& title, const std::string& message);

	~ScreenUpdateBlocker();

private:
	// Called whenever the main window is changing its "active" state property.
	void onMainWindowFocus();

	void onRealize();
};

} // namespace ui

#endif /* _SCREEN_UPDATE_BLOCKER_H_ */
