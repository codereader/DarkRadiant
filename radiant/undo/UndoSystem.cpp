#include "UndoSystem.h"

#include "i18n.h"

#include "itextstream.h"
#include "ieventmanager.h"
#include "ipreferencesystem.h"
#include "iscenegraph.h"

#include <iostream>

#include "registry/registry.h"
#include "modulesystem/StaticModule.h"
#include "Operation.h"
#include "StackFiller.h"

namespace undo 
{

namespace
{
	const std::string RKEY_UNDO_QUEUE_SIZE = "user/ui/undo/queueSize";
	const std::size_t MAX_UNDO_LEVELS = 16384;
}

// Constructor
UndoSystem::UndoSystem() :
	_undoLevels(64),
	_activeUndoStack(nullptr)
{}

UndoSystem::~UndoSystem()
{
	clear();
}

void UndoSystem::keyChanged()
{
	_undoLevels = registry::getValue<int>(RKEY_UNDO_QUEUE_SIZE);
}

IUndoStateSaver* UndoSystem::getStateSaver(IUndoable& undoable, IMapFileChangeTracker& tracker)
{
    auto result = _undoables.insert(std::make_pair(&undoable, UndoStackFiller(tracker)));

	// If we're in the middle of an active undo operation, assign this to the tracker (#4861)
	if (_activeUndoStack != nullptr)
	{
		result.first->second.setStack(_activeUndoStack);
	}

    return &(result.first->second);
}

void UndoSystem::releaseStateSaver(IUndoable& undoable)
{
	_undoables.erase(&undoable);
}

std::size_t UndoSystem::size() const
{
	return _undoStack.size();
}

void UndoSystem::start()
{
	_redoStack.clear();
	if (_undoStack.size() == _undoLevels)
	{
		_undoStack.pop_front();
	}
	startUndo();
	trackersBegin();
}

void UndoSystem::cancel()
{
	// Try to add the last operation as "temp"
	if (finishUndo("$TEMPORARY"))
	{
		// Instantly remove the added operation
		_undoStack.pop_back();
	}
}

void UndoSystem::finish(const std::string& command)
{
	if (finishUndo(command)) {
		rMessage() << command << std::endl;
	}
}

void UndoSystem::undo()
{
	if (_undoStack.empty())
	{
		rMessage() << "Undo: no undo available" << std::endl;
		return;
	}
		
	const OperationPtr& operation = _undoStack.back();
	rMessage() << "Undo: " << operation->getName() << std::endl;

	startRedo();
	trackersUndo();
	operation->restoreSnapshot();
	finishRedo(operation->getName());
	_undoStack.pop_back();

	_signalPostUndo.emit();

	// Trigger the onPostUndo event on all scene nodes
	GlobalSceneGraph().foreachNode([&] (const scene::INodePtr& node)->bool
	{
		node->onPostUndo();
		return true;
	});

	GlobalSceneGraph().sceneChanged();
}

void UndoSystem::redo()
{
	if (_redoStack.empty())
	{
		rMessage() << "Redo: no redo available" << std::endl;
		return;
	}
		
	const OperationPtr& operation = _redoStack.back();
	rMessage() << "Redo: " << operation->getName() << std::endl;

	startUndo();
	trackersRedo();
	operation->restoreSnapshot();
	finishUndo(operation->getName());
	_redoStack.pop_back();

	_signalPostRedo.emit();

	// Trigger the onPostRedo event on all scene nodes
	GlobalSceneGraph().foreachNode([&] (const scene::INodePtr& node)->bool
	{
		node->onPostRedo();
		return true;
	});

	GlobalSceneGraph().sceneChanged();
}

void UndoSystem::clear()
{
	setActiveUndoStack(nullptr);
	_undoStack.clear();
	_redoStack.clear();
	trackersClear();

	// greebo: This is called on map shutdown, so don't clear the observers,
	// there are some "persistent" observers like EntityInspector and ShaderClipboard
}

sigc::signal<void>& UndoSystem::signal_postUndo()
{
	return _signalPostUndo;
}

// Emitted after a redo operation is fully completed, allows objects to refresh their state
sigc::signal<void>& UndoSystem::signal_postRedo()
{
	return _signalPostRedo;
}

void UndoSystem::attachTracker(Tracker& tracker)
{
	ASSERT_MESSAGE(_trackers.find(&tracker) == _trackers.end(), "undo tracker already attached");
	_trackers.insert(&tracker);
}

void UndoSystem::detachTracker(Tracker& tracker)
{
	ASSERT_MESSAGE(_trackers.find(&tracker) != _trackers.end(), "undo tracker cannot be detached");
	_trackers.erase(&tracker);
}

const std::string& UndoSystem::getName() const
{
	static std::string _name(MODULE_UNDOSYSTEM);
	return _name;
}

const StringSet& UndoSystem::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_SCENEGRAPH);
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_MAP);
	}

	return _dependencies;
}

void UndoSystem::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << "UndoSystem::initialiseModule called" << std::endl;

	// Add commands for console input
	GlobalCommandSystem().addCommand("Undo", std::bind(&UndoSystem::undoCmd, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("Redo", std::bind(&UndoSystem::redoCmd, this, std::placeholders::_1));

	// Bind events to commands
	GlobalEventManager().addCommand("Undo", "Undo");
	GlobalEventManager().addCommand("Redo", "Redo");

	_undoLevels = registry::getValue<int>(RKEY_UNDO_QUEUE_SIZE);

	// Add self to the key observers to get notified on change
	GlobalRegistry().signalForKey(RKEY_UNDO_QUEUE_SIZE).connect(
        sigc::mem_fun(this, &UndoSystem::keyChanged)
    );

	// add the preference settings
	constructPreferences();

	GlobalMapModule().signal_mapEvent().connect(
		sigc::mem_fun(*this, &UndoSystem::onMapEvent)
	);
}

void UndoSystem::undoCmd(const cmd::ArgumentList& args)
{
	undo();
}

void UndoSystem::redoCmd(const cmd::ArgumentList& args)
{
	redo();
}

void UndoSystem::onMapEvent(IMap::MapEvent ev)
{
	if (ev == IMap::MapUnloaded)
	{
		clear();
	}
}

// Sets the size of the undoStack
void UndoSystem::setLevels(std::size_t levels)
{
	if (levels > MAX_UNDO_LEVELS)
	{
		levels = MAX_UNDO_LEVELS;
	}

	while (_undoStack.size() > levels)
	{
		_undoStack.pop_front();
	}
	_undoLevels = levels;
}

std::size_t UndoSystem::getLevels() const
{
	return _undoLevels;
}

void UndoSystem::startUndo()
{
	_undoStack.start("unnamedCommand");
	setActiveUndoStack(&_undoStack);
}

bool UndoSystem::finishUndo(const std::string& command)
{
	bool changed = _undoStack.finish(command);
	setActiveUndoStack(nullptr);
	return changed;
}

void UndoSystem::startRedo()
{
	_redoStack.start("unnamedCommand");
	setActiveUndoStack(&_redoStack);
}

bool UndoSystem::finishRedo(const std::string& command)
{
	bool changed = _redoStack.finish(command);
	setActiveUndoStack(nullptr);
	return changed;
}

// Assigns the given stack to all of the Undoables listed in the map
void UndoSystem::setActiveUndoStack(UndoStack* stack)
{
	_activeUndoStack = stack;

	for (UndoablesMap::value_type& pair : _undoables)
	{
		pair.second.setStack(_activeUndoStack);
	}
}

void UndoSystem::foreachTracker(const std::function<void(Tracker&)>& functor) const
{
	std::for_each(_trackers.begin(), _trackers.end(), [&] (Tracker* tracker)
	{ 
		functor(*tracker);
	});
}

void UndoSystem::trackersClear() const
{
	foreachTracker([&] (Tracker& tracker) { tracker.clear(); });
}

void UndoSystem::trackersBegin() const
{
	foreachTracker([&] (Tracker& tracker) { tracker.begin(); });
}

void UndoSystem::trackersUndo() const
{
	foreachTracker([&] (Tracker& tracker) { tracker.undo(); });
}

void UndoSystem::trackersRedo() const
{
	foreachTracker([&] (Tracker& tracker) { tracker.redo(); });
}

void UndoSystem::constructPreferences()
{
	IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Settings/Undo System"));
	page.appendSpinner(_("Undo Queue Size"), RKEY_UNDO_QUEUE_SIZE, 0, 1024, 1);
}

// Static module instance
module::StaticModule<UndoSystem> _staticUndoSystemModule;

} // namespace undo
