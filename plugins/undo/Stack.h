#pragma once

#include "debugging/debugging.h"

namespace undo
{

/* greebo: This is some kind of Stack wrapper keepign track
 * of all the Undo Operations.
 *
 * When start() is called, a new Operation is allocated  and 
 * on calling save(Undoable*) the Undoable is actually stored within
 * the allocated Operation. The method finish() deallocates the
 * memory used in this timespan.
 */
class UndoStack
{
	//! Note: using std::list instead of vector/deque, to avoid copying of undos
	typedef std::list<OperationPtr> Operations;

	// The list of Operations that can be undone
	Operations _stack;

	// The pending undo operation (a working variable, so to say)
	OperationPtr _pending;

public:

	bool empty() const
	{
		return _stack.empty();
	}

	std::size_t size() const
	{
		return _stack.size();
	}

	const OperationPtr& back() const
	{
		return _stack.back();
	}

	const OperationPtr& front() const 
	{
		return _stack.front();
	}

	void pop_front()
	{
		_stack.pop_front();
	}

	void pop_back()
	{
		_stack.pop_back();
	}

	void clear()
	{
		_stack.clear();
	}

	// Allocate a new Operation to work with
	void start(const std::string& command)
	{
		if (_pending)
		{
			_pending.reset();
		}

		_pending.reset(new Operation(command));
	}

	// Finish the current undo operation
	bool finish(const std::string& command)
	{
		if (_pending)
		{
			_pending.reset();
			return false;
		}
		else 
		{
			// Rename the last undo operation (it was "unnamed" till now)
			ASSERT_MESSAGE(!_stack.empty(), "undo stack empty");
			_stack.back()->setName(command);
			return true;
		}
	}

	// Store an Undoable into the last snapshot
	void save(IUndoable& undoable)
	{
		// Check, if there is still a pending undo command around
		if (_pending)
		{
			// Save the pending undo command
			_stack.push_back(_pending);
			_pending.reset();
		}

		// Save the UndoMemento of the most recently added command into the snapshot
		back()->save(undoable);
	}

}; // class UndoStack

} // namespace undo
