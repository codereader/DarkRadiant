#include "iundo.h"

/* greebo: The RadiantUndoSystem (interface: iundo.h) is maintaining an internal
 * stack of UndoCommands that contains the pointers to the Undoables as well as
 * to their UndoMementos. 
 * 
 * On undo, the Undoables are called to re-import the states stored in the UndoMementos.  
 */

#include "iregistry.h"
#include "ipreferencesystem.h"

#include <iostream>
#include <map>
#include <set>

#include "imodule.h"

#include "SnapShot.h"
#include "Operation.h"
#include "Stack.h"
#include "StackFiller.h"

namespace undo {

namespace {
	const std::string RKEY_UNDO_QUEUE_SIZE = "user/ui/undo/queueSize";
}

class RadiantUndoSystem : 
	public UndoSystem,
	public RegistryKeyObserver
{
	// The operation Observers which get notified on certain events
	typedef std::set<Observer*> ObserverSet;
	ObserverSet _observers;

	static const std::size_t MAX_UNDO_LEVELS = 1024;

	// The undo and redo stacks
	UndoStack _undoStack;
	UndoStack _redoStack;

	typedef std::map<Undoable*, UndoStackFiller> UndoablesMap;
	UndoablesMap _undoables;
	
	std::size_t _undoLevels;

	typedef std::set<UndoTracker*> Trackers;
	Trackers _trackers;

public:
	// Constructor
	RadiantUndoSystem() : 
		_undoLevels(64) 
	{}

	virtual ~RadiantUndoSystem() {
		clear();
	}

	// Gets called as soon as the observed registry keys get changed
	void keyChanged(const std::string& key, const std::string& val) {
        // TODO: use val here
		_undoLevels = GlobalRegistry().getInt(RKEY_UNDO_QUEUE_SIZE);
	}

	UndoObserver* observer(Undoable* undoable) {
		ASSERT_NOTNULL(undoable);

		return &_undoables[undoable];
	}

	void release(Undoable* undoable) {
		ASSERT_NOTNULL(undoable);

		_undoables.erase(undoable);
	}

	// Sets the size of the undoStack
	void setLevels(std::size_t levels) {
		if (levels > MAX_UNDO_LEVELS) {
			levels = MAX_UNDO_LEVELS;
		}

		while (_undoStack.size() > levels) {
			_undoStack.pop_front();
		}
		_undoLevels = levels;
	}

	std::size_t getLevels() const {
		return _undoLevels;
	}

	std::size_t size() const {
		return _undoStack.size();
	}

	void startUndo() {
		_undoStack.start("unnamedCommand");
		mark_undoables(&_undoStack);
	}

	bool finishUndo(const std::string& command) {
		bool changed = _undoStack.finish(command);
		mark_undoables(0);
		return changed;
	}

	void startRedo() {
		_redoStack.start("unnamedCommand");
		mark_undoables(&_redoStack);
	}

	bool finishRedo(const std::string& command) {
		bool changed = _redoStack.finish(command);
		mark_undoables(0);
		return changed;
	}

	void start() {
		_redoStack.clear();
		if (_undoStack.size() == _undoLevels) {
			_undoStack.pop_front();
		}
		startUndo();
		trackersBegin();
	}

	// greebo: This finishes the current operation and
	// removes it instantly from the stack
	void cancel() {
		// Try to add the last operation as "temp"
		if (finishUndo("$TEMPORARY")) {
			// Instantly remove the added operation
			_undoStack.pop_back();
		}
	}

	void finish(const std::string& command) {
		if (finishUndo(command)) {
			globalOutputStream() << command.c_str() << '\n';
		}
	}

	void undo() {
		if (_undoStack.empty()) {
			globalOutputStream() << "Undo: no undo available\n";
		}
		else {
			Operation* operation = _undoStack.back();
			globalOutputStream() << "Undo: " << operation->_command.c_str() << "\n";

			startRedo();
			trackersUndo();
			operation->_snapshot.restore();
			finishRedo(operation->_command.c_str());
			_undoStack.pop_back();

			for (ObserverSet::iterator i = _observers.begin(); i != _observers.end(); /* in-loop */) {
				Observer* observer = *(i++);
				observer->postUndo();
			}
		}
	}

	void redo() {
		if (_redoStack.empty()) {
			globalOutputStream() << "Redo: no redo available\n";
		}
		else {
			Operation* operation = _redoStack.back();
			globalOutputStream() << "Redo: " << operation->_command.c_str() << "\n";

			startUndo();
			trackersRedo();
			operation->_snapshot.restore();
			finishUndo(operation->_command);
			_redoStack.pop_back();

			for (ObserverSet::iterator i = _observers.begin(); i != _observers.end(); /* in-loop */) {
				Observer* observer = *(i++);
				observer->postRedo();
			}
		}
	}

	void clear() {
		mark_undoables(0);
		_undoStack.clear();
		_redoStack.clear();
		trackersClear();
		_observers.clear();
	}

	void addObserver(Observer* observer) {
		_observers.insert(observer);
	}

	void removeObserver(Observer* observer) {
		_observers.erase(observer);
	}

	void trackerAttach(UndoTracker& tracker) {
		ASSERT_MESSAGE(_trackers.find(&tracker) == _trackers.end(), "undo tracker already attached");
		_trackers.insert(&tracker);
	}

	void trackerDetach(UndoTracker& tracker) {
		ASSERT_MESSAGE(_trackers.find(&tracker) != _trackers.end(), "undo tracker cannot be detached");
		_trackers.erase(&tracker);
	}

	void trackersClear() const {
		for (Trackers::const_iterator i = _trackers.begin(); i != _trackers.end(); ++i) {
			(*i)->clear();
		}
	}

	void trackersBegin() const {
		for (Trackers::const_iterator i = _trackers.begin(); i != _trackers.end(); ++i) {
			(*i)->begin();
		}
	}

	void trackersUndo() const {
		for (Trackers::const_iterator i = _trackers.begin(); i != _trackers.end(); ++i) {
			(*i)->undo();
		}
	}

	void trackersRedo() const {
		for (Trackers::const_iterator i = _trackers.begin(); i != _trackers.end(); ++i) {
			(*i)->redo();
		}
	}
	
	// Gets called by the PreferenceSystem as request to create the according settings page
	void constructPreferences() {
		PreferencesPagePtr page = GlobalPreferenceSystem().getPage("Settings/Undo System");
		page->appendSpinner("Undo Queue Size", RKEY_UNDO_QUEUE_SIZE, 0, 1024, 1);
	}

	// RegisterableModule implementation
	virtual const std::string& getName() const {
		static std::string _name(MODULE_UNDOSYSTEM);
		return _name;
	}

	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies;

		if (_dependencies.empty()) {
			_dependencies.insert(MODULE_XMLREGISTRY);
			_dependencies.insert(MODULE_PREFERENCESYSTEM);
		}

		return _dependencies;
	}

	virtual void initialiseModule(const ApplicationContext& ctx) {
		globalOutputStream() << "UndoSystem::initialiseModule called\n";
		_undoLevels = GlobalRegistry().getInt(RKEY_UNDO_QUEUE_SIZE);
		
		// Add self to the key observers to get notified on change
		GlobalRegistry().addKeyObserver(this, RKEY_UNDO_QUEUE_SIZE);
		
		// add the preference settings
		constructPreferences();
	}

private:

	// Assigns the given stack to all of the Undoables listed in the map
	void mark_undoables(UndoStack* stack) {
		for (UndoablesMap::iterator i = _undoables.begin(); i != _undoables.end(); ++i) {
			i->second.setStack(stack);
		}
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
}
