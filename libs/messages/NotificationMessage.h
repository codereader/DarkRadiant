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
public:
    enum Type
    {
        Information,
        Warning,
        Error,
    };

private:
    std::string _message;

    Type _type;

public:
    NotificationMessage(const std::string& message, Type type) :
        _message(message),
        _type(type)
    {}

    const std::string& getMessage() const
    {
        return _message;
    }

    Type getType() const
    {
        return _type;
    }

    // Convenience method, creating an instance and dispatching it to the message bus
    static void SendInformation(const std::string& message)
    {
        NotificationMessage msg(message, Information);
        GlobalRadiantCore().getMessageBus().sendMessage(msg);
    }

    static void SendWarning(const std::string& message)
    {
        NotificationMessage msg(message, Warning);
        GlobalRadiantCore().getMessageBus().sendMessage(msg);
    }

    static void SendError(const std::string& message)
    {
        NotificationMessage msg(message, Error);
        GlobalRadiantCore().getMessageBus().sendMessage(msg);
    }
};

}
