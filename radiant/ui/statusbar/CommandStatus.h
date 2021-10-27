#pragma once

#include "iundo.h"
#include "imap.h"

#include "messages/MapOperationMessage.h"

namespace ui
{

namespace statusbar
{

/**
 * Status bar widget displaying the most recently completed
 * operation or any undo / redo activitiy.
 */
class CommandStatus final
{
private:
    std::size_t _mapOperationListener;

public:
    CommandStatus();
    ~CommandStatus();

    void onOperationFinished(map::OperationMessage& message);
};

}

}
