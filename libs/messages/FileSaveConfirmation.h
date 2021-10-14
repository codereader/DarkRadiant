#pragma once

#include <stdexcept>
#include "imessagebus.h"
#include "iradiant.h"

namespace radiant
{

/**
 * Message sent when the backend code wants the user to confirm saving a modified file
 * before a new map is loaded, with the option to Save the modified map, Discard the changes
 * or to cancel the request.
 * Senders: If this message stays unresponded the switch is allowed.
 */
class FileSaveConfirmation :
    public radiant::IMessage
{
public:
    enum class Action
    {
        SaveChanges,
        DiscardChanges,
        Cancel,
    };

private:
    std::string _title;
    std::string _message;

    Action _action;

public:
    FileSaveConfirmation(const std::string& title, const std::string& message) :
        _title(title),
        _message(message),
        _action(Action::DiscardChanges)
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
        return IMessage::Type::FileSaveConfirmation;
    }

    Action getChosenAction() const
    {
        return _action;
    }

    void setAction(Action action)
    {
        _action = action;
    }

    // Convenience method, creating an instance and dispatching it to the message bus, returning the answer
    static Action SendAndReceiveAnswer(const std::string& message, const std::string& title = std::string())
    {
        FileSaveConfirmation msg(title, message);
        GlobalRadiantCore().getMessageBus().sendMessage(msg);

        return msg.getChosenAction();
    }
};

}
