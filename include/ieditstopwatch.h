#pragma once

#include "imodule.h"
#include <sigc++/signal.h>

namespace map
{

/**
* Stopwatch to measure the time spent editing a particular map.
* The time is persisted to the .darkradiant/.mapx file and keeps running
* as long as the application is in focus.
*
* The class will maintain its own internal timer class, but will broadcast
* a query message (type is IMessage::ApplicationIsActiveQuery) over the 
* MessageBus such that the UI module can react and prevent the timer 
* from increasing when the application is not in focus or blocked 
* in some other way.
*/
class IMapEditStopwatch :
	public RegisterableModule
{
public:
	virtual ~IMapEditStopwatch() {}

	// Starts the stopwatch, the map edit timer will start ticking (again)
	// This does not reset the timings
	virtual void start() = 0;

	// Stops the stopwatch, the map edit timer will not be increased anymore
	// This does not reset the timings
	virtual void stop() = 0;

	// Gets the total number of seconds the map has been edited
	virtual unsigned long getTotalSecondsEdited() = 0;

	// Sets the total number of seconds the map has been edited so far
	// This is used when loading maps from disk, to restore the timer to its previous state
	virtual void setTotalSecondsEdited(unsigned long newValue) = 0;

	// Signal emitted when the timer value changes
	virtual sigc::signal<void>& sig_TimerChanged() = 0;
};

}

const char* const MODULE_EDITING_STOPWATCH("EditingStopwatch");

inline map::IMapEditStopwatch& GlobalMapEditStopwatch()
{
	// Cache the reference locally
	static map::IMapEditStopwatch& _instance(
		*std::static_pointer_cast<map::IMapEditStopwatch>(
			module::GlobalModuleRegistry().getModule(MODULE_EDITING_STOPWATCH)
		)
	);
	return _instance;
}
