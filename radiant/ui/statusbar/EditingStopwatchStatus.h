#pragma once

#include "i18n.h"
#include "iuimanager.h"
#include "ieditstopwatch.h"
#include <sigc++/connection.h>
#include <sigc++/functors/mem_fun.h>
#include <fmt/format.h>

namespace ui
{

class EditingStopwatchStatus
{
private:
	sigc::connection _conn;

	const char* const STATUS_BAR_ELEMENT = "EditTime";

public:
	EditingStopwatchStatus()
	{
		_conn = GlobalMapEditStopwatch().sig_TimerChanged().connect(
			sigc::mem_fun(this, &EditingStopwatchStatus::onTimerChanged)
		);

		// Add the status bar element
		GlobalUIManager().getStatusBarManager().addTextElement(STATUS_BAR_ELEMENT, "stopwatch.png",
			IStatusBarManager::POS_MAP_EDIT_TIME, _("Time spent on this map"));

		GlobalUIManager().getStatusBarManager().setText(STATUS_BAR_ELEMENT, "00:00:00");
	}

	~EditingStopwatchStatus()
	{
		_conn.disconnect();
	}

private:
	void onTimerChanged()
	{
		auto secondsEdited = GlobalMapEditStopwatch().getTotalSecondsEdited();

		// Format the time and pass it to the status bar
		unsigned long hours = secondsEdited / 3600;
		unsigned long minutes = (secondsEdited % 3600) / 60;
		unsigned long seconds = secondsEdited % 60;

		GlobalUIManager().getStatusBarManager().setText(STATUS_BAR_ELEMENT,
			fmt::format("{0:02d}:{1:02d}:{2:02d}", hours, minutes, seconds));
	}
};

}
