#pragma once

#include <stdexcept>
#include "imessagebus.h"
#include "iradiant.h"

namespace radiant
{

/**
 * Message sent when the backend code wants the user to
 * confirm overwriting a file. 
 * Senders: If this message stays unresponded no overwrite should occur.
 */
class FileOverwriteConfirmation :
    public radiant::IMessage
{
private:
    std::string _title;
    std::string _message;

    bool _confirmed;

public:
    FileOverwriteConfirmation(const std::string& title, const std::string& message) :
        _title(title),
        _message(message),
        _confirmed(false)
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
        return IMessage::Type::FileOverwriteConfirmation;
    }

    void confirmOverwrite(bool confirmed = true)
    {
        _confirmed = confirmed;
    }

    bool overwriteConfirmed()
    {
        return isHandled() && _confirmed;
    }

    // Convenience method, creating an instance and dispatching it to the message bus, returning the answer
    static bool SendAndReceiveAnswer(const std::string& message, const std::string& title = std::string())
    {
        FileOverwriteConfirmation msg(title, message);
        GlobalRadiantCore().getMessageBus().sendMessage(msg);

        return msg.overwriteConfirmed();
    }
};

}