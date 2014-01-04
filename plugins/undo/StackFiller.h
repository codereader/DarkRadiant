#pragma once

namespace undo {

/**
 * greebo: This class acts as some sort of "duplication guard".
 * Undoable objects like brushes and patches will save their state
 * by calling the save() method - to ensure Undoables don't submit
 * their state more than once, the associated UndoStack reference 
 * is cleared after submission in the save() routine. Further calls
 * to save() will not have any effect. The stack reference is set
 * by the UndoSystem on start of an undo or redo operation.
 */
class UndoStackFiller :
	public IUndoStateSaver
{
	UndoStack* _stack;
public:

	// Constructor
	UndoStackFiller() : 
		_stack(NULL)
	{}

	void save(IUndoable& undoable)
	{
		if (_stack != NULL)
		{
			// Make sure the stack is dissociated after saving
			// to make sure further save() calls don't have any effect
			_stack->save(undoable);
			_stack = NULL;
		}
	}

	// Assign the stack of this class. This usually happens when starting
	// an undo or redo operation.
	void setStack(UndoStack* stack)
	{
		_stack = stack;
	}
};

} // namespace undo
