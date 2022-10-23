#include "Console.h"

#include "iradiant.h"

#include "wxutil/ConsoleView.h"
#include <wx/sizer.h>

#include "messages/ClearConsole.h"

namespace ui
{

Console::Console(wxWindow* parent) :
	DockablePanel(parent),
	_view(new wxutil::ConsoleView(this)),
	_commandEntry(new CommandEntry(this))
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	GetSizer()->Add(_view, 1, wxEXPAND);
	GetSizer()->Add(_commandEntry, 0, wxEXPAND);

    // Get a lock on the logging system before doing these changes
    std::lock_guard<std::mutex> lock(GlobalRadiantCore().getLogWriter().getStreamLock());

	// We're ready to catch log output, register ourselves
	GlobalRadiantCore().getLogWriter().attach(this);

    _clearConsoleHandler = GlobalRadiantCore().getMessageBus().addListener(
        radiant::IMessage::Type::ClearConsole,
        radiant::TypeListener<radiant::ClearConsoleMessage>(
            sigc::mem_fun(this, &Console::clear)));
}

Console::~Console()
{
    GlobalRadiantCore().getMessageBus().removeListener(_clearConsoleHandler);
	GlobalRadiantCore().getLogWriter().detach(this);
}

void Console::clear(radiant::ClearConsoleMessage& msg)
{
	_view->Clear();
}

void Console::SetFocus()
{
    // Propagate focus to command entry
    _commandEntry->SetFocus();
}

void Console::writeLog(const std::string& outputStr, applog::LogLevel level)
{
	switch (level)
	{
	case applog::LogLevel::Verbose:
	case applog::LogLevel::Standard:
		_view->appendText(outputStr, wxutil::ConsoleView::ModeStandard);
		break;
	case applog::LogLevel::Warning:
		_view->appendText(outputStr, wxutil::ConsoleView::ModeWarning);
		break;
	case applog::LogLevel::Error:
		_view->appendText(outputStr, wxutil::ConsoleView::ModeError);
		break;
	default:
		_view->appendText(outputStr, wxutil::ConsoleView::ModeStandard);
	};
}

} // namespace ui
