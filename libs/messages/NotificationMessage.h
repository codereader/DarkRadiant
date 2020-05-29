#pragma once

#include "imessagebus.h"
#include "iradiant.h"

namespace radiant
{

/**
 * Message object sent to the MessageBus to notify about something.
 */
class NotificationMessage :
    public IMessage
{
private:
    std::string _message;
public:
    NotificationMessage(const std::string& message) :
        _message(message)
    {}

    const std::string& getMessage() const
    {
        return _message;
    }

    // Convenience method, creating an instance and dispatching it to the message bus
    static void Send(const std::string& message)
    {
        NotificationMessage msg(message);
        GlobalRadiantCore().getMessageBus().sendMessage(msg);
    }
};

}
