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
		// Constructor
		UndoableState(IUndoable& undoable) :
			_undoable(undoable),
			_data(_undoable.exportState())
		{}

		void restoreState()
		{
			_undoable.importState(_data);
		}
	};

	// The Snapshot (the list of structs containing Undoable+Data)
	std::list<UndoableState> _snapshot;

	// The name of the UndoOperaton
	std::string _command;

public:
	// Constructor
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

	void save(IUndoable& undoable)
	{
		// Record the state of the given undable and push it to the snapshot
		// The order is relevant, we use push_front()
		_snapshot.push_front(UndoableState(undoable));
	}

	void restoreSnapshot()
	{
		for (auto& undoablePlusMemento : _snapshot)
		{
			undoablePlusMemento.restoreState();
		}
	}
};
typedef std::shared_ptr<Operation> OperationPtr;

} // namespace undo
