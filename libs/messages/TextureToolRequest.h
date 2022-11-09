#pragma once

#include "imessagebus.h"
#include "iradiant.h"

namespace ui
{

/**
 * Message object sent to have all active TextureTool views to perform an action
 */
class TextureToolRequest :
	public radiant::IMessage
{
public:
    enum Action
    {
        ResetView,
        UseLightTheme,
        UseDarkTheme,
        QueueViewRefresh,
        ForceViewRefresh,
    };

private:
    Action _action;

public:
    TextureToolRequest(Action action) :
        _action(action)
    {}

    std::size_t getId() const override
    {
        return Type::TextureToolRequest;
    }

    Action getAction() const
    {
        return _action;
    }

    // Convenience method
    static void Send(Action action)
    {
        TextureToolRequest msg(action);
        GlobalRadiantCore().getMessageBus().sendMessage(msg);
    }
};

}
