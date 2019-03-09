#include "RadiantModule.h"
#include "RadiantThreadManager.h"

#include <iostream>
#include <ctime>

#include "iregistry.h"
#include "icommandsystem.h"
#include "itextstream.h"
#include "iuimanager.h"
#include "ieventmanager.h"
#include "i18n.h"
#include "imainframe.h"

#include "scene/Node.h"

#include "ui/mainframe/ScreenUpdateBlocker.h"
#include "ui/mru/MRU.h"

#include "modulesystem/StaticModule.h"
#include "selection/algorithm/General.h"
#include "brush/csg/CSG.h"

#include "ui/modelselector/ModelSelector.h"
#include "EventRateLimiter.h"
#include "selection/shaderclipboard/ShaderClipboard.h"

#include <wx/app.h>

namespace radiant
{

sigc::signal<void> RadiantModule::signal_radiantStarted() const
{
    return _radiantStarted;
}

sigc::signal<void> RadiantModule::signal_radiantShutdown() const
{
    return _radiantShutdown;
}

ThreadManager& RadiantModule::getThreadManager()
{
    if (!_threadManager)
    {
        _threadManager.reset(new RadiantThreadManager);
    }
    return *_threadManager;
}

void RadiantModule::broadcastShutdownEvent()
{
	_threadManager.reset();

    _radiantShutdown.emit();
    _radiantShutdown.clear();
}

// Broadcasts a "startup" event to all the listeners
void RadiantModule::broadcastStartupEvent()
{
    _radiantStarted.emit();
}

// RegisterableModule implementation
const std::string& RadiantModule::getName() const
{
	static std::string _name(MODULE_RADIANT);
	return _name;
}

const StringSet& RadiantModule::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
    {
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_EVENTMANAGER);
	}

	return _dependencies;
}

void RadiantModule::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	// Reset the node id count
  	scene::Node::resetIds();

    selection::algorithm::registerCommands();
	brush::algorithm::registerCommands();

    // Subscribe for the post-module init event
    module::GlobalModuleRegistry().signal_allModulesInitialised().connect(
        sigc::mem_fun(this, &RadiantModule::postModuleInitialisation));
}

void RadiantModule::shutdownModule()
{
	rMessage() << getName() << "::shutdownModule called." << std::endl;

	_radiantStarted.clear();
    _radiantShutdown.clear();
}

void RadiantModule::postModuleInitialisation()
{
	// Construct the MRU commands and menu structure, load the recently used files
	GlobalMRU().initialise();

    // Initialise the mainframe
    GlobalMainFrame().construct();

	// Initialise the shaderclipboard
	GlobalShaderClipboard().clear();

	// Broadcast the startup event
    broadcastStartupEvent();

    // Load the shortcuts from the registry
    GlobalEventManager().loadAccelerators();

    // Pre-load models
    ui::ModelSelector::Populate();

	// Show the top level window as late as possible
	GlobalMainFrame().getWxTopLevelWindow()->Show();

    time_t localtime;
    time(&localtime);
    rMessage() << "Startup complete at " << ctime(&localtime) << std::endl;
}

// Define the static Radiant module
module::StaticModule<RadiantModule> radiantCoreModule;

// Return the static Radiant module to other code within the main binary
RadiantModulePtr getGlobalRadiant()
{
	return radiantCoreModule.getModule();
}

} // namespace radiant

