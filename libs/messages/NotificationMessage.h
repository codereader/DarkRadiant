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
    std::string _title;
    std::string _message;

    Type _type;

public:
    NotificationMessage(const std::string& message, Type type) :
        NotificationMessage(std::string(), message, type)
    {}

    NotificationMessage(const std::string& title, const std::string& message, Type type) :
        _title(title),
        _message(message),
        _type(type)
    {}

    const std::string& getTitle() const
    {
        return _title;
    }

    bool hasTitle() const
    {
        return !_title.empty();
    }

    const std::string& getMessage() const
    {
        return _message;
    }

    std::size_t getId() const override
    {
        return IMessage::Type::Notification;
    }

    Type getType() const
    {
        return _type;
    }

    // Convenience method, creating an instance and dispatching it to the message bus
    static void SendInformation(const std::string& message, const std::string& title = std::string())
    {
        NotificationMessage msg(title, message, Information);
        GlobalRadiantCore().getMessageBus().sendMessage(msg);
    }

    static void SendWarning(const std::string& message, const std::string& title = std::string())
    {
        NotificationMessage msg(title, message, Warning);
        GlobalRadiantCore().getMessageBus().sendMessage(msg);
    }

    static void SendError(const std::string& message, const std::string& title = std::string())
    {
        NotificationMessage msg(title, message, Error);
        GlobalRadiantCore().getMessageBus().sendMessage(msg);
    }
};

}
