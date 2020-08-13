#pragma once

#include "ieditstopwatch.h"
#include "imap.h"

#include <memory>
#include <wx/event.h>
#include <wx/timer.h>
#include <sigc++/connection.h>

namespace map
{

/**
* Stopwatch to measure the time spent editing a particular map. 
* The time is persisted to the .darkradiant file and keeps running
* as long as the application is in focus.
*/
class EditingStopwatch :
	public IMapEditStopwatch,
	public wxEvtHandler
{
private:
	sigc::connection _mapSignal;

	// Mapping timer in second resolution
	unsigned long _secondsEdited;

	// Helper object to get called per second
	std::unique_ptr<wxTimer> _timer;

	sigc::signal<void> _sigTimerChanged;

public:
	EditingStopwatch();

	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
	void shutdownModule() override;

	void start() override;
	void stop() override;

	unsigned long getTotalSecondsEdited() override;
	void setTotalSecondsEdited(unsigned long newValue) override;

	sigc::signal<void>& sig_TimerChanged() override;

private:
	void onMapEvent(IMap::MapEvent ev);
	void onRadiantStartup();
	void onIntervalReached(wxTimerEvent& ev);
	void readFromMapProperties();
	void writeToMapProperties(const scene::IMapRootNodePtr& root);
	void onResourceExporting(const scene::IMapRootNodePtr& root);
};

}

