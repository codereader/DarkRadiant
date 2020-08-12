#include "LongRunningOperationHandler.h"

#include <functional>
#include "iradiant.h"
#include "imainframe.h"
#include "i18n.h"
#include "UserInterfaceModule.h"

namespace ui
{

LongRunningOperationHandler::LongRunningOperationHandler() :
	_level(0)
{
	GlobalRadiantCore().getMessageBus().addListener(radiant::IMessage::Type::LongRunningOperation,
		radiant::TypeListener<radiant::LongRunningOperationMessage>(
			std::bind(&LongRunningOperationHandler::onMessage, this, std::placeholders::_1)
			)
	);
}

void LongRunningOperationHandler::onMessage(radiant::LongRunningOperationMessage& message)
{
	std::lock_guard<std::mutex> lock(_lock);

	if (message.getType() == radiant::OperationEvent::Started)
	{
		std::string description = message.getMessage();

		if (description.empty())
		{
			description = _("...crunching...");
		}

		if (++_level == 1)
		{
			GetUserInterfaceModule().dispatch([description, this]()
			{
				std::lock_guard<std::mutex> lock(_lock);

				// Level might have been decreased in the meantime, check it
				if (_level > 0)
				{
					_blocker = GlobalMainFrame().getScopedScreenUpdateBlocker(_("Processing..."), description);
				}
			});
		}
	}
	else if (message.getType() == radiant::OperationEvent::Finished)
	{
		assert(_level > 0);

		if (_level > 0 && --_level == 0)
		{
			GetUserInterfaceModule().dispatch([this]()
			{
				std::lock_guard<std::mutex> lock(_lock);

				_blocker.reset();
			});
		}
	}
}

}
