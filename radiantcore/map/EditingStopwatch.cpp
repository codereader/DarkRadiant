#include "EditingStopwatch.h"

#include <sigc++/functors/mem_fun.h>

#include "iradiant.h"
#include "itextstream.h"
#include "imap.h"
#include "imapresource.h"
#include "imapinfofile.h"

#include <fmt/format.h>
#include "string/convert.h"
#include "module/StaticModule.h"
#include "EditingStopwatchInfoFileModule.h"
#include "messages/ApplicationIsActiveRequest.h"

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

void EditingStopwatch::initialiseModule(const IApplicationContext& ctx)
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

	_timer.reset(new util::Timer(TIMER_INTERVAL_SECS * 1000,
		sigc::mem_fun(*this, &EditingStopwatch::onIntervalReached)));
}

void EditingStopwatch::shutdownModule()
{
	stop();
	_mapSignal.disconnect();
}

void EditingStopwatch::onIntervalReached()
{
	if (applicationIsActive())
	{
		setTotalSecondsEdited(getTotalSecondsEdited() + TIMER_INTERVAL_SECS);
	}
}

bool EditingStopwatch::applicationIsActive()
{
	radiant::ApplicationIsActiveRequest msg;
	GlobalRadiantCore().getMessageBus().sendMessage(msg);

	return msg.getApplicationIsActive();
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
		_timer->start(TIMER_INTERVAL_SECS * 1000);
	}
}

void EditingStopwatch::stop()
{
	if (_timer)
	{
		_timer->stop();
	}
}

unsigned long EditingStopwatch::getTotalSecondsEdited()
{
	std::lock_guard<std::recursive_mutex> lock(_timingMutex);

	return _secondsEdited;
}

void EditingStopwatch::setTotalSecondsEdited(unsigned long newValue)
{
	std::lock_guard<std::recursive_mutex> lock(_timingMutex);

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
module::StaticModuleRegistration<EditingStopwatch> _stopwatchModule;

}
