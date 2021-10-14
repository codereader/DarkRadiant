#pragma once

#include "ui/idialogmanager.h"
#include "iradiant.h"
#include "i18n.h"
#include "messages/FileOverwriteConfirmation.h"

namespace ui
{

class FileOverwriteConfirmationHandler
{
private:
	std::size_t _msgSubscription;

public:
    FileOverwriteConfirmationHandler()
	{
		_msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
            radiant::IMessage::Type::FileOverwriteConfirmation,
			radiant::TypeListener<radiant::FileOverwriteConfirmation>(
				sigc::mem_fun(this, &FileOverwriteConfirmationHandler::handleRequest)));
	}

	~FileOverwriteConfirmationHandler()
	{
		GlobalRadiantCore().getMessageBus().removeListener(_msgSubscription);
	}

private:
	void handleRequest(radiant::FileOverwriteConfirmation& msg)
	{
        // Ask the user
        auto msgBox = GlobalDialogManager().createMessageBox(
            msg.hasTitle() ? msg.getTitle() : _("Confirm overwrite"),
            msg.getMessage(),
            ui::IDialog::MESSAGE_ASK
        );

        auto result = msgBox->run();

        if (result == ui::IDialog::RESULT_YES)
        {
            msg.confirmOverwrite();
        }

        msg.setHandled(true);
	}
};

}
