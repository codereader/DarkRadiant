#pragma once

#include "imodule.h"

#include <sigc++/signal.h>
#include <functional>

const char* const MODULE_RADIANT("Radiant");

namespace applog { class ILogWriter;  }

namespace radiant
{

/**
 * Main application host
 */
class IRadiant
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

    virtual ~IRadiant() {}
};

}

/**
 * \brief
 * Interface to the core application.
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

inline IRadiantBase& GlobalRadiant()
{
	// Cache the reference locally
	static IRadiantBase& _radiant(
		*std::static_pointer_cast<IRadiantBase>(
			module::GlobalModuleRegistry().getModule(MODULE_RADIANT)
		)
	);
	return _radiant;
}
