#include "iundo.h"

/* greebo: The RadiantUndoSystem (interface: iundo.h) is maintaining an internal
 * stack of UndoCommands that contains the pointers to the Undoables as well as
 * to their UndoMementos. 
 * 
 * On undo, the Undoables are called to re-import the states stored in the UndoMementos.  
 */

#include "iregistry.h"
#include "preferencesystem.h"

#include <map>
#include <set>

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
	public PreferenceConstructor,
	public RegistryKeyObserver 
{
public:
	// Radiant Module stuff
	typedef UndoSystem Type;
	STRING_CONSTANT(Name, "*");

	// Return the static instance
	UndoSystem* getTable() {
		return this;
	}

private:
	INTEGER_CONSTANT(MAX_UNDO_LEVELS, 1024);

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
	RadiantUndoSystem()
		: _undoLevels(GlobalRegistry().getInt(RKEY_UNDO_QUEUE_SIZE)) 
	{
		// Add self to the key observers to get notified on change
		GlobalRegistry().addKeyObserver(this, RKEY_UNDO_QUEUE_SIZE);
		
		// greebo: Register this class in the preference system so that the constructPreferencePage() gets called.
		GlobalPreferenceSystem().addConstructor(this);
	}

	~RadiantUndoSystem() {
		clear();
	}

	// Gets called as soon as the observed registry keys get changed
	void keyChanged() {
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
		if (static_cast<int>(levels) > MAX_UNDO_LEVELS()) {
			levels = MAX_UNDO_LEVELS();
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
		}
	}

	void clear() {
		mark_undoables(0);
		_undoStack.clear();
		_redoStack.clear();
		trackersClear();
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
	void constructPreferencePage(PreferenceGroup& group) {
		PreferencesPage* page(group.createPage("Undo", "Undo Queue Settings"));
		page->appendSpinner("Undo Queue Size", RKEY_UNDO_QUEUE_SIZE, 0, 1024);
	}

private:

	// Assigns the given stack to all of the Undoables listed in the map
	void mark_undoables(UndoStack* stack) {
		for (UndoablesMap::iterator i = _undoables.begin(); i != _undoables.end(); ++i) {
			i->second.setStack(stack);
		}
	}

}; // class RadiantUndoSystem

} // namespace undo

// The dependencies class
class RadiantUndoSystemDependencies : 
	public GlobalRegistryModuleRef,
	public GlobalPreferenceSystemModuleRef 
{};

/* Required code to register the module with the ModuleServer.
 */

#include "modulesystem/singletonmodule.h"

typedef SingletonModule<undo::RadiantUndoSystem, RadiantUndoSystemDependencies> RadiantUndoSystemModule;

// Static instance of the RadiantUndoSystemModule
RadiantUndoSystemModule _theRadiantUndoSystemModule;

extern "C" void RADIANT_DLLEXPORT Radiant_RegisterModules(ModuleServer& server)
{
  initialiseModule(server);
  _theRadiantUndoSystemModule.selfRegister();
}
