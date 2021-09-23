#pragma once

#include "imessagebus.h"

namespace selection
{

/**
 * Message sent out by the main selection system when the user
 * hits ESC (or the custom shortcut bound to "UnselectSelection").
 * This gives listeners a chance to react to the unselect request
 * before it is handled by the RadiantSelectionSystem.
 * Unhandled requests will be processed as usual.
 */
class UnselectSelectionRequest :
    public radiant::IMessage
{
private:
    bool _handled;

public:
    UnselectSelectionRequest() :
        _handled(false)
    {}

    std::size_t getId() const override
    {
        return Type::UnselectSelectionRequest;
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
