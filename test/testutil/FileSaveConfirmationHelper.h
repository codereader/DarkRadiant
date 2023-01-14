#pragma once

#include "messages/FileSaveConfirmation.h"

namespace test
{

class FileSaveConfirmationHelper
{
private:
    std::size_t _msgSubscription;
    radiant::FileSaveConfirmation::Action _actionToTake;
    bool _messageReceived;

public:
    FileSaveConfirmationHelper(radiant::FileSaveConfirmation::Action actionToTake) :
        _actionToTake(actionToTake),
        _messageReceived(false)
    {
        // Subscribe to the event asking for the target path
        _msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
            radiant::IMessage::Type::FileSaveConfirmation,
            radiant::TypeListener<radiant::FileSaveConfirmation>(
                [this](radiant::FileSaveConfirmation& msg)
        {
            _messageReceived = true;
            msg.setAction(_actionToTake);
            msg.setHandled(true);
        }));
    }

    bool messageReceived() const
    {
        return _messageReceived;
    }

    ~FileSaveConfirmationHelper()
    {
        GlobalRadiantCore().getMessageBus().removeListener(_msgSubscription);
    }
};

}
