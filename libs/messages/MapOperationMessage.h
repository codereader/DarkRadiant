#pragma once

#include "iradiant.h"
#include "imessagebus.h"

namespace map
{

/**
 * Message sent when an operation affecting the loaded map is finished.
 * This can be an ordinary edit operation, loading, saving, or an undo/redo step.
 */
class OperationMessage :
    public radiant::IMessage
{
private:
    std::string _message;

public:
    OperationMessage(const std::string& message) :
        _message(message)
    {}

    std::size_t getId() const override
    {
        return IMessage::Type::MapOperationFinished;
    }

    const std::string& getMessage() const
    {
        return _message;
    }

    static void Send(const std::string& message)
    {
        OperationMessage msg(message);
        GlobalRadiantCore().getMessageBus().sendMessage(msg);
    }
};

}
