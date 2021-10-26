#pragma once

#include "iundo.h"

#include <list>
#include <memory>
#include <string>

namespace undo
{

/**
 * A named Undo/Redo Operation summarises the state
 * of all the undoable objects that have been touched
 * between start() and finish().
 */
class Operation
{
private:
	// This holds the Undoable reference and its exported data
	// The data is recorded right in the constructor
	class UndoableState
	{
	private:
		IUndoable& _undoable;
		IUndoMementoPtr _data;

	public:
        UndoableState(IUndoable& undoable) :
            _undoable(undoable),
            _data(_undoable.exportState())
        {}

        // Noncopyable
        UndoableState(const UndoableState& other) = delete;
        UndoableState& operator=(const UndoableState& other) = delete;

		void restore()
		{
			_undoable.importState(_data);
		}
	};

	// The Snapshot (the list of structs containing Undoable+Data)
	std::list<UndoableState> _snapshot;

	// The name of the UndoOperaton
	std::string _command;

public:
    using Ptr = std::shared_ptr<Operation>;

	Operation(const std::string& command) :
		_command(command)
	{}

	const std::string& getName() const
	{
		return _command;
	}

	void setName(const std::string& name)
	{
		_command = name;
	}

    bool empty() const
    {
        return _snapshot.empty();
    }

	void save(IUndoable& undoable)
	{
		// Record the state of the given undable and push it to the snapshot
		// The order is relevant, we add to the front
		_snapshot.emplace_front(undoable);
	}

	void restoreSnapshot()
	{
        // Walk through the snapshot front-to-back, the most recently added one is at the front
		for (auto& state : _snapshot)
		{
            state.restore();
		}
	}
};

} // namespace
