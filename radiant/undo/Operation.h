#pragma once

#include <memory>

namespace undo
{

class Operation
{
private:
	// The Snapshot that (i.e. the list of Undoables and all their UndoMementos)
	Snapshot _snapshot;

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
		_snapshot.save(undoable);
	}

	void restoreSnapshot()
	{
		_snapshot.restore();
	}
};
typedef std::shared_ptr<Operation> OperationPtr;

} // namespace undo
