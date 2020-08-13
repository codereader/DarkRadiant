#include "EditingStopwatch.h"

#include <sigc++/functors/mem_fun.h>

#include "i18n.h"
#include "iradiant.h"
#include "itextstream.h"
#include "imainframe.h"
#include "imap.h"
#include "imapresource.h"
#include "imapinfofile.h"

#include <fmt/format.h>
#include "string/convert.h"
#include "module/StaticModule.h"
#include "EditingStopwatchInfoFileModule.h"

namespace map
{

namespace
{
	const int TIMER_INTERVAL_SECS = 1;
	
	const char* const MAP_PROPERTY_KEY = "EditTimeInSeconds";
}

EditingStopwatch::EditingStopwatch() :
	_secondsEdited(0)
{}

const std::string& EditingStopwatch::getName() const
{
	static std::string _name(MODULE_EDITING_STOPWATCH);
	return _name;
}

const StringSet& EditingStopwatch::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_MAP);
		_dependencies.insert(MODULE_MAPINFOFILEMANAGER);
		_dependencies.insert(MODULE_MAPRESOURCEMANAGER);
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

	GlobalMapResourceManager().signal_onResourceExporting().connect(
		sigc::mem_fun(this, &EditingStopwatch::onResourceExporting)
	);

	// Register the timer when the application has come up
	GlobalRadiant().signal_radiantStarted().connect(
		sigc::mem_fun(*this, &EditingStopwatch::onRadiantStartup));
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
	// TODO: Send signal over messagebus
	if (GlobalMainFrame().isActiveApp() && GlobalMainFrame().screenUpdatesEnabled())
	{
		setTotalSecondsEdited(getTotalSecondsEdited() + TIMER_INTERVAL_SECS);
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
		// Check if we have a non-empty property on the map root node
		readFromMapProperties();
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
		// the timing is not written to the map root node here,
		// but in the separate resource-exporting event
		stop();
		break;
	case IMap::MapSaved:
		start();
		break;
		
	default:
		break;
	};
}

void EditingStopwatch::onResourceExporting(const scene::IMapRootNodePtr& root)
{
	writeToMapProperties(root);
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
	_sigTimerChanged.emit();
}

sigc::signal<void>& EditingStopwatch::sig_TimerChanged()
{
	return _sigTimerChanged;
}

void EditingStopwatch::readFromMapProperties()
{
	auto root = GlobalMapModule().getRoot();

	if (root && !root->getProperty(MAP_PROPERTY_KEY).empty())
	{
		auto value = string::convert<unsigned long>(root->getProperty(MAP_PROPERTY_KEY));

		rMessage() << "Read " << value << " seconds of total map editing time." << std::endl;

		setTotalSecondsEdited(value);
	}
}

void EditingStopwatch::writeToMapProperties(const scene::IMapRootNodePtr& root)
{
	if (root)
	{
		root->setProperty(MAP_PROPERTY_KEY, string::to_string(getTotalSecondsEdited()));
	}
}

// Static module registration
module::StaticModule<EditingStopwatch> _stopwatchModule;

}
