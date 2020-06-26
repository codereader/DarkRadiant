#pragma once

#include "imessagebus.h"

namespace map
{

class AutomaticMapSaveRequest :
    public radiant::IMessage
{
private:
    bool _denied;
    std::string _reason;

public:
    AutomaticMapSaveRequest()
    {}

    std::size_t getId() const override
    {
        return Type::AutomaticMapSaveRequest;
    }

    // Deny this request to prevent the save from happening
    void denyWithReason(const std::string& reason)
    {
        _denied = true;
        _reason = reason;
    }

    const std::string& getReason() const
    {
        return _reason;
    }

    // TRUE whether this request has been denied and auto save should not happen
    bool isDenied() const
    {
        return _denied;
    }
};

}
