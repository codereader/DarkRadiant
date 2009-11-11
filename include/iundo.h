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

#if !defined(INCLUDED_IUNDO_H)
#define INCLUDED_IUNDO_H

/// \file
/// \brief The undo-system interface. Uses the 'memento' pattern.

#include "imodule.h"
#include <cstddef>
#include "generic/callbackfwd.h"

/* greebo: An UndoMemento has to be allocated on the heap
 * and contains all the information that is needed to describe
 * the status of an Undoable. 
 * 
 * Mandatory interface method is release() which should free 
 * itself from the heap.  
 */
class UndoMemento
{
public:
    virtual ~UndoMemento() {}
	virtual void release() = 0;
};

/* greebo: This is the abstract base class for an Undoable object.
 * Derive from this class if your instance/object should be Undoable.
 * 
 * The exportState method has to allocate a new UndoMemento with all
 * the necessary object data and return its pointer, so it can be
 * referenced to later.
 * 
 * The importState() method should re-import the values saved in the
 * UndoMemento (could be named restoreFromMemento() as well). 
 */
class Undoable
{
public:
    virtual ~Undoable() {}
	virtual UndoMemento* exportState() const = 0;
	virtual void importState(const UndoMemento* state) = 0;
};

class UndoObserver
{
public:
    virtual ~UndoObserver() {}
	virtual void save(Undoable* undoable) = 0;
};

class UndoTracker
{
public:
    virtual ~UndoTracker() {}
	virtual void clear() = 0;
	virtual void begin() = 0;
	virtual void undo() = 0;
	virtual void redo() = 0;
};

const std::string MODULE_UNDOSYSTEM("UndoSystem");

class UndoSystem :
	public RegisterableModule
{
public:
	virtual UndoObserver* observer(Undoable* undoable) = 0;
	virtual void release(Undoable* undoable) = 0;

	virtual std::size_t size() const = 0;
	virtual void start() = 0;
	virtual void finish(const std::string& command) = 0;
	virtual void undo() = 0;
	virtual void redo() = 0;
	virtual void clear() = 0;

	class Observer {
	public:
	    virtual ~Observer() {}
		// Gets called after an undo operation is fully completed, allows objects to refresh their state
		virtual void postUndo() = 0;
		// Gets called after a redo operation is fully completed, allows objects to refresh their state
		virtual void postRedo() = 0;
	};

	// Adds/removes an observer, which gets called on certain events
	virtual void addObserver(Observer* observer) = 0;
	virtual void removeObserver(Observer* observer) = 0;
	
	// greebo: This finishes the current operation and removes
	// it immediately from the stack, therefore it never existed. 
	virtual void cancel() = 0;

	virtual void trackerAttach(UndoTracker& tracker) = 0;
	virtual void trackerDetach(UndoTracker& tracker) = 0;
};

// The accessor function
inline UndoSystem& GlobalUndoSystem() {
	// Cache the reference locally
	static UndoSystem& _undoSystem(
		*boost::static_pointer_cast<UndoSystem>(
			module::GlobalModuleRegistry().getModule(MODULE_UNDOSYSTEM)
		)
	);
	return _undoSystem;
}

class UndoableCommand
{
	const std::string _command;
public:

	UndoableCommand(const std::string& command) : 
		_command(command) 
	{
		GlobalUndoSystem().start();
	}

	~UndoableCommand() {
		GlobalUndoSystem().finish(_command);
	}
};

#endif
