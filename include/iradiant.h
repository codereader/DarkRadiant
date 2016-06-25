#pragma once

#include "imodule.h"

#include <sigc++/signal.h>
#include <functional>

const std::string MODULE_RADIANT("Radiant");

class ThreadManager;

// Interface to provide feedback during running operations
// see IRadiant::performLongRunningOperation()
class ILongRunningOperation
{
public:
	virtual ~ILongRunningOperation() {}

	// Update the operation progress fraction - range [0..1]
	virtual void setProgress(float progress) = 0;

	// Set the message that is displayed to the user
	virtual void setMessage(const std::string& message) = 0;
};

/**
 * \brief
 * Interface to the core application.
 */
class IRadiant :
	public RegisterableModule
{
public:

    /// Signal emitted when main Radiant application is constructed
    virtual sigc::signal<void> signal_radiantStarted() const = 0;

    /// Signal emitted just before Radiant shuts down
    virtual sigc::signal<void> signal_radiantShutdown() const = 0;

    enum MapEvent
    {
        MapLoading,     // emitted just before a map is starting to be loaded
        MapLoaded,      // emitted when the current map is done loading
        MapUnloading,   // emitted just before a map is unloaded from memory
        MapUnloaded,    // emitted after a map has been unloaded
    };

    typedef sigc::signal<void, MapEvent> MapEventSignal;

    /// Returns the signal that is emitted on various events
    virtual MapEventSignal signal_mapEvent() const = 0;
    
    /// Get the threading manager
    virtual ThreadManager& getThreadManager() = 0;

	// Runs a long running operation that should block input on all windows
	// until it completes. The operation functor needs to take a reference to
	// an operation object which can be used to give feedback like progress or
	// text messages that might be displayed to the user.
	virtual void performLongRunningOperation(
		const std::function<void(ILongRunningOperation&)>& operationFunc,
		const std::string& title = std::string()) = 0;
};

inline IRadiant& GlobalRadiant()
{
	// Cache the reference locally
	static IRadiant& _radiant(
		*std::static_pointer_cast<IRadiant>(
			module::GlobalModuleRegistry().getModule(MODULE_RADIANT)
		)
	);
	return _radiant;
}
