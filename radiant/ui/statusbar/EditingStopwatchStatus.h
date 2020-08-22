#pragma once

#include <sigc++/connection.h>
#include "messages/ApplicationIsActiveRequest.h"

namespace ui
{

// Class handling the editing stop watch UI part (status bar and app-is-active checks)
class EditingStopwatchStatus
{
private:
	sigc::connection _conn;
	std::size_t _msgSubscription;

public:
	EditingStopwatchStatus();

	~EditingStopwatchStatus();

private:
	void handleRequest(radiant::ApplicationIsActiveRequest& msg);

	void onTimerChanged();
};

}
