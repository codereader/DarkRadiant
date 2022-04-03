#pragma once

#include "imessagebus.h"
#include "iradiant.h"
#include "messages/CommandExecutionFailed.h"

namespace test
{

class CommandFailureHelper
{
private:
    std::size_t _msgSubscription;
    bool _messageReceived;
    std::string _lastReceivedMessage;

public:
    CommandFailureHelper() :
        _messageReceived(false)
    {
        // Subscribe to the event asking for the target path
        _msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
            radiant::IMessage::Type::CommandExecutionFailed,
            radiant::TypeListener<radiant::CommandExecutionFailedMessage>(
                [this](radiant::CommandExecutionFailedMessage& msg)
                {
                    _messageReceived = true;
                    _lastReceivedMessage = msg.getMessage();
                }));
    }

    bool messageReceived() const
    {
        return _messageReceived;
    }

    const std::string& getLastReceivedMessage() const
    {
        return _lastReceivedMessage;
    }

    ~CommandFailureHelper()
    {
        GlobalRadiantCore().getMessageBus().removeListener(_msgSubscription);
    }
};

}
