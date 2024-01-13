#pragma once

#include "imessagebus.h"
#include "iradiant.h"
#include "messages/AutomaticMapSaveRequest.h"

namespace wxutil
{

class AutoSaveRequestBlocker
{
private:
    std::size_t _msgSubscription;
    std::string _reason;

public:
    AutoSaveRequestBlocker(const std::string& reason) :
        _reason(reason)
    {
        _msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
            radiant::IMessage::Type::AutomaticMapSaveRequest,
            radiant::TypeListener<map::AutomaticMapSaveRequest>(
                sigc::mem_fun(*this, &AutoSaveRequestBlocker::handleRequest)));
    }

    ~AutoSaveRequestBlocker()
    {
        GlobalRadiantCore().getMessageBus().removeListener(_msgSubscription);
    }

private:
    void handleRequest(map::AutomaticMapSaveRequest& msg)
    {
        msg.denyWithReason(_reason);
    }
};

}
