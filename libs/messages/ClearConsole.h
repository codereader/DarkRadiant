#pragma once

#include "imessagebus.h"
#include "iradiant.h"

namespace radiant
{

/**
 * Message object sent when the active console views should be cleared.
 */
class ClearConsoleMessage :
	public IMessage
{
public:
    std::size_t getId() const override
    {
        return Type::ClearConsole;
    }

    // Convenience method
    static void Send()
    {
        ClearConsoleMessage msg;
        GlobalRadiantCore().getMessageBus().sendMessage(msg);
    }
};

}
