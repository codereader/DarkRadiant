#include "iundo.h"

#include "imodule.h"
#include "i18n.h"

#include "icommandsystem.h"
#include "itextstream.h"
#include "ieventmanager.h"
#include "ipreferencesystem.h"
#include "iscenegraph.h"
#include "imap.h"

#include <iostream>
#include <map>
#include <set>

#include "registry/registry.h"
#include "SnapShot.h"
#include "Operation.h"
#include "Stack.h"
#include "StackFiller.h"

#include <functional>

namespace undo {

namespace
{
	const std::string RKEY_UNDO_QUEUE_SIZE = "user/ui/undo/queueSize";
}

/** 
 * greebo: The RadiantUndoSystem (interface: iundo.h) is maintaining an internal
 * stack of UndoCommands that contains the pointers to the Undoables as well as
 * to their UndoMementos.
 *
 * On undo or redo, the Undoables are called to re-import the states 
 * as stored in the UndoMementos.
 */
class RadiantUndoSystem : 
	public UndoSystem
{
private:
	static const std::size_t MAX_UNDO_LEVELS = 16384;

	// The undo and redo stacks
	UndoStack _undoStack;
	UndoStack _redoStack;

	typedef std::map<IUndoable*, UndoStackFiller> UndoablesMap;
	UndoablesMap _undoables;

	std::size_t _undoLevels;

	typedef std::set<Tracker*> Trackers;
	Trackers _trackers;

	sigc::signal<void> _signalPostUndo;
	sigc::signal<void> _signalPostRedo;

public:
	// Constructor
	RadiantUndoSystem() :
		_undoLevels(64)
	{}

	virtual ~RadiantUndoSystem()
	{
		clear();
	}

	// Gets called as soon as the observed registry keys get changed
	void keyChanged()
    {
		_undoLevels = registry::getValue<int>(RKEY_UNDO_QUEUE_SIZE);
	}

	IUndoStateSaver* getStateSaver(IUndoable& undoable)
	{
		return &_undoables[&undoable];
	}

    IUndoStateSaver* getStateSaver(IUndoable& undoable, IMapFileChangeTracker& tracker) override
    {
        auto result = _undoables.insert(std::make_pair(&undoable, UndoStackFiller(tracker)));
        return &(result.first->second);
    }

	void releaseStateSaver(IUndoable& undoable) override
	{
		_undoables.erase(&undoable);
	}

	std::size_t size() const override
	{
		return _undoStack.size();
	}

	void start() override
	{
		_redoStack.clear();
		if (_undoStack.size() == _undoLevels)
		{
			_undoStack.pop_front();
		}
		startUndo();
		trackersBegin();
	}

	// greebo: This finishes the current operation and
	// removes it instantly from the stack
	void cancel() override
	{
		// Try to add the last operation as "temp"
		if (finishUndo("$TEMPORARY"))
		{
			// Instantly remove the added operation
			_undoStack.pop_back();
		}
	}

	void finish(const std::string& command) override
	{
		if (finishUndo(command)) {
			rMessage() << command << std::endl;
		}
	}

	void undo() override
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

	void redo() override
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

	void clear() override
	{
		setActiveUndoStack(NULL);
		_undoStack.clear();
		_redoStack.clear();
		trackersClear();

		// greebo: This is called on map shutdown, so don't clear the observers,
		// there are some "persistent" observers like EntityInspector and ShaderClipboard
	}

	sigc::signal<void>& signal_postUndo() override
	{
		return _signalPostUndo;
	}

	// Emitted after a redo operation is fully completed, allows objects to refresh their state
	sigc::signal<void>& signal_postRedo() override
	{
		return _signalPostRedo;
	}

	void attachTracker(Tracker& tracker) override
	{
		ASSERT_MESSAGE(_trackers.find(&tracker) == _trackers.end(), "undo tracker already attached");
		_trackers.insert(&tracker);
	}

	void detachTracker(Tracker& tracker) override
	{
		ASSERT_MESSAGE(_trackers.find(&tracker) != _trackers.end(), "undo tracker cannot be detached");
		_trackers.erase(&tracker);
	}

	// RegisterableModule implementation
	virtual const std::string& getName() const override
	{
		static std::string _name(MODULE_UNDOSYSTEM);
		return _name;
	}

	virtual const StringSet& getDependencies() const override
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

	virtual void initialiseModule(const ApplicationContext& ctx) override
	{
		rMessage() << "UndoSystem::initialiseModule called" << std::endl;

		// Add commands for console input
		GlobalCommandSystem().addCommand("Undo", std::bind(&RadiantUndoSystem::undoCmd, this, std::placeholders::_1));
		GlobalCommandSystem().addCommand("Redo", std::bind(&RadiantUndoSystem::redoCmd, this, std::placeholders::_1));

		// Bind events to commands
		GlobalEventManager().addCommand("Undo", "Undo");
		GlobalEventManager().addCommand("Redo", "Redo");

		_undoLevels = registry::getValue<int>(RKEY_UNDO_QUEUE_SIZE);

		// Add self to the key observers to get notified on change
		GlobalRegistry().signalForKey(RKEY_UNDO_QUEUE_SIZE).connect(
            sigc::mem_fun(this, &RadiantUndoSystem::keyChanged)
        );

		// add the preference settings
		constructPreferences();

		GlobalMapModule().signal_mapEvent().connect(
			sigc::mem_fun(*this, &RadiantUndoSystem::onMapEvent)
		);
	}

	// This is connected to the CommandSystem
	void undoCmd(const cmd::ArgumentList& args)
	{
		undo();
	}

	// This is connected to the CommandSystem
	void redoCmd(const cmd::ArgumentList& args)
	{
		redo();
	}

private:
	void onMapEvent(IMap::MapEvent ev)
	{
		if (ev == IMap::MapUnloaded)
		{
			clear();
		}
	}

	// Sets the size of the undoStack
	void setLevels(std::size_t levels) 
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

	std::size_t getLevels() const
	{
		return _undoLevels;
	}

	void startUndo()
	{
		_undoStack.start("unnamedCommand");
		setActiveUndoStack(&_undoStack);
	}

	bool finishUndo(const std::string& command)
	{
		bool changed = _undoStack.finish(command);
		setActiveUndoStack(NULL);
		return changed;
	}

	void startRedo()
	{
		_redoStack.start("unnamedCommand");
		setActiveUndoStack(&_redoStack);
	}

	bool finishRedo(const std::string& command)
	{
		bool changed = _redoStack.finish(command);
		setActiveUndoStack(NULL);
		return changed;
	}

	// Assigns the given stack to all of the Undoables listed in the map
	void setActiveUndoStack(UndoStack* stack)
	{
		for (UndoablesMap::iterator i = _undoables.begin(); i != _undoables.end(); ++i)
		{
			i->second.setStack(stack);
		}
	}

	void foreachTracker(const std::function<void(Tracker&)>& functor) const
	{
		std::for_each(_trackers.begin(), _trackers.end(), [&] (Tracker* tracker)
		{ 
			functor(*tracker);
		});
	}

	void trackersClear() const
	{
		foreachTracker([&] (Tracker& tracker) { tracker.clear(); });
	}

	void trackersBegin() const
	{
		foreachTracker([&] (Tracker& tracker) { tracker.begin(); });
	}

	void trackersUndo() const
	{
		foreachTracker([&] (Tracker& tracker) { tracker.undo(); });
	}

	void trackersRedo() const
	{
		foreachTracker([&] (Tracker& tracker) { tracker.redo(); });
	}

	// Gets called by the PreferenceSystem as request to create the according settings page
	void constructPreferences() 
	{
		IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Settings/Undo System"));
		page.appendSpinner(_("Undo Queue Size"), RKEY_UNDO_QUEUE_SIZE, 0, 1024, 1);
	}

}; // class RadiantUndoSystem
typedef std::shared_ptr<RadiantUndoSystem> RadiantUndoSystemPtr;

} // namespace undo

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(undo::RadiantUndoSystemPtr(new undo::RadiantUndoSystem));
}
