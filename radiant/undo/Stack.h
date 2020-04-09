#pragma once

#include "debugging/debugging.h"
#include <list>
#include "Operation.h"

namespace undo
{

/** 
 * greebo: The UndoSystem keeps track of Undoable and Redoable operations,
 * which are kept in a chain-like data structure.
 *
 * Each named operation in this stack contains a snapshot of the undoable objects
 * that have been touched between start() and finish().
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

	// The pending undo operation (will be committed as soon as the first
	// undoable saves its data to the stack)
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
		// When starting an operation, we create one and declare it as pending
		// It will not be added to the stack, it still might end up empty
		// We will also replace any previously pending operation with the new one
		// even though it should not happen by design
		ASSERT_MESSAGE(!_pending, "undo operation already started");

		_pending = std::make_shared<Operation>(command);
	}

	// Finish the current undo operation
	bool finish(const std::string& command)
	{
		if (_pending)
		{
			// The started operation has not been filled with any data
			// so just discard it without doing anything
			_pending.reset();
			return false;
		}
		
		// Reaching this point we don't have a *pending* operation
		// but we need to make sure we have *any* operation at all
		ASSERT_MESSAGE(!_stack.empty(), "undo stack empty");

		// Rename the last undo operation (it may be "unnamed" till now)
		_stack.back()->setName(command);
		return true;
	}

	// Store an Undoable into the last snapshot
	void save(IUndoable& undoable)
	{
		// Check, if there is still a pending undo operation waiting to be added to the stack
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
