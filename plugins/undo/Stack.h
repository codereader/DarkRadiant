#ifndef UNDOSTACK_H_
#define UNDOSTACK_H_

#include "debugging/debugging.h"

namespace undo {

/* greebo: This is some kind of Stack implementation that keeps track
 * of all the Undo Operations.
 * 
 * When start() is called, a new Operation is allocated on the heap
 * and on calling save(Undoable*) the Undoable is actually stored within
 * the allocated Operation. The method finish() deallocates the 
 * memory used in this timespan.
 */

class UndoStack 
{
	//! Note: using std::list instead of vector/deque, to avoid copying of undos
	typedef std::list<Operation*> Operations;

	// The list of Operations that can be undone
	Operations _stack;
	
	// The pending undo operation (a working variable, so to say)
	Operation* _pending;

public:
	// Constructor
	UndoStack() : 
		_pending(NULL) 
	{}
	
	// Destructor
	~UndoStack() {
		// Free all the allocated memory
		clear();
	}
	
	bool empty() const {
		return _stack.empty();
	}
	
	std::size_t size() const {
		return _stack.size();
	}
	
	Operation* back() {
		return _stack.back();
	}
	
	const Operation* back() const {
		return _stack.back();
	}
	
	Operation* front() {
		return _stack.front();
	}
	
	const Operation* front() const {
		return _stack.front();
	}
	
	void pop_front() {
		delete _stack.front();
		_stack.pop_front();
	}

	void pop_back() {
		delete _stack.back();
		_stack.pop_back();
	}

	void clear() {
		if (!_stack.empty()) {
			for (Operations::iterator i = _stack.begin(); i != _stack.end(); ++i) {
				delete *i;
			}
			_stack.clear();
		}
	}

	// Allocate a new Operation to work with
	void start(const std::string& command) {
		if (_pending != NULL) {
			delete _pending;
		}

		_pending = new Operation(command);
	}

	// Finish the current undo operation
	bool finish(const std::string& command) {
		if (_pending != NULL) {
			delete _pending;
			_pending = NULL;
			return false;
		}
		else {
			// Rename the last undo operation (it was "unnamed" till now)
			ASSERT_MESSAGE(!_stack.empty(), "undo stack empty");
			_stack.back()->_command = command;
			return true;
		}
	}

	// Store an Undoable into the last snapshot
	void save(Undoable* undoable) {
		// Check, if there is still a pending undo command around
		if (_pending != NULL) {
			// Save the pending undo command
			_stack.push_back(_pending);
			_pending = NULL;
		}

		// Save the UndoMemento of the most recently added command into the snapshot
		back()->_snapshot.save(undoable);
	}

}; // class UndoStack

} // namespace undo

#endif /*UNDOSTACK_H_*/
