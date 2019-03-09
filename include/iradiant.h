#pragma once

#include "imodule.h"

#include <sigc++/signal.h>
#include <functional>

const std::string MODULE_RADIANT("Radiant");

class ThreadManager;

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

    /// Get the threading manager
    virtual ThreadManager& getThreadManager() = 0;
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
