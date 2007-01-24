#ifndef STACKFILLER_H_
#define STACKFILLER_H_

namespace undo {

/* greebo: This Filler class actually just adds the Undoable to the 
 * contained stack. Don't know what good it is to write an own class for this...
 */
class UndoStackFiller : 
	public UndoObserver 
{
	UndoStack* _stack;
public:

	// Constructor
	UndoStackFiller()
		: _stack(NULL) 
	{}
	
	void save(Undoable* undoable) {
		ASSERT_NOTNULL(undoable);

		if (_stack != NULL) {
			_stack->save(undoable);
			_stack = NULL;
		}
	}

	// Assign the stack of this class
	void setStack(undo::UndoStack* stack) {
		_stack = stack;
	}
};

} // namespace undo

#endif /*STACKFILLER_H_*/
