#pragma once

#include <map>
#include <list>
#include "imodule.h"

#include "ModuleLoader.h"

namespace module 
{

/** greebo: This is the actual implementation of the ModuleRegistry as defined in imodule.h.
 *          See the imodule.h file for a detailed documentation of the public methods.
 */
class ModuleRegistry :
	public IModuleRegistry
{
private:
    typedef std::map<std::string, RegisterableModulePtr> ModulesMap;

	// This is where the uninitialised modules go after registration
	ModulesMap _uninitialisedModules;

	// After initialisiation, modules get enlisted here.
	ModulesMap _initialisedModules;

	// Set to TRUE as soon as initialiseModules() is finished
	bool _modulesInitialised;

	// Set to TRUE after all modules have been shutdown
	bool _modulesShutdown;

    // Pointer to the application context
	ApplicationContext* _context;

	// For progress meter in the splash screen
	float _progress;

    // Signals fired after ALL modules have been initialised or shut down.
    sigc::signal<void> _sigAllModulesInitialised;
	sigc::signal<void> _sigAllModulesUninitialised;
	ProgressSignal _sigModuleInitialisationProgress;

	// Dynamic library loader
	ModuleLoader _loader;

	// Private constructor
	ModuleRegistry();

public:
    ~ModuleRegistry();

	// Registers the given module
    void registerModule(const RegisterableModulePtr& module) override;

	// Initialise all registered modules
    void loadAndInitialiseModules() override;

	// Shutdown all modules
    void shutdownModules() override;

	// Get the module
    RegisterableModulePtr getModule(const std::string& name) const override;

    // Returns TRUE if the named module exists in the records
    bool moduleExists(const std::string& name) const override;

	// Get the application context info structure
    const ApplicationContext& getApplicationContext() const override;

    sigc::signal<void> signal_allModulesInitialised() const override;
	ProgressSignal signal_moduleInitialisationProgress() const override;
    sigc::signal<void> signal_allModulesUninitialised() const override;

	std::size_t getCompatibilityLevel() const override;

    /// Invoked by RadiantApp to set the application context
	void setContext(ApplicationContext& context)
    {
        _context = &context;
    }

	// Contains the singleton instance
	static ModuleRegistry& Instance();

	// Returns a list of modules
	std::string getModuleList(const std::string& separator = "\n");

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
