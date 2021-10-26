#pragma once

#include "iundo.h"
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
class UndoStackFiller final :
	public IUndoStateSaver
{
private:
    IUndoSystem& _owner;
	UndoStack* _stack;

public:
    UndoStackFiller(IUndoSystem& owner) :
        _owner(owner),
        _stack(nullptr)
    {}

    // Noncopyable
    UndoStackFiller(const UndoStackFiller& other) = delete;
    UndoStackFiller& operator=(const UndoStackFiller& other) = delete;

    void save(IUndoable& undoable)
    {
        if (_stack != nullptr)
        {
            // Export the Undoable's memento
            _stack->save(undoable);

            // Make sure the stack is dissociated after saving
            // to make sure further save() calls don't have any effect
            _stack = nullptr;
        }
    }

    IUndoSystem& getUndoSystem() override
    {
        return _owner;
    }

	// Assign the stack of this class. This usually happens when starting
	// an undo or redo operation.
	void setStack(UndoStack* stack)
	{
		_stack = stack;
	}
};

} // namespace undo
