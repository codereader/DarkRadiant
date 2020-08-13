#include "EditingStopwatchStatus.h"

#include "ieditstopwatch.h"
#include "i18n.h"
#include "iuimanager.h"
#include "imainframe.h"
#include <sigc++/functors/mem_fun.h>
#include <fmt/format.h>
#include "ui/UserInterfaceModule.h"

namespace ui
{

namespace
{
	const char* const STATUS_BAR_ELEMENT = "EditTime";
}

EditingStopwatchStatus::EditingStopwatchStatus()
{
	_conn = GlobalMapEditStopwatch().sig_TimerChanged().connect(
		sigc::mem_fun(this, &EditingStopwatchStatus::onTimerChanged)
	);

	_msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
		radiant::IMessage::Type::ApplicationIsActiveQuery,
		radiant::TypeListener<radiant::ApplicationIsActiveRequest>(
			sigc::mem_fun(this, &EditingStopwatchStatus::handleRequest)));

	// Add the status bar element
	GlobalUIManager().getStatusBarManager().addTextElement(STATUS_BAR_ELEMENT, "stopwatch.png",
		IStatusBarManager::POS_MAP_EDIT_TIME, _("Time spent on this map"));

	GlobalUIManager().getStatusBarManager().setText(STATUS_BAR_ELEMENT, "00:00:00");
}

EditingStopwatchStatus::~EditingStopwatchStatus()
{
	GlobalRadiantCore().getMessageBus().removeListener(_msgSubscription);
	_conn.disconnect();
}

void EditingStopwatchStatus::handleRequest(radiant::ApplicationIsActiveRequest& msg)
{
	msg.setApplicationIsActive(
		GlobalMainFrame().isActiveApp() && GlobalMainFrame().screenUpdatesEnabled()
	);
}

void EditingStopwatchStatus::onTimerChanged()
{
	// This function might be called from any thread, so dispatch this
	GetUserInterfaceModule().dispatch([]()
	{
		auto secondsEdited = GlobalMapEditStopwatch().getTotalSecondsEdited();

		// Format the time and pass it to the status bar
		unsigned long hours = secondsEdited / 3600;
		unsigned long minutes = (secondsEdited % 3600) / 60;
		unsigned long seconds = secondsEdited % 60;

		GlobalUIManager().getStatusBarManager().setText(STATUS_BAR_ELEMENT,
			fmt::format("{0:02d}:{1:02d}:{2:02d}", hours, minutes, seconds));
	});
}

}
