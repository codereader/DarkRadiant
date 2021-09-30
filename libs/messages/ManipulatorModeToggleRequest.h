#pragma once

#include "imessagebus.h"
#include "imanipulator.h"

namespace selection
{

/**
 * Message sent out by the main selection system when the user
 * hits a shortcut to toggle a manipulator mode.
 * This gives listeners a chance to react to the request
 * before it is handled by the RadiantSelectionSystem itself.
 * Unhandled requests will be processed as usual.
 */
class ManipulatorModeToggleRequest :
    public radiant::IMessage
{
private:
    bool _handled;
    IManipulator::Type _type;

public:
    ManipulatorModeToggleRequest(IManipulator::Type type) :
        _handled(false),
        _type(type)
    {}

    std::size_t getId() const override
    {
        return Type::ManipulatorModeToggleRequest;
    }

    IManipulator::Type getType() const
    {
        return _type;
    }

    // Handled requests will not be processed by the selection system
    void setHandled(bool handled)
    {
        _handled = true;
    }

    // TRUE whether this request has been handled
    bool isHandled() const
    {
        return _handled;
    }
};

}
