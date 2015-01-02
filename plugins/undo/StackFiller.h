#pragma once

#include "iundo.h"
#include "mapfile.h"
#include "Stack.h"

namespace undo 
{

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

    IMapFileChangeTracker* _tracker;

public:

	// Constructor
	UndoStackFiller() : 
		_stack(nullptr),
        _tracker(nullptr)
	{}

    UndoStackFiller(IMapFileChangeTracker& tracker) :
        _stack(nullptr),
        _tracker(&tracker)
    {}

	void save(IUndoable& undoable)
	{
        if (_stack != nullptr)
		{
            // Optionally notify the change tracker
            if (_tracker != nullptr)
            {
                _tracker->changed();
            }

            // Export the Undoable's memento
			_stack->save(undoable);

            // Make sure the stack is dissociated after saving
            // to make sure further save() calls don't have any effect
            _stack = nullptr;
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
