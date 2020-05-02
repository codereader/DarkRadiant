#pragma once

#include "imodule.h"

#include <sigc++/signal.h>
#include <functional>

const char* const MODULE_RADIANT_APP("Radiant");
const char* const MODULE_RADIANT_CORE("RadiantCore");

namespace applog { class ILogWriter;  }

namespace radiant
{

class IMessageBus; // see imessagebus.h

/**
 * Main application host, offering access to the Module Registry
 * and the logging infrastructure.
 *
 * As it implements the RegisterableModule interface, an instance
 * can be acquired through the GlobalModuleRegistry at runtime.
 * Module Registration doesn't need to be performed explicitly,
 * the IRadiant module will register itself.
 */
class IRadiant : 
    public RegisterableModule
{
public:
    typedef std::shared_ptr<IRadiant> Ptr;

    /**
     * Central logging class of the module. Use this to
     * attach your own ILogDevices to receive logging output.
     */
    virtual applog::ILogWriter& getLogWriter() = 0;

    /**
     * Returns the central module registry instance.
     */
    virtual IModuleRegistry& getModuleRegistry() = 0;

    /**
     * Get a reference to the central message handler.
     */
    virtual IMessageBus& getMessageBus() = 0;

    /**
     * Loads and initialises all modules, starting up the 
     * application.
     */
    virtual void startup() = 0;

    virtual ~IRadiant() {}
};

}

/**
 * \brief
 * Interface to the radiant application.
 */
class IRadiantBase :
	public RegisterableModule
{
public:

    /// Signal emitted when main Radiant application is constructed
    virtual sigc::signal<void> signal_radiantStarted() const = 0;

    /// Signal emitted just before Radiant shuts down
    virtual sigc::signal<void> signal_radiantShutdown() const = 0;
};

inline radiant::IRadiant& GlobalRadiantCore()
{
    // Cache the reference locally
    static radiant::IRadiant& _radiant(
        *std::static_pointer_cast<radiant::IRadiant>(
            module::GlobalModuleRegistry().getModule(MODULE_RADIANT_CORE)
        )
    );
    return _radiant;
}

inline IRadiantBase& GlobalRadiant()
{
	// Cache the reference locally
	static IRadiantBase& _radiant(
		*std::static_pointer_cast<IRadiantBase>(
			module::GlobalModuleRegistry().getModule(MODULE_RADIANT_APP)
		)
	);
	return _radiant;
}
