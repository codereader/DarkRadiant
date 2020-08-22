#pragma once

#include "imessagebus.h"

namespace radiant
{

/**
 * Message sent to query whether DarkRadiant is active.
 * The UI module should respond to this request and set the flag
 * in the message instance accordingly, using setApplicationIsActive().
 *
 * If no code responds to this query, the application is considered
 * active by default.
 */
class ApplicationIsActiveRequest :
    public radiant::IMessage
{
private:
    bool _isActive;

public:
    ApplicationIsActiveRequest() :
        _isActive(true)
    {}

    std::size_t getId() const override
    {
        return Type::ApplicationIsActiveQuery;
    }

    void setApplicationIsActive(bool isActive)
    {
        _isActive = isActive;
    }

    // TRUE if the application is currently active
    bool getApplicationIsActive() const
    {
        return _isActive;
    }
};

}
