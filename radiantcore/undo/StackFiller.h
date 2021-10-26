#pragma once

#include "iundo.h"
#include "Stack.h"

namespace undo 
{

/**
 * greebo: This class represents a one-shot state saver, only the first
 * call to saveState() will submit the IUndoable's state, subsequent calls
 * don't have any effect.
 * 
 * The associated UndoStack reference is set up right before an undo/redo
 * operation by the UndoSystem, and is cleared after submission in 
 * the saveState() routine.
 */
class UndoStackFiller final :
	public IUndoStateSaver
{
private:
    IUndoSystem& _owner;
    IUndoable& _undoable;
	UndoStack* _stack;

public:
    using Ptr = std::shared_ptr<UndoStackFiller>;

    UndoStackFiller(IUndoSystem& owner, IUndoable& undoable) :
        _owner(owner),
        _undoable(undoable),
        _stack(nullptr)
    {}

    // Noncopyable
    UndoStackFiller(const UndoStackFiller& other) = delete;
    UndoStackFiller& operator=(const UndoStackFiller& other) = delete;

    void saveState() override
    {
        if (_stack != nullptr)
        {
            // Export the Undoable's memento
            _stack->save(_undoable);

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
