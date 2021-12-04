#include "CommandStatus.h"

#include "i18n.h"
#include "ui/istatusbarmanager.h"
#include "iradiant.h"

namespace ui
{

namespace statusbar
{

namespace
{
    const char* const STATUS_BAR_ELEMENT = "Commands";
    const int MESSAGE_LIFETIME_MSECS = 4000;
}

CommandStatus::CommandStatus() :
    _timer(this)
{
    _mapOperationListener = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::MapOperationFinished,
        radiant::TypeListener<map::OperationMessage>(
            sigc::mem_fun(this, &CommandStatus::onOperationFinished)));

    // Add the status bar element
    GlobalStatusBarManager().addTextElement(STATUS_BAR_ELEMENT, "", StandardPosition::Commands, "");

    Bind(wxEVT_TIMER, &CommandStatus::onTimerIntervalReached, this);
}

CommandStatus::~CommandStatus()
{
    _timer.Stop();
    GlobalRadiantCore().getMessageBus().removeListener(_mapOperationListener);
}

void CommandStatus::onOperationFinished(map::OperationMessage& message)
{
    GlobalStatusBarManager().setText(STATUS_BAR_ELEMENT, message.getMessage(), false);

    // (re-)start the timer on every message
    _timer.Start(MESSAGE_LIFETIME_MSECS, true);
}

void CommandStatus::onTimerIntervalReached(wxTimerEvent& ev)
{
    // Clear the timer when the interval is reached
    GlobalStatusBarManager().setText(STATUS_BAR_ELEMENT, _("Ready"), false);
}

}

}
