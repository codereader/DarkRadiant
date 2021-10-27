#pragma once

#include "iundo.h"
#include "imap.h"
#include <wx/timer.h>

#include "messages/MapOperationMessage.h"

namespace ui
{

namespace statusbar
{

/**
 * Status bar widget displaying the most recently completed
 * operation or any undo / redo activitiy.
 */
class CommandStatus final :
    public wxEvtHandler
{
private:
    std::size_t _mapOperationListener;

    wxTimer _timer;

public:
    CommandStatus();
    ~CommandStatus();

private:
    void onOperationFinished(map::OperationMessage& message);
    void onTimerIntervalReached(wxTimerEvent& ev);
};

}

}
