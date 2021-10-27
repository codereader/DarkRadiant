#include "CommandStatus.h"

#include "ui/istatusbarmanager.h"
#include "iradiant.h"

namespace ui
{

namespace statusbar
{

namespace
{
    const char* const STATUS_BAR_ELEMENT = "Commands";
}

CommandStatus::CommandStatus()
{
    _mapOperationListener = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::MapOperationFinished,
        radiant::TypeListener<map::OperationMessage>(
            sigc::mem_fun(this, &CommandStatus::onOperationFinished)));

    // Add the status bar element
    GlobalStatusBarManager().addTextElement(STATUS_BAR_ELEMENT, "", StandardPosition::Commands, "");
}

CommandStatus::~CommandStatus()
{
    GlobalRadiantCore().getMessageBus().removeListener(_mapOperationListener);
}

void CommandStatus::onOperationFinished(map::OperationMessage& message)
{
    GlobalStatusBarManager().setText(STATUS_BAR_ELEMENT, message.getMessage(), false);
}

}

}
