#pragma once

#include "imessagebus.h"
#include "iradiant.h"

namespace radiant
{

/**
 * Message object sent when any of the texturable objects
 * in the scene have been changed. Any UI dealing with
 * textures might need to update their controls.
 */
class TextureChangedMessage :
	public IMessage
{
public:
	TextureChangedMessage()
	{}

    std::size_t getId() const override
    {
        return Type::TextureChanged;
    }

    // Convenience method
    static void Send()
    {
        TextureChangedMessage msg;
        GlobalRadiantCore().getMessageBus().sendMessage(msg);
    }
};

}
