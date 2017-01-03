#include "RadiantModule.h"
#include "RadiantThreadManager.h"

#include <iostream>
#include <ctime>

#include "ifiletypes.h"
#include "iregistry.h"
#include "icommandsystem.h"
#include "itextstream.h"
#include "iuimanager.h"
#include "ieclass.h"
#include "ipreferencesystem.h"
#include "ieventmanager.h"
#include "iclipper.h"
#include "i18n.h"
#include "imainframe.h"

#include "scene/Node.h"

#include "ui/texturebrowser/TextureBrowser.h"
#include "ui/mainframe/ScreenUpdateBlocker.h"
#include "ui/mru/MRU.h"
#include "map/Map.h"
#include "brush/csg/CSG.h"

#include "modulesystem/StaticModule.h"
#include "selection/algorithm/General.h"

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

namespace
{

class LongRunningOperation :
	public ILongRunningOperation,
	private ui::ScreenUpdateBlocker
{
private:
	bool _messageChanged;
	std::string _message;

public:
	LongRunningOperation(const std::string& title, const std::string& message) :
		ScreenUpdateBlocker(title, message),
		_messageChanged(true),
		_message(message)
	{}

	void pulse()
	{
		ScreenUpdateBlocker::pulse();
	}

	void setProgress(float progress)
	{
		ScreenUpdateBlocker::setProgress(progress);
	}

	// Set the message that is displayed to the user
	void setMessage(const std::string& message)
	{
		_message = message;
		_messageChanged = true;
	}

	bool messageChanged()
	{
		return _messageChanged;
	}

	void dispatch()
	{
		_messageChanged = false;

		ScreenUpdateBlocker::setMessage(_message);
	}
};

}

void RadiantModule::performLongRunningOperation(
	const std::function<void(ILongRunningOperation&)>& operationFunc,
	const std::string& message)
{
	LongRunningOperation operation(_("Processing..."), message);

	std::size_t threadId = getThreadManager().execute([&]()
	{
		operationFunc(operation);
	});

	while (getThreadManager().threadIsRunning(threadId))
	{
		operation.pulse();

		if (operation.messageChanged())
		{
			operation.dispatch();
		}

		wxTheApp->Yield();

		wxThread::Sleep(15); // sleep for a few ms, then ask again
	}

	GlobalMainFrame().updateAllWindows();
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
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_SELECTIONSYSTEM);
		_dependencies.insert(MODULE_RENDERSYSTEM);
		_dependencies.insert(MODULE_CLIPPER);
	}

	return _dependencies;
}

void RadiantModule::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << "RadiantModule::initialiseModule called." << std::endl;

	// Reset the node id count
  	scene::Node::resetIds();

    selection::algorithm::registerCommands();
	brush::algorithm::registerCommands();

	GlobalCommandSystem().addCommand("Exit", exitCmd);
	GlobalEventManager().addCommand("Exit", "Exit");

    // Subscribe for the post-module init event
    module::GlobalModuleRegistry().signal_allModulesInitialised().connect(
        sigc::mem_fun(this, &RadiantModule::postModuleInitialisation));
}

void RadiantModule::shutdownModule()
{
	rMessage() << "RadiantModule::shutdownModule called." << std::endl;

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

void RadiantModule::exitCmd(const cmd::ArgumentList& args)
{
    // Just tell the main application window to close, which will invoke
    // appropriate event handlers.
    GlobalMainFrame().getWxTopLevelWindow()->Close(false /* don't force */);
}

// Define the static Radiant module
module::StaticModule<RadiantModule> radiantCoreModule;

// Return the static Radiant module to other code within the main binary
RadiantModulePtr getGlobalRadiant()
{
	return radiantCoreModule.getModule();
}

} // namespace radiant

