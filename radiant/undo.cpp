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

#include "iundo.h"
#include "preferencesystem.h"
#include "string/string.h"
#include "generic/callback.h"
#include "preferences.h"
#include "stringio.h"

#include <list>
#include <map>
#include <set>

#include "undo/SnapShot.h"
#include "undo/Operation.h"
#include "undo/Stack.h"
#include "undo/StackFiller.h"

class RadiantUndoSystem : 
	public UndoSystem 
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

	RadiantUndoSystem()
		: _undoLevels(64) 
	{}

	~RadiantUndoSystem() {
		clear();
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

private:

	// Assigns the given stack to all of the Undoables listed in the map
	void mark_undoables(undo::UndoStack* stack) {
		for (UndoablesMap::iterator i = _undoables.begin(); i != _undoables.end(); ++i) {
			i->second.setStack(stack);
		}
	}

}; // class RadiantUndoSystem



void UndoLevels_importString(RadiantUndoSystem& undo, const char* value) {
	int levels;
	Int_importString(levels, value);
	undo.setLevels(levels);
}
typedef ReferenceCaller1<RadiantUndoSystem, const char*, UndoLevels_importString> UndoLevelsImportStringCaller;
void UndoLevels_exportString(const RadiantUndoSystem& undo, const StringImportCallback& importer) {
	Int_exportString(static_cast<int>(undo.getLevels()), importer);
}
typedef ConstReferenceCaller1<RadiantUndoSystem, const StringImportCallback&, UndoLevels_exportString> UndoLevelsExportStringCaller;

#include "generic/callback.h"

void UndoLevelsImport(RadiantUndoSystem& self, int value) {
	self.setLevels(value);
}
typedef ReferenceCaller1<RadiantUndoSystem, int, UndoLevelsImport> UndoLevelsImportCaller;
void UndoLevelsExport(const RadiantUndoSystem& self, const IntImportCallback& importCallback) {
	importCallback(static_cast<int>(self.getLevels()));
}
typedef ConstReferenceCaller1<RadiantUndoSystem, const IntImportCallback&, UndoLevelsExport> UndoLevelsExportCaller;


void Undo_constructPreferences(RadiantUndoSystem& undo, PrefPage* page) {
	page->appendSpinner("Undo Queue Size", 64, 0, 1024, IntImportCallback(UndoLevelsImportCaller(undo)), IntExportCallback(UndoLevelsExportCaller(undo)));
}
void Undo_constructPage(RadiantUndoSystem& undo, PreferenceGroup& group) {
	PreferencesPage* page(group.createPage("Undo", "Undo Queue Settings"));
	Undo_constructPreferences(undo, reinterpret_cast<PrefPage*>(page));
}
void Undo_registerPreferencesPage(RadiantUndoSystem& undo) {
	PreferencesDialog_addSettingsPage(ReferenceCaller1<RadiantUndoSystem, PreferenceGroup&, Undo_constructPage>(undo));
}

class UndoSystemDependencies : public GlobalPreferenceSystemModuleRef {};

class UndoSystemAPI {
	RadiantUndoSystem m_undosystem;
public:
	typedef UndoSystem Type;
	STRING_CONSTANT(Name, "*");

	UndoSystemAPI() {
		GlobalPreferenceSystem().registerPreference("UndoLevels", makeIntStringImportCallback(UndoLevelsImportCaller(m_undosystem)), makeIntStringExportCallback(UndoLevelsExportCaller(m_undosystem)));

		Undo_registerPreferencesPage(m_undosystem);
	}
	UndoSystem* getTable() {
		return &m_undosystem;
	}
};

#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"

typedef SingletonModule<UndoSystemAPI, UndoSystemDependencies> UndoSystemModule;
typedef Static<UndoSystemModule> StaticUndoSystemModule;
StaticRegisterModule staticRegisterUndoSystem(StaticUndoSystemModule::instance());










class undoable_test : public Undoable {
struct state_type : public UndoMemento {
		state_type() : test_data(0) {}
		state_type(const state_type& other) : UndoMemento(other), test_data(other.test_data) {}
		void release() {
			delete this;
		}

		int test_data;
	};
	state_type m_state;
	UndoObserver* m_observer;
public:
	undoable_test()
			: m_observer(GlobalUndoSystem().observer(this)) {}
	~undoable_test() {
		GlobalUndoSystem().release(this);
	}
	UndoMemento* exportState() const {
		return new state_type(m_state);
	}
	void importState(const UndoMemento* state) {
		ASSERT_NOTNULL(state);

		m_observer->save(this);
		m_state = *(static_cast<const state_type*>(state));
	}

	void mutate(unsigned int data) {
		m_observer->save(this);
		m_state.test_data = data;
	}
};

#if 0

class TestUndo {
public:
	TestUndo() {
		undoable_test test;
		GlobalUndoSystem().begin("bleh");
		test.mutate(3);
		GlobalUndoSystem().begin("blah");
		test.mutate(4);
		GlobalUndoSystem().undo();
		GlobalUndoSystem().undo();
		GlobalUndoSystem().redo();
		GlobalUndoSystem().redo();
	}
};

TestUndo g_TestUndo;

#endif

