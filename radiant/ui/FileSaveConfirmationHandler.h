#pragma once

#include "ui/idialogmanager.h"
#include "iradiant.h"
#include "i18n.h"
#include "messages/FileSaveConfirmation.h"

namespace ui
{

class FileSaveConfirmationHandler
{
private:
    std::size_t _msgSubscription;

public:
    FileSaveConfirmationHandler()
    {
        _msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
            radiant::IMessage::Type::FileSaveConfirmation,
            radiant::TypeListener<radiant::FileSaveConfirmation>(
                sigc::mem_fun(*this, &FileSaveConfirmationHandler::handleRequest)));
    }

    ~FileSaveConfirmationHandler()
    {
        GlobalRadiantCore().getMessageBus().removeListener(_msgSubscription);
    }

private:
    void handleRequest(radiant::FileSaveConfirmation& msg)
    {
        // Ask the user
        auto msgBox = GlobalDialogManager().createMessageBox(
            msg.getTitle(), msg.getMessage(),
            ui::IDialog::MESSAGE_SAVECONFIRMATION
        );

        auto result = msgBox->run();

        msg.setHandled(true);

        switch (result)
        {
        case IDialog::RESULT_CANCELLED:
            msg.setAction(radiant::FileSaveConfirmation::Action::Cancel);
            break;
        case IDialog::RESULT_YES:
            msg.setAction(radiant::FileSaveConfirmation::Action::SaveChanges);
            break;
        default:
            msg.setAction(radiant::FileSaveConfirmation::Action::DiscardChanges);
        }
    }
};

}
