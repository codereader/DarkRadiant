#include "Console.h"

#include "iuimanager.h"
#include "igroupdialog.h"
#include "iradiant.h"

#include "wxutil/ConsoleView.h"
#include <wx/sizer.h>

#include <functional>

namespace ui
{

Console::Console(wxWindow* parent) :
	wxPanel(parent, wxID_ANY),
	_view(new wxutil::ConsoleView(this)),
	_commandEntry(new CommandEntry(this))
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	GetSizer()->Add(_view, 1, wxEXPAND);
	GetSizer()->Add(_commandEntry, 0, wxEXPAND);

    GlobalCommandSystem().addCommand("clear",
        std::bind(&Console::clearCmd, this, std::placeholders::_1));

    // Get a lock on the logging system before doing these changes
    std::lock_guard<std::mutex> lock(GlobalRadiantCore().getLogWriter().getStreamLock());

	// We're ready to catch log output, register ourselves
	GlobalRadiantCore().getLogWriter().attach(this);
}

Console::~Console()
{
	// TODO - there might be more than one console instance handle this
	GlobalCommandSystem().removeCommand("clear");

	GlobalRadiantCore().getLogWriter().detach(this);
}

void Console::clearCmd(const cmd::ArgumentList& args)
{
	_view->Clear();
}

void Console::toggle(const cmd::ArgumentList& args)
{
	GlobalGroupDialog().togglePage("console");
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
