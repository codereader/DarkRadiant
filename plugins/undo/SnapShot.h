#pragma once

#include "iundo.h"

namespace undo
{

/**
 * greebo: An UndoMementoKeeper can apply the saved state to
 * an Undoable on request. The pointers to the Undables and their
 * UndoMementos are stored internally.
 */
class UndoMementoKeeper
{
public:
	IUndoable& _undoable;
private:
	IUndoMementoPtr _data;
public:
	// Constructor
	UndoMementoKeeper(IUndoable& undoable) :
		_undoable(undoable), 
		_data(_undoable.exportState())
	{}

	void restoreState()
	{
		_undoable.importState(_data);
	}
};

/** 
 * greebo: Basically, this class can contain a whole list of Undoables and all their UndoMementos,
 * as there can be multiple Undoables whose states have to be saved in a Snapshot.
 *
 * What happens on save(): The Undable is queried for its UndoMemento (the actual data)
 * whose pointer is stored along with the Undable* itself into a list.
 *
 * Upon request (restore() or release()) the UndoMementos are restored back to their according
 * Undoables or released from memory, resp.
 */
class Snapshot :
	public std::list<UndoMementoKeeper>
{
public:
	// Adds a StateApplicator to the internal list. The Undoable pointer is saved as well as
	// the pointer to its UndoMemento (queried by exportState().
	void save(IUndoable& undoable)
	{
		push_front(UndoMementoKeeper(undoable));
	}

	// Cycles through all the StateApplicators and tells them to restore the state.
	void restore()
	{
		std::for_each(begin(), end(), [&] (UndoMementoKeeper& state)
		{
			state.restoreState();
		});
	}
};

} // namespace undo
