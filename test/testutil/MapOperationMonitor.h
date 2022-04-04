#pragma once

#include "imessagebus.h"
#include "iradiant.h"
#include "messages/MapOperationMessage.h"

namespace test
{

// Listens for MapOperationMessages
class MapOperationMonitor
{
private:
    std::size_t _msgSubscription;
    bool _messageReceived;
    std::string _lastReceivedMessage;

public:
    MapOperationMonitor() :
        _messageReceived(false)
    {
        // Subscribe to the event asking for the target path
        _msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
            radiant::IMessage::Type::MapOperationFinished,
            radiant::TypeListener<map::OperationMessage>(
                [this](map::OperationMessage& msg)
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

    ~MapOperationMonitor()
    {
        GlobalRadiantCore().getMessageBus().removeListener(_msgSubscription);
    }
};

}
