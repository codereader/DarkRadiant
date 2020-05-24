#pragma once

#include "iradiant.h"
#include <sigc++/functors/mem_fun.h>

#include "messages/MapExportOperation.h"
#include "wxutil/ModalProgressDialog.h"
#include "wxutil/dialog/MessageBox.h"

namespace ui
{

class MapExportProgressHandler
{
private:
	std::size_t _msgSubscription;

	wxutil::ModalProgressDialogPtr _dialog;

public:
	MapExportProgressHandler()
	{
		_msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
			radiant::TypeListener<map::ExportOperation>(
				sigc::mem_fun(this, &MapExportProgressHandler::handleMapExportMessage)));
	}

	~MapExportProgressHandler()
	{
		GlobalRadiantCore().getMessageBus().removeListener(_msgSubscription);
	}

private:
	void handleMapExportMessage(map::ExportOperation& msg)
	{
		try
		{
			switch (msg.getType())
			{
			case map::ExportOperation::Started:
				if (GlobalMainFrame().isActiveApp())
				{
					_dialog.reset(new wxutil::ModalProgressDialog(_("Writing map")));
				}
				break;

			case map::ExportOperation::Progress:
				if (!_dialog) break;

				if (msg.getNumTotalNodes() > 0)
				{
					_dialog->setTextAndFraction(msg.getText(), msg.getProgressFraction());
				}
				else
				{
					_dialog->setText(msg.getText());
					_dialog->Pulse();
				}
				break;

			case map::ExportOperation::Finished:
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
