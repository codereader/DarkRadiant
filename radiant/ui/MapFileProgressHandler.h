#pragma once

#include "iradiant.h"
#include <sigc++/functors/mem_fun.h>

#include "messages/MapFileOperation.h"
#include "wxutil/ModalProgressDialog.h"
#include "wxutil/dialog/MessageBox.h"

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
			switch (msg.getType())
			{
			case map::FileOperation::Started:
				if (GlobalMainFrame().isActiveApp())
				{
					_dialog.reset(new wxutil::ModalProgressDialog(_("Writing map")));
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
			msg.cancelOperation();
		}
	}
};

}
