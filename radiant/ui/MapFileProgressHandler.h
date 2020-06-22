#pragma once

#include "imap.h"
#include "iradiant.h"
#include <sigc++/functors/mem_fun.h>

#include "messages/MapFileOperation.h"
#include "wxutil/ModalProgressDialog.h"
#include "wxutil/dialog/MessageBox.h"
#include "ui/mainframe/ScreenUpdateBlocker.h"
#include "registry/registry.h"

namespace ui
{

class MapFileProgressHandler
{
private:
	std::size_t _msgSubscription;

	std::unique_ptr<ScreenUpdateBlocker> _blocker;

public:
	MapFileProgressHandler()
	{
		_msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
			radiant::TypeListener<map::FileOperation>(
				sigc::mem_fun(this, &MapFileProgressHandler::handleFileOperation)));
	}

	~MapFileProgressHandler()
	{
		GlobalRadiantCore().getMessageBus().removeListener(_msgSubscription);
	}

private:
	void handleFileOperation(map::FileOperation& msg)
	{
		try
		{
			switch (msg.getMessageType())
			{
			case map::FileOperation::Started:
				if (GlobalMainFrame().isActiveApp() && !registry::getValue<bool>(RKEY_MAP_SUPPRESS_LOAD_STATUS_DIALOG))
				{
					_blocker.reset(new ScreenUpdateBlocker(
						msg.getOperationType() == map::FileOperation::Type::Export ? _("Writing map") : _("Loading map"),
						_("Processing..."), true));
				}
				break;

			case map::FileOperation::Progress:
				if (!_blocker) break;

				if (msg.canCalculateProgress())
				{
					_blocker->setMessageAndProgress(msg.getText(), msg.getProgressFraction());
				}
				else
				{
					_blocker->setMessage(msg.getText());
					_blocker->pulse();
				}
				break;

			case map::FileOperation::Finished:
				_blocker.reset();
				break;
			};
		}
		catch (const wxutil::ModalProgressDialog::OperationAbortedException&)
		{
			_blocker.reset();
			msg.cancelOperation();
		}
	}
};

}
