#pragma once

#include "imessagebus.h"
#include "iselection.h"

namespace selection
{

/**
 * Message sent out by the main selection system when the user
 * hits a shortcut to toggle the component selection mode.
 * This gives listeners a chance to react to the request
 * before it is handled by the RadiantSelectionSystem itself.
 * Unhandled requests will be processed as usual.
 */
class ComponentSelectionModeToggleRequest :
    public radiant::IMessage
{
private:
    bool _handled;
    ComponentSelectionMode _mode;

public:
    ComponentSelectionModeToggleRequest(ComponentSelectionMode mode) :
        _handled(false),
        _mode(mode)
    {}

    std::size_t getId() const override
    {
        return Type::ComponentSelectionModeToggleRequest;
    }

    ComponentSelectionMode getMode() const
    {
        return _mode;
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
