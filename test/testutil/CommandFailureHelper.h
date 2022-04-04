#pragma once

#include "imessagebus.h"
#include "iradiant.h"
#include "messages/CommandExecutionFailed.h"
#include "command/ExecutionNotPossible.h"

namespace test
{

class CommandFailureHelper
{
private:
    std::size_t _msgSubscription;
    bool _messageReceived;
    std::string _lastReceivedMessage;
    bool _executionNotPossible;

public:
    CommandFailureHelper() :
        _messageReceived(false),
        _executionNotPossible(false)
    {
        // Subscribe to the event asking for the target path
        _msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
            radiant::IMessage::Type::CommandExecutionFailed,
            radiant::TypeListener<radiant::CommandExecutionFailedMessage>(
                [this](radiant::CommandExecutionFailedMessage& msg)
                {
                    _messageReceived = true;
                    _lastReceivedMessage = msg.getMessage();

                    // Check the exception type more closely
                    _executionNotPossible = dynamic_cast<const cmd::ExecutionNotPossible*>(&msg.getException()) != nullptr;
                }));
    }

    bool messageReceived() const
    {
        return _messageReceived;
    }

    bool executionHasNotBeenPossible() const
    {
        return _executionNotPossible;
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
