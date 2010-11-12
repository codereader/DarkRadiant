#include "Console.h"

#include "iuimanager.h"
#include "igroupdialog.h"

#include "gtkutil/nonmodal.h"
#include "gtkutil/IConv.h"

#include "LogLevels.h"
#include "LogWriter.h"
#include "StringLogDevice.h"

#include <boost/bind.hpp>
#include <boost/algorithm/string/replace.hpp>

namespace ui {

Console::Console() :
	Gtk::VBox(false, 6),
	_view(Gtk::manage(new gtkutil::ConsoleView)),
	_commandEntry(Gtk::manage(new CommandEntry))
{
	// Pack the scrolled textview and the entry box to the vbox
	pack_start(*_view, true, true, 0);
	pack_start(*_commandEntry, false, false, 0);
	show_all();

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

void Console::shutdown()
{
	applog::LogWriter::Instance().detach(this);

	GlobalCommandSystem().removeCommand("clear");
}

void Console::destroy()
{
	if (InstancePtr() != NULL)
	{
		Instance().shutdown();
		InstancePtr().reset();
	}
}

Console& Console::Instance()
{
	ConsolePtr& instance = InstancePtr();

	if (instance == NULL)
	{
		instance.reset(new Console);
	}

	return *instance;
}

ConsolePtr& Console::InstancePtr()
{
	static ConsolePtr _consolePtr;
	return _consolePtr;
}

} // namespace ui
