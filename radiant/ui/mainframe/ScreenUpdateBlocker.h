#ifndef _SCREEN_UPDATE_BLOCKER_H_
#define _SCREEN_UPDATE_BLOCKER_H_

#include "gtkutil/window/BlockingTransientWindow.h"

namespace ui {

class ScreenUpdateBlocker :
	public gtkutil::TransientWindow
{
public:
	// Pass the window title and the text message to the constructor
	ScreenUpdateBlocker(const std::string& title, const std::string& message);

	~ScreenUpdateBlocker();
};

} // namespace ui

#endif /* _SCREEN_UPDATE_BLOCKER_H_ */
