#pragma once

#include <mutex>
#include "imainframe.h"
#include "messages/LongRunningOperationMessage.h"

namespace ui
{

/**
 * Listener class responding to LongRunningOperationMessages
 * sent from the core routines. It will show a blocking dialog
 * when any operation is started, and will hide that window
 * once the last running operation is finished.
 */
class LongRunningOperationHandler
{
private:
	std::size_t _level;

	IScopedScreenUpdateBlockerPtr _blocker;

	std::mutex _lock;

public:
	LongRunningOperationHandler();

private:
	void onMessage(radiant::LongRunningOperationMessage& message);
};

}
