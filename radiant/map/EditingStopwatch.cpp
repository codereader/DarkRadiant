#include "EditingStopwatch.h"

#include <sigc++/functors/mem_fun.h>

#include "i18n.h"
#include "iradiant.h"
#include "iuimanager.h"
#include "itextstream.h"
#include "imainframe.h"
#include "imap.h"
#include "imapinfofile.h"

#include <fmt/format.h>
#include "modulesystem/StaticModule.h"
#include "EditingStopwatchInfoFileModule.h"

namespace map
{

namespace
{
	const int TIMER_INTERVAL_SECS = 1;
	const char* const STATUS_BAR_ELEMENT = "EditTime";
}

EditingStopwatch::EditingStopwatch() :
	_secondsEdited(0)
{}

const std::string& EditingStopwatch::getName() const
{
	static std::string _name("EditingStopwatch");
	return _name;
}

const StringSet& EditingStopwatch::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_MAP);
		_dependencies.insert(MODULE_MAPINFOFILEMANAGER);
		_dependencies.insert(MODULE_UIMANAGER);
	}

	return _dependencies;
}

void EditingStopwatch::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	_mapSignal = GlobalMapModule().signal_mapEvent().connect(
		sigc::mem_fun(*this, &EditingStopwatch::onMapEvent)
	);

	GlobalMapInfoFileManager().registerInfoFileModule(
		std::make_shared<EditingStopwatchInfoFileModule>()
	);

	// Register the timer when the application has come up
	GlobalRadiant().signal_radiantStarted().connect(
		sigc::mem_fun(*this, &EditingStopwatch::onRadiantStartup));

	// Add the status bar element
	GlobalUIManager().getStatusBarManager().addTextElement(STATUS_BAR_ELEMENT, "stopwatch.png", 
		IStatusBarManager::POS_MAP_EDIT_TIME, _("Time spent on this map"));
	GlobalUIManager().getStatusBarManager().setText(STATUS_BAR_ELEMENT, "00:00:00");
}

void EditingStopwatch::shutdownModule()
{
	stop();
	_mapSignal.disconnect();
}

void EditingStopwatch::onRadiantStartup()
{
	Bind(wxEVT_TIMER, sigc::mem_fun(*this, &EditingStopwatch::onIntervalReached));

	_timer.reset(new wxTimer(this));
	start();
}

void EditingStopwatch::onIntervalReached(wxTimerEvent& ev)
{
	if (GlobalMainFrame().isActiveApp() && GlobalMainFrame().screenUpdatesEnabled())
	{
		_secondsEdited += TIMER_INTERVAL_SECS;

		// Format the time and pass it to the status bar
		unsigned long hours = _secondsEdited / 3600;
		unsigned long minutes = (_secondsEdited % 3600) / 60;
		unsigned long seconds = _secondsEdited % 60;

		GlobalUIManager().getStatusBarManager().setText(STATUS_BAR_ELEMENT,
			fmt::format("{0:02d}:{1:02d}:{2:02d}", hours, minutes, seconds));
	}
}

void EditingStopwatch::onMapEvent(IMap::MapEvent ev)
{
	switch (ev)
	{
	// We reset the timer when a map is about to be loaded,
	// such that we don't use any previously found value.
	// If a persisted info could be found in the info file,
	// the new value will be set by the InfoFileModule.
	case IMap::MapLoading:
		stop();
		setTotalSecondsEdited(0);
		break;

	// Start the clock once the map is done loading
	case IMap::MapLoaded:
		start();
		break;
		
	// When a map is unloaded, we reset the value to 0 again
	// to prevent leaving stuff behind.
	case IMap::MapUnloaded:
		stop();
		setTotalSecondsEdited(0);
		break;

	// We start/stop during save operations
	case IMap::MapSaving:
		stop();
		break;
	case IMap::MapSaved:
		start();
		break;
		
	default:
		break;
	};
}

void EditingStopwatch::start()
{
	if (_timer)
	{
		_timer->Start(TIMER_INTERVAL_SECS * 1000);
	}
}

void EditingStopwatch::stop()
{
	if (_timer)
	{
		_timer->Stop();
	}
}

unsigned long EditingStopwatch::getTotalSecondsEdited()
{
	return _secondsEdited;
}

void EditingStopwatch::setTotalSecondsEdited(unsigned long newValue)
{
	_secondsEdited = newValue;
}

// Static module registration
module::StaticModule<EditingStopwatch> _stopwatchModule;

EditingStopwatch& EditingStopwatch::GetInstanceInternal()
{
	return *_stopwatchModule.getModule();
}

}
