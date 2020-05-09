#pragma once

#include <functional>
#include "iradiant.h"
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

public:
	LongRunningOperationHandler() :
		_level(0)
	{
		GlobalRadiantCore().getMessageBus().addListener(
			radiant::TypeListener<radiant::LongRunningOperationMessage>(
				std::bind(&LongRunningOperationHandler::onMessage, this, std::placeholders::_1)
			)
		);
	}

private:
	void onMessage(radiant::LongRunningOperationMessage& message)
	{
		if (message.getType() == radiant::OperationEvent::Started)
		{
			std::string title = message.getTitle();

			if (title.empty())
			{
				title = _("Operation in progress");
			}

			if (++_level == 1)
			{
				_blocker = GlobalMainFrame().getScopedScreenUpdateBlocker(title, std::string());
			}
		}
		else if (message.getType() == radiant::OperationEvent::Finished)
		{
			assert(_level > 0);

			if (_level > 0 && --_level == 0)
			{
				_blocker.reset();
			}
		}
	}
};

}
