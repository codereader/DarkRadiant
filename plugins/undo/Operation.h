#ifndef UNDOOPERATION_H_
#define UNDOOPERATION_H_

namespace undo {

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
	~Operation() {
		// Tell the snapshot to release all the memory of its UndoStates
		_snapshot.release();
	}

}; // class UndoOperation

} // namespace undo

#endif /*UNDOOPERATION_H_*/
