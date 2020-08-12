#include "MapFileProgressHandler.h"

#include "imap.h"
#include "iradiant.h"
#include "i18n.h"
#include <sigc++/functors/mem_fun.h>

#include "wxutil/ModalProgressDialog.h"
#include "wxutil/dialog/MessageBox.h"
#include "registry/registry.h"
#include "UserInterfaceModule.h"

namespace ui
{

MapFileProgressHandler::MapFileProgressHandler() :
	_wasCancelled(false),
	_level(0)
{
	_msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
		radiant::IMessage::Type::MapFileOperation,
		radiant::TypeListener<map::FileOperation>(
			sigc::mem_fun(this, &MapFileProgressHandler::handleFileOperation)));
}

MapFileProgressHandler::~MapFileProgressHandler()
{
	GlobalRadiantCore().getMessageBus().removeListener(_msgSubscription);
}

void MapFileProgressHandler::dispatchWithLockAndCatch(const std::function<void()>& function)
{
	GetUserInterfaceModule().dispatch([function, this] ()
	{
		std::lock_guard<std::mutex> lock(_lock);

		try
		{
			// Any updates to the _blocker instance might throw an OperationAbortedException
			function();
		}
		catch (const wxutil::ModalProgressDialog::OperationAbortedException&)
		{
			// We're in the UI thread, so destroy the dialog right here
			_blocker.reset();

			// Remember that we got cancelled, next time a message is incoming,
			// we will signal this to the caller
			_wasCancelled = true;
		}
	});
}

void MapFileProgressHandler::handleFileOperation(map::FileOperation& msg)
{
	auto lock = std::make_unique< std::lock_guard<std::mutex> >(_lock);

	// A previous _blocker update might indicate a cancel operation, propagate this info
	if (_wasCancelled)
	{
		msg.cancelOperation();
		return;
	}
	
	switch (msg.getMessageType())
	{
	case map::FileOperation::Started:
	{
		auto title = msg.getOperationType() == map::FileOperation::Type::Export ?
			_("Writing map") : _("Loading map");

		if (++_level == 1)
		{
			dispatchWithLockAndCatch([title, this]()
			{
				// Level might have been decreased in the meantime, check it
				if (_level > 0 && GlobalMainFrame().isActiveApp() && 
					!registry::getValue<bool>(RKEY_MAP_SUPPRESS_LOAD_STATUS_DIALOG))
				{
					_blocker.reset(new ScreenUpdateBlocker(title, _("Processing..."), true));
				}
			});
		}
	}
	break;

	case map::FileOperation::Progress:
	{
		if (!_blocker || _level == 0) break;

		auto text = msg.getText();

		if (msg.canCalculateProgress())
		{
			auto fraction = msg.getProgressFraction();

			dispatchWithLockAndCatch([text, fraction, this]()
			{
				_blocker->setMessageAndProgress(text, fraction);
			});
		}
		else
		{
			dispatchWithLockAndCatch([text, this]()
			{
				_blocker->setMessage(text);
				_blocker->pulse();
			});
		}
	}
	break;

	case map::FileOperation::Finished:
	{
		assert(_level > 0);

		if (_level > 0 && --_level == 0)
		{
			dispatchWithLockAndCatch([this]()
			{
				_blocker.reset();
			});
		}
		break;
	}
	};

	// Release the lock, and give the UI a chance to process
	lock.reset();

	wxTheApp->Yield();
}

}
