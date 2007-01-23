#ifndef SNAPSHOT_H_
#define SNAPSHOT_H_

#include "iundo.h"

/* greebo: Basically, this class can contain a whole list of Undoables and all their UndoMementos, 
 * as there can be multiple Undoables whose states have to be saved in a Snapshot.
 * 
 * What happens on save(): The Undable is queried for its UndoMemento (the actual data)
 * whose pointer is stored along with the Undable* itself into a list.
 * 
 * Upon request (restore() or release()) the UndoMementos are restored back to their according
 * Undoables or released from memory, resp.  
 */

namespace undo {
 
class Snapshot 
{
	/* greebo: A StateApplicator applies the saved state to
	 * an Undoable. The pointers to the Undables and their
	 * UndoMementos are stored internally.
	 */
	class StateApplicator 
	{
	public:
		Undoable* _undoable;
	private:
		UndoMemento* _data;
	public:
		// Constructor
		StateApplicator(Undoable* undoable, UndoMemento* data) : 
			_undoable(undoable), _data(data) 
		{}
				
		void restore() {
			_undoable->importState(_data);
		}

		void release() {
			_data->release();
		}
	};

	typedef std::list<StateApplicator> StateApplicatorList;
	StateApplicatorList _states;

public:

	bool empty() const {
		return _states.empty();
	}

	std::size_t size() const {
		return _states.size();
	}

	// Adds a StateApplicator to the internal list. The Undoable pointer is saved as well as
	// the pointer to its UndoMemento (queried by exportState().
	void save(Undoable* undoable) {
		_states.push_front(StateApplicator(undoable, undoable->exportState()));
	}

	// Cycles through all the StateApplicators and tells them to restore the state.
	void restore() {
		for (StateApplicatorList::iterator i = _states.begin(); i != _states.end(); ++i) {
			i->restore();
		}
	}

	// Releases all the UndoMemento from the heap by cycling through the StateApplicators
	void release() {
		for (StateApplicatorList::iterator i = _states.begin(); i != _states.end(); ++i) {
			i->release();
		}
	}

}; // class Snapshot

} // namespace undo

#endif /*SNAPSHOT_H_*/
