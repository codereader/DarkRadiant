#include "Console.h"

#include "iuimanager.h"
#include "igroupdialog.h"

#include "gtkutil/nonmodal.h"
#include "gtkutil/IConv.h"
#include "gtkutil/ConsoleView.h"
#include <wx/sizer.h>

#include "LogLevels.h"
#include "LogWriter.h"
#include "StringLogDevice.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string/replace.hpp>

namespace ui {

Console::Console(wxWindow* parent) :
	wxPanel(parent, wxID_ANY),
	_view(Gtk::manage(new gtkutil::ConsoleView)),
	_commandEntry(new CommandEntry(this))
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	// wxTODO: ConsoleView
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
	// wxTODO - there might be more than one console instance handle this
	GlobalCommandSystem().removeCommand("clear");

	applog::LogWriter::Instance().detach(this);
}

void Console::clearCmd(const cmd::ArgumentList& args)
{
	_view->clear();
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
			_view->appendText(outputStr, gtkutil::ConsoleView::STANDARD);
			break;
		case applog::SYS_WARNING:
			_view->appendText(outputStr, gtkutil::ConsoleView::WARNING);
			break;
		case applog::SYS_ERROR:
			_view->appendText(outputStr, gtkutil::ConsoleView::ERROR);
			break;
		default:
			_view->appendText(outputStr, gtkutil::ConsoleView::STANDARD);
	};
}

} // namespace ui
