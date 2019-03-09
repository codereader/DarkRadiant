#include "ModuleRegistry.h"

#include "i18n.h"
#include "itextstream.h"
#include <stdexcept>
#include <iostream>
#include "ApplicationContextImpl.h"

#include <wx/app.h>
#include <fmt/format.h>

namespace module
{

ModuleRegistry::ModuleRegistry() :
	_modulesInitialised(false),
	_modulesShutdown(false),
    _context(nullptr)
{
	rMessage() << "ModuleRegistry instantiated." << std::endl;

    // Initialise the Reference in the GlobalModuleRegistry() accessor.
    RegistryReference::Instance().setRegistry(*this);
}

ModuleRegistry::~ModuleRegistry()
{
    // The modules map might be non-empty if the app is failing during very
    // early startup stages, and unloadModules() might not have been called yet.
    // Some modules might need to call this instance during their own destruction,
    // so it's better not to rely on the shared_ptr to destruct them.
    unloadModules();
}

void ModuleRegistry::unloadModules()
{
	_uninitialisedModules.clear();
    
    // greebo: It's entirely possible that the clear() method will clear the
    // last shared_ptr of a module. Module might still call this class' moduleExists()
    // method which in turn refers to a semi-destructed ModulesMap instance.
    // So, copy the contents to a temporary map before clearing it out.
    ModulesMap tempMap;
    tempMap.swap(_initialisedModules);
    
	tempMap.clear();

    // We need to delete all pending objects before unloading modules
    // wxWidgets needs a chance to delete them before memory access is denied
    if (wxTheApp != NULL)
    {
        wxTheApp->ProcessIdle();
    }

	_loader.unloadModules();
}

void ModuleRegistry::registerModule(const RegisterableModulePtr& module)
{
	assert(module); // don't take NULL module pointers

	if (_modulesInitialised)
	{
		// The train has left, this module is registered too late
		throw std::logic_error(
			"ModuleRegistry: module " + module->getName() +
			" registered after initialisation."
		);
	}

	// Check the compatibility level of this module against our internal one
	if (module->getCompatibilityLevel() != getCompatibilityLevel())
	{
		rError() << "ModuleRegistry: Incompatible module rejected: " << module->getName() << 
			" (module level: " << module->getCompatibilityLevel() << ", registry level: " << 
			getCompatibilityLevel() << ")" << std::endl;
		return;
	}

	// Add this module to the list of uninitialised ones
	std::pair<ModulesMap::iterator, bool> result = _uninitialisedModules.insert(
		ModulesMap::value_type(module->getName(), module)
	);

	// Don't allow modules with the same name being added twice
	if (!result.second)
    {
		throw std::logic_error(
			"ModuleRegistry: multiple modules named " + module->getName()
		);
	}

	rMessage() << "Module registered: " << module->getName() << std::endl;
}

// Initialise the module (including dependencies, if necessary)
void ModuleRegistry::initialiseModuleRecursive(const std::string& name)
{
	// Check if the module exists at all
	if (_uninitialisedModules.find(name) == _uninitialisedModules.end())
    {
		throw std::logic_error("ModuleRegistry: Module doesn't exist: " + name);
	}

	// Check if the module is already initialised
	if (_initialisedModules.find(name) != _initialisedModules.end())
    {
		return;
	}

	// Tag this module as "ready" by inserting it into the initialised list.
	_initialisedModules.insert(ModulesMap::value_type(name, _uninitialisedModules[name]));

	// Create a shortcut to the module
	RegisterableModulePtr module = _uninitialisedModules[name];
	const StringSet& dependencies = module->getDependencies();

    // Debug builds should ensure that the dependencies don't reference the
    // module itself directly
    assert(dependencies.find(name) == dependencies.end());

	// Initialise the dependencies first
	for (const std::string& namedDependency : dependencies)
	{
        initialiseModuleRecursive(namedDependency);
	}

	_progress = 0.1f + (static_cast<float>(_initialisedModules.size())/_uninitialisedModules.size())*0.9f;

	_sigModuleInitialisationProgress.emit(
		fmt::format(_("Initialising Module: {0}"), name),
		_progress);

	// Initialise the module itself, now that the dependencies are ready
    wxASSERT(_context);
	module->initialiseModule(*_context);
}

// Initialise all registered modules
void ModuleRegistry::loadAndInitialiseModules()
{
	if (_modulesInitialised)
    {
		throw std::runtime_error("ModuleRegistry::initialiseModule called twice.");
	}

	_sigModuleInitialisationProgress.emit(_("Searching for Modules"), 0.0f);

	rMessage() << "ModuleRegistry Compatibility Level is " << getCompatibilityLevel() << std::endl;

	// Invoke the ModuleLoad routine to load the DLLs from modules/ and plugins/
	_loader.loadModules(_context->getLibraryPath());

	_progress = 0.1f;
	_sigModuleInitialisationProgress.emit(_("Initialising Modules"), _progress);

	for (ModulesMap::iterator i = _uninitialisedModules.begin();
		 i != _uninitialisedModules.end(); ++i)
	{
		// greebo: Dive into the recursion
		// (this will return immediately if the module is already initialised).
		initialiseModuleRecursive(i->first);
	}

	// Make sure this isn't called again
	_modulesInitialised = true;

    _progress = 1.0f;
	_sigModuleInitialisationProgress.emit(_("Modules initialised"), _progress);

	// Fire the signal now, this will destroy the Splash dialog as well
	_sigAllModulesInitialised.emit();
}

void ModuleRegistry::shutdownModules()
{
	if (_modulesShutdown)
    {
		throw std::logic_error("ModuleRegistry: shutdownModules called twice.");
	}

	for (ModulesMap::value_type& pair : _initialisedModules)
	{
		pair.second->shutdownModule();
	}

    // Fire the signal before unloading the modules
    _sigAllModulesUninitialised.emit();

	// Free all the shared ptrs
	unloadModules();

	_modulesShutdown = true;
}

bool ModuleRegistry::moduleExists(const std::string& name) const
{
	// Try to find the initialised module, uninitialised don't count as existing
    return _initialisedModules.find(name) != _initialisedModules.end();
}

// Get the module
RegisterableModulePtr ModuleRegistry::getModule(const std::string& name) const {

	// The return value (NULL) by default
	RegisterableModulePtr returnValue;

	// Try to find the module
	ModulesMap::const_iterator found = _initialisedModules.find(name);

	if (found != _initialisedModules.end())
    {
		returnValue = found->second;
	}

	if (!returnValue)
    {
        rConsoleError() << "ModuleRegistry: Warning! Module with name "
		          << name << " requested but not found!" << std::endl;
	}

	return returnValue;
}

const ApplicationContext& ModuleRegistry::getApplicationContext() const
{
    wxASSERT(_context);
	return *_context;
}

sigc::signal<void> ModuleRegistry::signal_allModulesInitialised() const
{
    return _sigAllModulesInitialised;
}

ModuleRegistry::ProgressSignal ModuleRegistry::signal_moduleInitialisationProgress() const
{
	return _sigModuleInitialisationProgress;
}

sigc::signal<void> ModuleRegistry::signal_allModulesUninitialised() const
{
    return _sigAllModulesUninitialised;
}

std::size_t ModuleRegistry::getCompatibilityLevel() const
{
	return MODULE_COMPATIBILITY_LEVEL;
}

std::string ModuleRegistry::getModuleList(const std::string& separator)
{
	std::string returnValue;

	for (ModulesMap::value_type& pair : _initialisedModules)
	{
		returnValue += (returnValue.empty()) ? "" : separator;
		returnValue += pair.first;
	}

	return returnValue;
}

ModuleRegistry& ModuleRegistry::Instance()
{
	static ModuleRegistry _registry;
	return _registry;
}

IModuleRegistry& getRegistry()
{
	// Retrieve the singleton instance and deliver it
	return ModuleRegistry::Instance();
}

} // namespace module
