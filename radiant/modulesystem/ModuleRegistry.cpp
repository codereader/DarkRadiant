#include "ModuleRegistry.h"

#include "ui/splash/Splash.h"

#include "i18n.h"
#include "itextstream.h"
#include <stdexcept>
#include <iostream>
#include "ApplicationContextImpl.h"
#include "ModuleLoader.h"

#include <wx/app.h>
#include <boost/format.hpp>

namespace module
{

ModuleRegistry::ModuleRegistry() :
	_modulesInitialised(false),
	_modulesShutdown(false),
    _context(nullptr)
{
	rMessage() << "ModuleRegistry instantiated." << std::endl;
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
	_initialisedModules.clear();

    // We need to delete all pending objects before unloading modules
    // wxWidgets needs a chance to delete them before memory access is denied
    if (wxTheApp != NULL)
    {
        wxTheApp->ProcessIdle();
    }

	Loader::unloadModules();
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

	ui::Splash::Instance().setProgressAndText(
		(boost::format(_("Initialising Module: %s")) % name).str(),
		_progress);

	// Initialise the module itself, now that the dependencies are ready
    wxASSERT(_context);
	module->initialiseModule(*_context);
}

// Initialise all registered modules
void ModuleRegistry::initialiseModules()
{
	if (_modulesInitialised)
    {
		throw std::runtime_error("ModuleRegistry::initialiseModule called twice.");
	}

	_progress = 0.1f;
	ui::Splash::Instance().setProgressAndText(_("Initialising Modules"), _progress);

	for (ModulesMap::iterator i = _uninitialisedModules.begin();
		 i != _uninitialisedModules.end(); ++i)
	{
		// greebo: Dive into the recursion
		// (this will return immediately if the module is already initialised).
		initialiseModuleRecursive(i->first);
	}

	// Make sure this isn't called again
	_modulesInitialised = true;

    // Fire the signal now
    _sigAllModulesInitialised.emit();

    _progress = 1.0f;
    ui::Splash::Instance().setProgressAndText(_("Modules initialised"), _progress);
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

sigc::signal<void> ModuleRegistry::signal_allModulesUninitialised() const
{
    return _sigAllModulesUninitialised;
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
