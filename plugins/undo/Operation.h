#pragma once

#include <memory>

namespace undo
{

class Operation
{
public:
	// The Snapshot that (i.e. the list of Undoables and all their UndoMementos)
	Snapshot _snapshot;

	// The name of the UndoOperaton
	std::string _command;

	// Constructor
	Operation(const std::string& command) :
		_command(command)
	{}

	// Destructor
	~Operation()
	{
		// Tell the snapshot to release all the memory of its UndoStates
		_snapshot.release();
	}

};
typedef std::shared_ptr<Operation> OperationPtr;

} // namespace undo
