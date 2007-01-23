/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "undo.h"

#include "debugging/debugging.h"
#include "warnings.h"

#include "iregistry.h"
#include "iundo.h"
#include "preferencesystem.h"
#include "preferences.h"

#include <list>
#include <map>
#include <set>

#include "undo/SnapShot.h"
#include "undo/Operation.h"
#include "undo/Stack.h"
#include "undo/StackFiller.h"

namespace {
	const std::string RKEY_UNDO_QUEUE_SIZE = "user/ui/undo/queueSize";
}

class RadiantUndoSystem : 
	public UndoSystem,
	public PreferenceConstructor,
	public RegistryKeyObserver
{
	INTEGER_CONSTANT(MAX_UNDO_LEVELS, 1024);

	// The undo and redo stacks
	undo::UndoStack _undoStack;
	undo::UndoStack _redoStack;

	typedef std::map<Undoable*, undo::UndoStackFiller> UndoablesMap;
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

	bool finishUndo(const char* command) {
		bool changed = _undoStack.finish(command);
		mark_undoables(0);
		return changed;
	}

	void startRedo() {
		_redoStack.start("unnamedCommand");
		mark_undoables(&_redoStack);
	}

	bool finishRedo(const char* command) {
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

	void finish(const char* command) {
		if (finishUndo(command)) {
			globalOutputStream() << command << '\n';
		}
	}

	void undo() {
		if (_undoStack.empty()) {
			globalOutputStream() << "Undo: no undo available\n";
		}
		else {
			undo::Operation* operation = _undoStack.back();
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
			undo::Operation* operation = _redoStack.back();
			globalOutputStream() << "Redo: " << operation->_command.c_str() << "\n";

			startUndo();
			trackersRedo();
			operation->_snapshot.restore();
			finishUndo(operation->_command.c_str());
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
	void mark_undoables(undo::UndoStack* stack) {
		for (UndoablesMap::iterator i = _undoables.begin(); i != _undoables.end(); ++i) {
			i->second.setStack(stack);
		}
	}

}; // class RadiantUndoSystem

#include "generic/callback.h"

// The dependencies class
class UndoSystemDependencies : 
	public GlobalRegistryModuleRef,
	public GlobalPreferenceSystemModuleRef 
{};

class UndoSystemAPI 
{
	RadiantUndoSystem _undoSystem;
public:
	typedef UndoSystem Type;
	STRING_CONSTANT(Name, "*");

	UndoSystemAPI() {}
	
	UndoSystem* getTable() {
		return &_undoSystem;
	}
};

#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"

typedef SingletonModule<UndoSystemAPI, UndoSystemDependencies> UndoSystemModule;
typedef Static<UndoSystemModule> StaticUndoSystemModule;
StaticRegisterModule staticRegisterUndoSystem(StaticUndoSystemModule::instance());
