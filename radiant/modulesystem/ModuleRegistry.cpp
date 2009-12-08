#include "ModuleRegistry.h"

#include "itextstream.h"
#include <stdexcept>
#include <iostream>
#include "ApplicationContextImpl.h"
#include "ModuleLoader.h"

#include "ui/splash/Splash.h"

namespace module {

namespace {

	// Stream insertion operator for the set of dependencies
	std::ostream& operator<<(std::ostream& st, const StringSet& set) {
		st << "(";
		
		std::string output("");
		
		for (StringSet::const_iterator i = set.begin(); i != set.end(); i++) {
			output += (!output.empty()) ? ", " : ""; // delimiter
			output += *i;
		}
		
		st << output;
		
		st << ")";
		return st;
	}

}

ModuleRegistry::ModuleRegistry() :
	_modulesInitialised(false),
	_modulesShutdown(false)
{
	globalOutputStream() << "ModuleRegistry instantiated.\n";
}

void ModuleRegistry::unloadModules() {
	_uninitialisedModules.clear();
	_initialisedModules.clear();
	
	// greebo: Unloading the modules before the main binary 
	// shut down caused crashes > disabled this.
	//Loader::unloadModules();
}

void ModuleRegistry::registerModule(RegisterableModulePtr module) {
	assert(module); // don't take NULL module pointers
	
	if (_modulesInitialised) {
		// The train has left, this module is registered too late
		throw std::logic_error(
			"ModuleRegistry: module " + module->getName() + 
			" registered after initialisation."
		);
	}
	
	// Add this module to the list of uninitialised ones 
	std::pair<ModulesMap::iterator, bool> result = _uninitialisedModules.insert(
		ModulesMap::value_type(module->getName(), module)
	);

	// Don't allow modules with the same name being added twice 
	if (!result.second) {
		throw std::logic_error(
			"ModuleRegistry: multiple modules named " + module->getName()
		);
	}
	
	globalOutputStream() << "Module registered: " << module->getName().c_str() << "\n";
}

// Initialise the module (including dependencies, if necessary)
void ModuleRegistry::initialiseModuleRecursive(const std::string& name) 
{
	// Check if the module exists at all
	if (_uninitialisedModules.find(name) == _uninitialisedModules.end()) {
		throw std::logic_error(
			"ModuleRegistry: Module doesn't exist: " + name + "\n"
		);
	}
	
	// Check if the module is already initialised
	if (_initialisedModules.find(name) != _initialisedModules.end()) {
		//std::cout << "Module " << name << " already initialised.\n";
		return;
	}
	
	// Tag this module as "ready" by inserting it into the initialised list.
	_initialisedModules.insert(
		ModulesMap::value_type(name, _uninitialisedModules[name])
	);
	
	globalOutputStream() << "ModuleRegistry: "
                         << "preparing to initialise module: " 
                         << name << std::endl;
	
	// Create a shortcut to the module
	RegisterableModulePtr module = _uninitialisedModules[name];
	const StringSet& dependencies = module->getDependencies();

    // Debug builds should ensure that the dependencies don't reference the
    // module itself directly 
    assert(dependencies.find(name) == dependencies.end());
	
	// Initialise the dependencies first
	for (StringSet::const_iterator i = dependencies.begin(); 
		 i != dependencies.end(); i++)
	{
        globalOutputStream() << "   " << name << " needs dependency " 
                             << *i << std::endl;
		initialiseModuleRecursive(*i);
	}

	_progress = 0.1f + (static_cast<float>(_initialisedModules.size())/_uninitialisedModules.size())*0.65f;
	
	ui::Splash::Instance().setProgressAndText("Initialising Module: " + name, _progress);

    globalOutputStream() << "ModuleRegistry: dependencies satisfied, "
                         << "invoking initialiser for " << name << std::endl;

	// Initialise the module itself, now that the dependencies are ready
	module->initialiseModule(_context);
	
	globalOutputStream() << "=> Module " << name << " initialised.\n";
}

// Initialise all registered modules
void ModuleRegistry::initialiseModules() {
	if (_modulesInitialised) {
		throw std::runtime_error("ModuleRegistry::initialiseModule called twice.\n");
	}

	_progress = 0.1f;
	ui::Splash::Instance().setProgressAndText("Initialising Modules", _progress);

	for (ModulesMap::iterator i = _uninitialisedModules.begin();
		 i != _uninitialisedModules.end(); i++)
	{
		// greebo: Dive into the recursion
		// (this will return immediately if the module is already initialised).
		initialiseModuleRecursive(i->first);
	}
	
	// Make sure this isn't called again
	_modulesInitialised = true;
}

void ModuleRegistry::shutdownModules() {
	if (_modulesShutdown) {
		throw std::logic_error("ModuleRegistry: shutdownModules called twice.");
	}
	
	for (ModulesMap::iterator i = _initialisedModules.begin();
		 i != _initialisedModules.end(); i++)
	{
		//std::cout << "Shutting down module: " << i->first << "\n";
		i->second->shutdownModule();
	}
	
	// Free all the shared ptrs
	unloadModules();

	_modulesShutdown = true;
}

bool ModuleRegistry::moduleExists(const std::string& name) const {
	// Try to find the initialised module, uninitialised don't count as existing
	ModulesMap::const_iterator found = _initialisedModules.find(name);
	return (found != _initialisedModules.end());
}

// Get the module
RegisterableModulePtr ModuleRegistry::getModule(const std::string& name) const {
	
	// The return value (NULL) by default
	RegisterableModulePtr returnValue;
	
	// Try to find the module
	ModulesMap::const_iterator found = _initialisedModules.find(name);
	if (found != _initialisedModules.end()) {
		returnValue = found->second;
	}
	
	if (returnValue == NULL) {
		std::cerr << "ModuleRegistry: Warning! Module with name " 
		          << name << " requested but not found!\n";
	}
	
	return returnValue;
}

const ApplicationContext& ModuleRegistry::getApplicationContext() const {
	return _context;
}

void ModuleRegistry::initialiseContext(int argc, char* argv[]) {
	_context.initialise(argc, argv);
}

void ModuleRegistry::initErrorHandler()
{
	_context.initErrorHandler();
}

std::string ModuleRegistry::getModuleList(const std::string& separator) {
	std::string returnValue;
	
	for (ModulesMap::const_iterator i = _initialisedModules.begin(); 
		 i != _initialisedModules.end(); i++)
	{
		returnValue += (returnValue.empty()) ? "" : separator;
		returnValue += i->first;
	}
	
	return returnValue;
}

ModuleRegistry& ModuleRegistry::Instance() {
	static ModuleRegistry _registry;
	return _registry;
}

IModuleRegistry& getRegistry() {
	// Retrieve the singleton instance and deliver it
	return ModuleRegistry::Instance();
}

} // namespace module
