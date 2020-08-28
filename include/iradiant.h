#pragma once

#include "imodule.h"

#include <sigc++/signal.h>
#include <functional>

const char* const MODULE_RADIANT_CORE("RadiantCore");

namespace applog { class ILogWriter;  }
namespace language { class ILanguageManager; } // see "i18n.h"

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
     * Get a reference to the Language Manager instance
     * used to resolve localised string resources.
     */
    virtual language::ILanguageManager& getLanguageManager() = 0;

    /**
     * Loads and initialises all modules, starting up the 
     * application. Might throw a StartupFailure exception
     * on unrecoverable errors.
     */
    virtual void startup() = 0;

    virtual ~IRadiant() {}

    // Exception thrown during Radiant startup
    class StartupFailure :
        public std::runtime_error
    {
    public:
        StartupFailure(const std::string& msg) :
            runtime_error(msg)
        {}
    };
};

}

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
