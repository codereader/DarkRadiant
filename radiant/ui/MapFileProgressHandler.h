#pragma once

#include "imap.h"
#include "iradiant.h"
#include <sigc++/functors/mem_fun.h>

#include "messages/MapFileOperation.h"
#include "wxutil/ModalProgressDialog.h"
#include "wxutil/dialog/MessageBox.h"
#include "registry/registry.h"

namespace ui
{

class MapFileProgressHandler
{
private:
	std::size_t _msgSubscription;

	wxutil::ModalProgressDialogPtr _dialog;

public:
	MapFileProgressHandler()
	{
		_msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
			radiant::TypeListener<map::FileOperation>(
				sigc::mem_fun(this, &MapFileProgressHandler::handleMapExportMessage)));
	}

	~MapFileProgressHandler()
	{
		GlobalRadiantCore().getMessageBus().removeListener(_msgSubscription);
	}

private:
	void handleMapExportMessage(map::FileOperation& msg)
	{
		try
		{
			switch (msg.getMessageType())
			{
			case map::FileOperation::Started:
				if (GlobalMainFrame().isActiveApp() && !registry::getValue<bool>(RKEY_MAP_SUPPRESS_LOAD_STATUS_DIALOG))
				{
					_dialog.reset(new wxutil::ModalProgressDialog(
						msg.getOperationType() == map::FileOperation::Type::Export ? _("Writing map") : _("Loading map")));
				}
				break;

			case map::FileOperation::Progress:
				if (!_dialog) break;

				if (msg.canCalculateProgress())
				{
					_dialog->setTextAndFraction(msg.getText(), msg.getProgressFraction());
				}
				else
				{
					_dialog->setText(msg.getText());
					_dialog->Pulse();
				}
				break;

			case map::FileOperation::Finished:
				_dialog.reset();
				break;
			};
		}
		catch (const wxutil::ModalProgressDialog::OperationAbortedException&)
		{
			_dialog.reset();
			msg.cancelOperation();
		}
	}
};

}
