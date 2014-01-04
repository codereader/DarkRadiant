#pragma once

#include "iundo.h"

namespace undo
{

/**
 * greebo: A StateApplicator applies the saved state to
 * an Undoable. The pointers to the Undables and their
 * UndoMementos are stored internally.
 */
class StateApplicator
{
public:
	Undoable& _undoable;
private:
	UndoMemento* _data;
public:
	// Constructor
	StateApplicator(Undoable& undoable) :
		_undoable(undoable), 
		_data(_undoable.exportState())
	{}

	void restore() {
		_undoable.importState(_data);
	}

	void release() {
		_data->release();
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
	public std::list<StateApplicator>
{
public:
	// Adds a StateApplicator to the internal list. The Undoable pointer is saved as well as
	// the pointer to its UndoMemento (queried by exportState().
	void save(Undoable& undoable)
	{
		push_front(StateApplicator(undoable));
	}

	// Cycles through all the StateApplicators and tells them to restore the state.
	void restore()
	{
		std::for_each(begin(), end(), [&] (StateApplicator& state)
		{
			state.restore();
		});
	}

	// Releases all the UndoMemento from the heap by cycling through the StateApplicators
	void release()
	{
		std::for_each(begin(), end(), [&] (StateApplicator& state)
		{
			state.release();
		});
	}

}; // class Snapshot

} // namespace undo
