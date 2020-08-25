#pragma once

#include "ieditstopwatch.h"
#include "imap.h"

#include <memory>
#include <mutex>
#include "time/Timer.h"
#include <sigc++/connection.h>

namespace map
{

/**
* Stopwatch to measure the time spent editing a particular map. 
* The time is persisted to the .darkradiant file and keeps running
* as long as the application is in focus.
*/
class EditingStopwatch :
	public IMapEditStopwatch
{
private:
	sigc::connection _mapSignal;

	// Mapping timer in second resolution
	unsigned long _secondsEdited;

	// Helper object to get called per second
	std::unique_ptr<util::Timer> _timer;

	sigc::signal<void> _sigTimerChanged;

	std::recursive_mutex _timingMutex;

public:
	EditingStopwatch();

	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
	void shutdownModule() override;

	void start() override;
	void stop() override;

	unsigned long getTotalSecondsEdited() override;
	void setTotalSecondsEdited(unsigned long newValue) override;

	sigc::signal<void>& sig_TimerChanged() override;

private:
	bool applicationIsActive();

	void onMapEvent(IMap::MapEvent ev);
	void onIntervalReached();
	void readFromMapProperties();
	void writeToMapProperties(const scene::IMapRootNodePtr& root);
	void onResourceExporting(const scene::IMapRootNodePtr& root);
};

}

