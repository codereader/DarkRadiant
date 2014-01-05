#include "iundo.h"

#include "imodule.h"
#include "i18n.h"

#include "icommandsystem.h"
#include "itextstream.h"
#include "ieventmanager.h"
#include "ipreferencesystem.h"
#include "iscenegraph.h"

#include <iostream>
#include <map>
#include <set>

#include "registry/registry.h"
#include "SnapShot.h"
#include "Operation.h"
#include "Stack.h"
#include "StackFiller.h"

#include <boost/bind.hpp>

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
	// The operation Observers which get notified on certain events
	// This is not a set to retain the order of the observers
	typedef std::list<Observer*> Observers;
	Observers _observers;

	static const std::size_t MAX_UNDO_LEVELS = 16384;

	// The undo and redo stacks
	UndoStack _undoStack;
	UndoStack _redoStack;

	typedef std::map<IUndoable*, UndoStackFiller> UndoablesMap;
	UndoablesMap _undoables;

	std::size_t _undoLevels;

	typedef std::set<IUndoTracker*> Trackers;
	Trackers _trackers;

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

	void releaseStateSaver(IUndoable& undoable)
	{
		_undoables.erase(&undoable);
	}

	std::size_t size() const
	{
		return _undoStack.size();
	}

	void start()
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
	void cancel()
	{
		// Try to add the last operation as "temp"
		if (finishUndo("$TEMPORARY"))
		{
			// Instantly remove the added operation
			_undoStack.pop_back();
		}
	}

	void finish(const std::string& command) {
		if (finishUndo(command)) {
			rMessage() << command << std::endl;
		}
	}

	void undo()
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

		for (Observers::iterator i = _observers.begin(); i != _observers.end(); /* in-loop */)
		{
			Observer* observer = *(i++);
			observer->postUndo();
		}

		// Trigger the onPostUndo event on all scene nodes
		GlobalSceneGraph().foreachNode([&] (const scene::INodePtr& node)->bool
		{
			node->onPostUndo();
			return true;
		});

		GlobalSceneGraph().sceneChanged();
	}

	void redo()
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

		for (Observers::iterator i = _observers.begin(); i != _observers.end(); /* in-loop */)
		{
			Observer* observer = *(i++);
			observer->postRedo();
		}

		// Trigger the onPostRedo event on all scene nodes
		GlobalSceneGraph().foreachNode([&] (const scene::INodePtr& node)->bool
		{
			node->onPostRedo();
			return true;
		});

		GlobalSceneGraph().sceneChanged();
	}

	void clear()
	{
		setActiveUndoStack(NULL);
		_undoStack.clear();
		_redoStack.clear();
		trackersClear();

		// greebo: This is called on map shutdown, so don't clear the observers,
		// there are some "persistent" observers like EntityInspector and ShaderClipboard
	}

	void addObserver(Observer* observer)
	{
		// Ensure no observer is added twice
		assert(std::find(_observers.begin(), _observers.end(), observer) == _observers.end());

		// Observers are added to the end of the list
		_observers.push_back(observer);
	}

	void removeObserver(Observer* observer)
	{
		Observers::iterator i = std::find(_observers.begin(), _observers.end(), observer);

		// Ensure that the observer is actually registered
		assert(i != _observers.end());

		if (i != _observers.end())
		{
			_observers.erase(i);
		}
	}

	void attachTracker(IUndoTracker& tracker)
	{
		ASSERT_MESSAGE(_trackers.find(&tracker) == _trackers.end(), "undo tracker already attached");
		_trackers.insert(&tracker);
	}

	void detachTracker(IUndoTracker& tracker)
	{
		ASSERT_MESSAGE(_trackers.find(&tracker) != _trackers.end(), "undo tracker cannot be detached");
		_trackers.erase(&tracker);
	}

	// RegisterableModule implementation
	virtual const std::string& getName() const
	{
		static std::string _name(MODULE_UNDOSYSTEM);
		return _name;
	}

	virtual const StringSet& getDependencies() const
	{
		static StringSet _dependencies;

		if (_dependencies.empty())
		{
			_dependencies.insert(MODULE_XMLREGISTRY);
			_dependencies.insert(MODULE_PREFERENCESYSTEM);
			_dependencies.insert(MODULE_COMMANDSYSTEM);
			_dependencies.insert(MODULE_SCENEGRAPH);
			_dependencies.insert(MODULE_EVENTMANAGER);
		}

		return _dependencies;
	}

	virtual void initialiseModule(const ApplicationContext& ctx)
	{
		rMessage() << "UndoSystem::initialiseModule called" << std::endl;

		// Add commands for console input
		GlobalCommandSystem().addCommand("Undo", boost::bind(&RadiantUndoSystem::undoCmd, this, _1));
		GlobalCommandSystem().addCommand("Redo", boost::bind(&RadiantUndoSystem::redoCmd, this, _1));

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

	void foreachTracker(const std::function<void(IUndoTracker&)>& functor) const
	{
		std::for_each(_trackers.begin(), _trackers.end(), [&] (IUndoTracker* tracker)
		{ 
			functor(*tracker);
		});
	}

	void trackersClear() const
	{
		foreachTracker([&] (IUndoTracker& tracker) { tracker.clear(); });
	}

	void trackersBegin() const
	{
		foreachTracker([&] (IUndoTracker& tracker) { tracker.begin(); });
	}

	void trackersUndo() const
	{
		foreachTracker([&] (IUndoTracker& tracker) { tracker.undo(); });
	}

	void trackersRedo() const
	{
		foreachTracker([&] (IUndoTracker& tracker) { tracker.redo(); });
	}

	// Gets called by the PreferenceSystem as request to create the according settings page
	void constructPreferences() 
	{
		PreferencesPagePtr page = GlobalPreferenceSystem().getPage(_("Settings/Undo System"));
		page->appendSpinner(_("Undo Queue Size"), RKEY_UNDO_QUEUE_SIZE, 0, 1024, 1);
	}

}; // class RadiantUndoSystem
typedef boost::shared_ptr<RadiantUndoSystem> RadiantUndoSystemPtr;

} // namespace undo

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(undo::RadiantUndoSystemPtr(new undo::RadiantUndoSystem));

	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());

	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);

	// Set up the assertion handler
	GlobalErrorHandler() = registry.getApplicationContext().getErrorHandlingFunction();
}
