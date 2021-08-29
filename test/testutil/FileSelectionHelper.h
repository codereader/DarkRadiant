#pragma once

#include "imapformat.h"
#include "iradiant.h"
#include "imessagebus.h"
#include "messages/FileSelectionRequest.h"

namespace test
{

/**
 * When the map algorithm asks for a map filename and format through messaging, 
 * this class responds to the request with the values passed to the ctor.
 */
class FileSelectionHelper
{
private:
    std::size_t _msgSubscription;
    std::string _path;
    map::MapFormatPtr _format;

public:
    FileSelectionHelper(const std::string& path, const map::MapFormatPtr& format) :
        _path(path),
        _format(format)
    {
        // Subscribe to the event asking for the target path
        _msgSubscription = GlobalRadiantCore().getMessageBus().addListener(
            radiant::IMessage::Type::FileSelectionRequest,
            radiant::TypeListener<radiant::FileSelectionRequest>(
                [this](radiant::FileSelectionRequest& msg)
        {
            msg.setHandled(true);
            msg.setResult(radiant::FileSelectionRequest::Result
                {
                    _path,
                    _format->getMapFormatName()
                });
        }));
    }

    ~FileSelectionHelper()
    {
        GlobalRadiantCore().getMessageBus().removeListener(_msgSubscription);
    }
};

}
