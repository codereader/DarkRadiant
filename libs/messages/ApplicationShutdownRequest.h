#pragma once

#include "imessagebus.h"

namespace radiant
{

/**
 * Message that is broadcast when the user requests to close
 * the application. Subscribes can deny() the request to prevent
 * the loss of unsaved data or similar.
 */
class ApplicationShutdownRequest :
    public radiant::IMessage
{
private:
    bool _denied;

public:
    ApplicationShutdownRequest() :
        _denied(false)
    {}

    std::size_t getId() const override
    {
        return Type::ApplicationShutdownRequest;
    }

    // Deny this request to keep the app running
    void deny()
    {
        _denied = true;
    }

    // TRUE whether this request has been denied and the app should stay alive
    bool isDenied() const
    {
        return _denied;
    }
};

}
