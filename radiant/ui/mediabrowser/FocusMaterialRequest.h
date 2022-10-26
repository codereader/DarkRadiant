#pragma once

#include "imessagebus.h"
#include "iradiant.h"

namespace ui
{

/**
 * Request to focus a specific material in the MediaBrowser
 */
class FocusMaterialRequest :
    public radiant::IMessage
{
private:
    std::string _requestedMaterial;
public:
    FocusMaterialRequest(const std::string& material) :
        _requestedMaterial(material)
    {}

    std::size_t getId() const override
    {
        return Type::FocusMaterialRequest;
    }

    const std::string& getRequestedMaterial() const
    {
        return _requestedMaterial;
    }

    // Convenience method
    static void Send(const std::string& material)
    {
        FocusMaterialRequest msg(material);
        GlobalRadiantCore().getMessageBus().sendMessage(msg);
    }
};

}
