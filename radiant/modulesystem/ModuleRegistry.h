#ifndef MODULEREGISTRY_H_
#define MODULEREGISTRY_H_

#include "ApplicationContextImpl.h"

#include <map>
#include <list>
#include "imodule.h"

namespace module {

/** greebo: This is the actual implementation of the ModuleRegistry as defined in imodule.h.
 *          See the imodule.h file for a detailed documentation of the public methods.
 */
class ModuleRegistry :
	public IModuleRegistry
{
	typedef std::map<std::string, RegisterableModulePtr> ModulesMap;
	
	// This is where the uninitialised modules go after registration 
	ModulesMap _uninitialisedModules;
	
	// After initialisiation, modules get enlisted here.
	ModulesMap _initialisedModules;
		
	// Set to TRUE as soon as initialiseModules() is finished
	bool _modulesInitialised;
	
	// Set to TRUE after all modules have been shutdown
	bool _modulesShutdown;
	
	// Application context
	ApplicationContextImpl _context;

	// For progress meter in the splash screen
	float _progress;
	
	// Private constructor
	ModuleRegistry();
		
public:
	
	// Registers the given module
	virtual void registerModule(RegisterableModulePtr module);
	
	// Initialise all registered modules
	virtual void initialiseModules();
	
	// Shutdown all modules
	virtual void shutdownModules();
	
	// Get the module
	virtual RegisterableModulePtr getModule(const std::string& name) const;
	
	// Get the application context info structure
	virtual const ApplicationContext& getApplicationContext() const;
	
	// Called by main() to initialise the application/settings paths
	void initialiseContext(int argc, char* argv[]);

	// Points the ASSERT_MESSAGE function to our GTK popup handler
	void initErrorHandler();
	
	// Contains the singleton instance
	static ModuleRegistry& Instance();
	
	// Returns a list of modules
	std::string getModuleList(const std::string& separator = "\n");

	// Returns TRUE if the named module exists in the records
	bool moduleExists(const std::string& name) const;
	
private:

	// greebo: Frees all the allocated RegisterableModules. This MUST happen before
	// the main() routine has reached the end of scope, because on some
	// systems (Win32) the DLLs get unloaded before the static ModuleRegistry
	// is destructed - the shared_ptrs don't work anymore and are causing double-deletes.
	void unloadModules();
	
	// Initialises the module (including dependencies, recursively).
	void initialiseModuleRecursive(const std::string& name);

}; // class Registry

} // namespace module

#endif /*MODULEREGISTRY_H_*/
