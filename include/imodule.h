#pragma once

#include <memory>
#include <functional>
#include <cassert>
#include <sigc++/trackable.h>
#include <sigc++/signal.h>
#include <mutex>
#include <stdexcept>

#include <string>
#include <set>
#include <vector>

#include "itextstream.h"

/**
 * \defgroup module Module system
 */

/** greebo: These registry keys can be used application-wide during runtime
 *          to retrieve the various paths.
 */
const char* const RKEY_APP_PATH = "user/paths/appPath";
const char* const RKEY_HOME_PATH = "user/paths/homePath";
const char* const RKEY_SETTINGS_PATH = "user/paths/settingsPath";
const char* const RKEY_BITMAPS_PATH = "user/paths/bitmapsPath";
const char* const RKEY_MAP_PATH = "user/paths/mapPath";
const char* const RKEY_PREFAB_PATH = "user/paths/prefabPath";

/**
 * greebo: Compatibility level: a number inlined into all the modules and returned 
 * by their RegisterableModule::getCompatibilityLevel() method.
 *
 * This number should be changed each time the set of module/plugin files (.so/.dll/.dylib) 
 * is modified, especially when files are going to be removed from a DarkRadiant release.
 * The number will be checked by the ModuleRegistry against the internally stored one
 * to detect outdated binaries and reject their registration.
 *
 * As long as no external module/plugin files are removed this number is safe to stay 
 * as it is. Keep this number compatible to std::size_t, i.e. unsigned.
 */
#define MODULE_COMPATIBILITY_LEVEL 20180104

// A function taking an error title and an error message string, invoked in debug builds
// for things like ASSERT_MESSAGE and ERROR_MESSAGE
typedef std::function<void (const std::string&, const std::string&)> ErrorHandlingFunction;

// This method holds a function pointer which can do some error display (like popups)
// Each module binary has its own copy of this, it's initialised in performDefaultInitialisation()
inline ErrorHandlingFunction& GlobalErrorHandler()
{
	static ErrorHandlingFunction _func;
	return _func;
}

/**
 * Provider for various information that may be required by modules during
 * initialisation.
 *
 * \ingroup module
 */
class ApplicationContext
{
public:

    /**
     * \brief
     * Argument list type.
     */
    typedef std::vector<std::string> ArgumentList;

    /**
	 * Destructor
	 */
	virtual ~ApplicationContext() {}

	/**
	 * Return the application path of the current Radiant instance.
	 */
	virtual std::string getApplicationPath() const = 0;

    /**
     * Return the application library path, containing modules and plugins.
     *
     * On Windows this is most likely the same as the application path. On
     * Linux it might be a hard-coded path like /usr/lib/darkradiant, or a
     * relocatable relative path like ../lib
     */
    virtual std::string getLibraryPath() const = 0;

    /**
     * Return the toplevel path contaning runtime data files, such as the GL
     * programs or UI descriptor files. On Windows this is the same as the
     * application path, on Linux it is determined at compile-time but probably
     * something like /usr/share/darkradiant.
     */
    virtual std::string getRuntimeDataPath() const = 0;

	/**
	 * Return the settings path of the current Radiant instance.
	 */
	virtual std::string getSettingsPath() const = 0;

	/**
	 * Return the settings path of the current Radiant instance.
	 */
	virtual std::string getBitmapsPath() const = 0;

    /// Return the path to HTML documentation files
    virtual std::string getHTMLPath() const = 0;

	/**
     * \brief
     * Return the list of command line arguments.
	 */
	virtual const ArgumentList& getCmdLineArgs() const = 0;

	/**
	 * Return the reference to the application's output/error streams.
	 */
	virtual std::ostream& getOutputStream() const = 0;
	virtual std::ostream& getErrorStream() const = 0;
	virtual std::ostream& getWarningStream() const = 0;

    // Provides a single mutex object which should be locked by client code
    // before writing to the any of the above streams.
    virtual std::mutex& getStreamLock() const = 0;

	/**
	 * Sets up the paths and stores them into the registry.
	 */
	virtual void savePathsToRegistry() const = 0;

	/**
	 * Retrieve a function pointer which can handle assertions and runtime errors
	 */
	virtual const ErrorHandlingFunction& getErrorHandlingFunction() const = 0;
};

/**
 * Set of strings for module dependencies.
 */
typedef std::set<std::string> StringSet;

/**
 * Interface class for modules. Each RegisterableModule can return its name and
 * its set of dependencies.
 *
 * Note that this interface does NOT concern itself with the type (interface)
 * of each individual module; it is up to the GlobalBlah() accessor function
 * associated with each module to perform the required downcast to the known
 * type.
 *
 * All RegisterableModules implement sigc::trackable, since they will often want
 * to connect themselves to another module's signal(s).
 *
 * \ingroup module
 */
class RegisterableModule: public sigc::trackable
{
private:
	const std::size_t _compatibilityLevel;

public:
	RegisterableModule() :
		_compatibilityLevel(MODULE_COMPATIBILITY_LEVEL)
	{}

    /**
	 * Destructor
	 */
	virtual ~RegisterableModule() {}

	/**
	 * Return the name of this module. This must be globally unique across all
	 * modules; the modulesystem will throw a logic_error if two modules attempt
	 * to register themselves with the same name.
	 */
	virtual const std::string& getName() const = 0;

	/**
	 * Return the set of dependencies for this module. The return value is a
	 * set of strings, each containing the unique name (as returned by
	 * getName()) of a module which must be initialised before this one.
	 */
	virtual const StringSet& getDependencies() const = 0;

	/**
	 * Instruct this module to initialise itself. A RegisterableModule must NOT
	 * invoke any calls to other modules in its constructor, since at the point
	 * of construction the other modules will not have been initialised. Once
	 * all of the dependencies are processed by the ModuleRegistry, each module
	 * will have its initialiseModule() method called at the appropriate point.
	 *
	 * The ModuleRegistry guarantees that any modules named in the set of
	 * dependencies returned by getDependencies() will be initialised and ready
	 * for use at the time this method is invoked. Attempting to access a module
	 * which was NOT listed as a dependency is undefined.
	 *
	 * @param ctx
	 * The ApplicationContext of the running Radiant application.
	 */
	virtual void initialiseModule(const ApplicationContext& ctx) = 0;

	/**
	 * Optional shutdown routine. Allows the module to de-register itself,
	 * shutdown windows, save stuff into the Registry and so on.
	 *
	 * All the modules are getting called one by one, all other modules
	 * are available until the last shutdownModule() call was invoked.
	 */
	virtual void shutdownModule() {
		// Empty default implementation
	}

	// Internally queried by the ModuleRegistry. To protect against leftover
	// binaries containing outdated moudles from being loaded and registered
	// the compatibility level is compared with the one in the ModuleRegistry.
	// Old modules with mismatching numbers will be rejected.
	// Function is intentionally non-virtual and inlined.
	std::size_t getCompatibilityLevel() const
	{
		return _compatibilityLevel;
	}
};

/**
 * Shared pointer typdef.
 */
typedef std::shared_ptr<RegisterableModule> RegisterableModulePtr;

/**
 * Interface for the module registry. This is the owner and manager of all
 * RegisterableModules defined in DLLs and the main binary.
 *
 * For obvious reasons, the ModuleRegistry itself is not a module, but a static
 * object owned by the main binary and returned through a globally-accessible
 * method.
 *
 * \ingroup module
 */
class IModuleRegistry 
{
public:
    /**
	 * Destructor
	 */
	virtual ~IModuleRegistry() {}

	/**
	 * Register a RegisterableModule. The name and dependencies are retrieved
	 * through the appropriate RegisterableModule interface methods.
	 *
	 * This method does not cause the RegisterableModule to be initialised.
	 */
	virtual void registerModule(const RegisterableModulePtr& module) = 0;

	/**
	 * Initialise all of the modules previously registered with
	 * registerModule() in the order required by their dependencies. This method
	 * is invoked once, at application startup, with any subsequent attempts
	 * to invoke this method throwing a logic_error.
	 */
	virtual void loadAndInitialiseModules() = 0;

	/**
	 * All the RegisterableModule::shutdownModule() routines are getting
	 * called one by one. No modules are actually destroyed during the
	 * iteration is ongoing, so each module is guaranteed to exist.
	 *
	 * After the last shutdownModule() call has been invoked, the modules
	 * can be safely unloaded/destroyed.
	 */
	virtual void shutdownModules() = 0;

	/**
	 * Retrieve the module associated with the provided unique name. If the
	 * named module is not found, an empty RegisterableModulePtr is returned
	 * (this allows modules to be optional).
	 *
	 * Note that the return value of this function is RegisterableModulePtr,
	 * which in itself is useless to application code. It is up to the accessor
	 * functions defined in each module interface (e.g. GlobalEntityCreator())
	 * to downcast the pointer to the appropriate type.
	 */
	virtual RegisterableModulePtr getModule(const std::string& name) const = 0;

	/**
	 * Returns TRUE if the named module exists in the records.
	 */
	virtual bool moduleExists(const std::string& name) const = 0;

	/**
	 * This retrieves a reference to the information structure ApplicationContext,
	 * which holds the AppPath, SettingsPath and references to the application
	 * streams. The latter can be used by modules to initialise their
	 * rMessage/globalErrorStreams().
	 */
	virtual const ApplicationContext& getApplicationContext() const = 0;

    /**
     * Invoked when all modules have been initialised.
     */
    virtual sigc::signal<void> signal_allModulesInitialised() const = 0;

	/**
	 * Progress function called during module loading and intialisation.
	 * The string value will carry a message about what is currently in progress.
	 * The float value passed to the signal indicates the overall progress and
	 * will be in the range [0.0f..1.0f].
	 */
	typedef sigc::signal<void, const std::string&, float> ProgressSignal;
	virtual ProgressSignal signal_moduleInitialisationProgress() const = 0;

    /**
    * Invoked when all modules have been shut down (i.e. after shutdownModule()).
    */
    virtual sigc::signal<void> signal_allModulesUninitialised() const = 0;

	// The compatibility level this Registry instance was compiled against.
	// Old module registrations will be rejected by the registry anyway,
	// on top of that they can actively query this number from the registry
	// to check whether they are being loaded into an incompatible binary.
	virtual std::size_t getCompatibilityLevel() const = 0;
};

namespace module
{
	/**
	 * \namespace module
	 * Types and functions implementing the module registry system.
	 *
	 * \ingroup module
	 */

	/** greebo: This is a container holding a reference to the registry.
	 *          The getRegistry() symbol above is not exported to the
	 *          modules in Win32 compiles. That's why this structure
	 * 			has to be initialised in the RegisterModule() routine.
	 *
	 * As soon as it's initialised, the module can access the ModuleRegistry
	 * with the routine GlobalModuleRegistry() below.
	 */
	class RegistryReference {
		IModuleRegistry* _registry;
	public:
		RegistryReference() :
			_registry(NULL)
		{}

		inline void setRegistry(IModuleRegistry& registry) {
			_registry = &registry;
		}

		inline IModuleRegistry& getRegistry() {
			assert(_registry); // must not be NULL
			return *_registry;
		}

		static RegistryReference& Instance() {
			static RegistryReference _registryRef;
			return _registryRef;
		}
	};

	/**
	 * Global accessor method for the ModuleRegistry.
	 */
	inline IModuleRegistry& GlobalModuleRegistry() 
	{
		return RegistryReference::Instance().getRegistry();
	}

	// Exception thrown if the module being loaded is incompatible with the main binary
	class ModuleCompatibilityException : 
		public std::runtime_error
	{
	public:
		ModuleCompatibilityException(const std::string& msg) :
			std::runtime_error(msg)
		{}
	};

	// greebo: This should be called once by each module at load time to initialise
	// the OutputStreamHolders
	inline void initialiseStreams(const ApplicationContext& ctx)
	{
		GlobalOutputStream().setStream(ctx.getOutputStream());
		GlobalWarningStream().setStream(ctx.getWarningStream());
		GlobalErrorStream().setStream(ctx.getErrorStream());

#ifndef NDEBUG
		GlobalDebugStream().setStream(ctx.getOutputStream());
#endif

		// Set up the mutex for thread-safe logging
		GlobalOutputStream().setLock(ctx.getStreamLock());
		GlobalWarningStream().setLock(ctx.getStreamLock());
		GlobalErrorStream().setLock(ctx.getStreamLock());
		GlobalDebugStream().setLock(ctx.getStreamLock());
	}

	// Helper method initialising a few references and checking a module's
	// compatibility level with the one reported by the ModuleRegistry
	inline void performDefaultInitialisation(IModuleRegistry& registry)
	{
		if (registry.getCompatibilityLevel() != MODULE_COMPATIBILITY_LEVEL)
		{
			throw ModuleCompatibilityException("Compatibility level mismatch");
		}

		// Initialise the streams using the given application context
		initialiseStreams(registry.getApplicationContext());

		// Remember the reference to the ModuleRegistry
		RegistryReference::Instance().setRegistry(registry);

		// Set up the assertion handler
		GlobalErrorHandler() = registry.getApplicationContext().getErrorHandlingFunction();
	}
}

// Platform-specific definition which needs to be defined both
// in the plugins and the main binary.
#if defined(WIN32)
	#if defined(_MSC_VER)
		// In VC++ we use this to export symbols instead of using .def files
		// Note: don't use __stdcall since this is adding stack bytes to the function name
		#define DARKRADIANT_DLLEXPORT __declspec(dllexport)
	#else
		#define DARKRADIANT_DLLEXPORT 
	#endif
#elif defined(__APPLE__)
    #define DARKRADIANT_DLLEXPORT __attribute__((visibility("default")))
#else
	#define DARKRADIANT_DLLEXPORT
#endif
