#include "Console.h"

#include "iuimanager.h"
#include "igroupdialog.h"

#include "wxutil/ConsoleView.h"
#include <wx/sizer.h>

#include "LogLevels.h"
#include "LogWriter.h"
#include "StringLogDevice.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string/replace.hpp>

namespace ui {

Console::Console(wxWindow* parent) :
	wxPanel(parent, wxID_ANY),
	_view(new wxutil::ConsoleView(this)),
	_commandEntry(new CommandEntry(this))
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	GetSizer()->Add(_view, 1, wxEXPAND);
	GetSizer()->Add(_commandEntry, 0, wxEXPAND);

	// We're ready to catch log output, register ourselves
	applog::LogWriter::Instance().attach(this);

	// Copy the temporary buffers over
	if (applog::StringLogDevice::InstancePtr() != NULL)
	{
		applog::StringLogDevice& logger = *applog::StringLogDevice::InstancePtr();

		for (int level = applog::SYS_VERBOSE;
			 level < applog::SYS_NUM_LOGLEVELS;
			 level++)
		{
			writeLog(logger.getString(static_cast<applog::ELogLevel>(level)) + "\n",
				static_cast<applog::ELogLevel>(level));
		}
	}

	// Destruct the temporary buffer
	applog::StringLogDevice::destroy();

	GlobalCommandSystem().addCommand("clear", boost::bind(&Console::clearCmd, this, _1));
}

Console::~Console()
{
	// TODO - there might be more than one console instance handle this
	GlobalCommandSystem().removeCommand("clear");

	applog::LogWriter::Instance().detach(this);
}

void Console::clearCmd(const cmd::ArgumentList& args)
{
	_view->Clear();
}

void Console::toggle(const cmd::ArgumentList& args)
{
	GlobalGroupDialog().togglePage("console");
}

void Console::writeLog(const std::string& outputStr, applog::ELogLevel level)
{
	switch (level)
	{
		case applog::SYS_VERBOSE:
		case applog::SYS_STANDARD:
			_view->appendText(outputStr, wxutil::ConsoleView::ModeStandard);
			break;
		case applog::SYS_WARNING:
			_view->appendText(outputStr, wxutil::ConsoleView::ModeWarning);
			break;
		case applog::SYS_ERROR:
			_view->appendText(outputStr, wxutil::ConsoleView::ModeError);
			break;
		default:
			_view->appendText(outputStr, wxutil::ConsoleView::ModeStandard);
	};
}

} // namespace ui
