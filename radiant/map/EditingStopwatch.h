#pragma once

#include "imodule.h"
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
	public RegisterableModule,
	public wxEvtHandler
{
private:
	sigc::connection _mapSignal;

	// Mapping timer in second resolution
	unsigned long _secondsEdited;

	// Helper object to get called per second
	std::unique_ptr<wxTimer> _timer;

public:
	EditingStopwatch();

	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
	void shutdownModule() override;

	void start();
	void stop();

	unsigned long getTotalSecondsEdited();
	void setTotalSecondsEdited(unsigned long newValue);

	// Internal accessor to the module
	static EditingStopwatch& GetInstanceInternal();

private:
	void onMapEvent(IMap::MapEvent ev);
	void onRadiantStartup();
	void onIntervalReached(wxTimerEvent& ev);
};

}

