#pragma once

#include "imessagebus.h"
#include "iselection.h"

namespace selection
{

/**
 * Message sent out by the main selection system when the user
 * hits a shortcut to snap the selection to the grid.
 * This gives listeners a chance to react to the request
 * before it is handled by the RadiantSelectionSystem itself.
 * Unhandled requests will be processed as usual.
 */
class GridSnapRequest :
    public radiant::IMessage
{
private:
    bool _handled;

public:
    GridSnapRequest() :
        _handled(false)
    {}

    std::size_t getId() const override
    {
        return Type::GridSnapRequest;
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
