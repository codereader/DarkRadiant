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
 * On start() a new Operation is allocated  and on save(IUndoable) 
 * the IUndoable's memento is actually stored within the allocated Operation. 
 * The method finish() commits the operation to the stack.
 */
class UndoStack
{
private:
	// The list of Operations that can be undone
	//! Note: using std::list instead of vector/deque, to avoid copying of undos
    std::list<Operation::Ptr> _stack;

	// The pending undo operation (will be committed on finish, if not empty)
    Operation::Ptr _pending;

public:

	bool empty() const
	{
		return _stack.empty();
	}

	std::size_t size() const
	{
		return _stack.size();
	}

	const Operation::Ptr& back() const
	{
		return _stack.back();
	}

	const Operation::Ptr& front() const
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
		if (!_pending || _pending->empty())
		{
			// The started operation has not been filled with any data
			// so just discard it without doing anything
			_pending.reset();
			return false;
		}
		
		// Rename the last undo operation (it may be "unnamed" till now)
        _pending->setName(command);

        // Move the pending operation into its place
        _stack.emplace_back(std::move(_pending));
		return true;
	}

    // Store an Undoable's state into the active operation
    void save(IUndoable& undoable)
    {
        assert(_pending);
        _pending->save(undoable);
    }
};

} // namespace
