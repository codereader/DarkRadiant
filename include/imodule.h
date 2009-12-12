#ifndef IMODULE_H_
#define IMODULE_H_

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include <string>
#include <set>
#include <vector>

/**
 * \defgroup module Module system
 */

/** greebo: These registry keys can be used application-wide during runtime
 *          to retrieve the various paths.
 */
namespace {
	const std::string RKEY_APP_PATH = "user/paths/appPath";
	const std::string RKEY_HOME_PATH = "user/paths/homePath";
	const std::string RKEY_SETTINGS_PATH = "user/paths/settingsPath";
	const std::string RKEY_BITMAPS_PATH = "user/paths/bitmapsPath";
	const std::string RKEY_ENGINE_PATH = "user/paths/enginePath";
	const std::string RKEY_MAP_PATH = "user/paths/mapPath";
	const std::string RKEY_PREFAB_PATH = "user/paths/prefabPath";
}

// A function taking an error title and an error message string, invoked in debug builds
// for things like ASSERT_MESSAGE and ERROR_MESSAGE
typedef boost::function<void (const std::string&, const std::string&)> ErrorHandlingFunction;

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
	virtual const std::string& getApplicationPath() const = 0;
	
	/**
	 * Return the settings path of the current Radiant instance.
	 */
	virtual const std::string& getSettingsPath() const = 0;
	
	/**
	 * Return the settings path of the current Radiant instance.
	 */
	virtual const std::string& getBitmapsPath() const = 0;

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
 * \ingroup module
 */
class RegisterableModule {
public:
	
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
};

/**
 * Shared pointer typdef.
 */
typedef boost::shared_ptr<RegisterableModule> RegisterableModulePtr;

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
class IModuleRegistry {
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
	virtual void registerModule(RegisterableModulePtr module) = 0;
	
	/**
	 * Initialise all of the modules previously registered with 
	 * registerModule() in the order required by their dependencies. This method
	 * is invoked once, at application startup, with any subsequent attempts
	 * to invoke this method throwing a logic_error.
	 */
	virtual void initialiseModules() = 0;
	
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
	 * globalOutputStream/globalErrorStreams().
	 */
	virtual const ApplicationContext& getApplicationContext() const = 0;
	
};

namespace module {

	/**
	 * \namespace module
	 * Types and functions implementing the module registry system.
	 * 
	 * \ingroup module
	 */

	/**
	 * Global ModuleRegistry accessor function exported by the main binary.
	 * Note: Don't use this from within modules (entity, shaders, etc.),
	 * this only works from within the DarkRadiant core binary (Win32).
	 */
	IModuleRegistry& getRegistry();
	
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
	inline IModuleRegistry& GlobalModuleRegistry() {
		return RegistryReference::Instance().getRegistry();
	}
}

// Platform-specific definition which needs to be defined both 
// in the plugins and the main binary.
#if defined(WIN32)
	#define DARKRADIANT_DLLEXPORT __stdcall
#else
	#define DARKRADIANT_DLLEXPORT
#endif

#endif /*IMODULE_H_*/
