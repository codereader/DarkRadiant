#pragma once

#include <map>
#include <set>
#include <functional>

#include "iundo.h"
#include "icommandsystem.h"

#include "Stack.h"
#include "StackFiller.h"
#include "registry/CachedKey.h"

namespace undo
{

constexpr const char* const RKEY_UNDO_QUEUE_SIZE = "user/ui/undo/queueSize";

/**
* greebo: The UndoSystem (interface: iundo.h) is maintaining two internal
* stacks of Operations (one for Undo, one for Redo), each containing a list
* of Undoables plus their snapshot data.
*
* The Undoables are responsible of submitting their data to the UndoSystem
* before their data is changed, not knowing which or whether an operation is 
* currently active. If there actually is an Undo Operation in the works,
* this system is handling all of the paperwork.
*
* On undo or redo, the Undoables are called to re-import the states
* as stored in their UndoMementos. When undoing operations, the Undoables
* themselves are again responsible for submitting their data to the UndoSystem
* such that it can record the changes and store them in the RedoStack.
*
* The RedoStack is discarded as soon as a new Undoable Operation is recorded
* and pushed to the UndoStack.
*/
class UndoSystem final :
	public IUndoSystem
{
private:
	// The undo and redo stacks
	UndoStack _undoStack;
	UndoStack _redoStack;

	UndoStack* _activeUndoStack;

	std::map<IUndoable*, UndoStackFiller> _undoables;

    registry::CachedKey<std::size_t> _undoLevels;

    sigc::signal<void(EventType, const std::string&)> _eventSignal;

public:
	UndoSystem();
	~UndoSystem();

	IUndoStateSaver* getStateSaver(IUndoable& undoable) override;
	void releaseStateSaver(IUndoable& undoable) override;

	void start() override;
	void cancel() override;
	void finish(const std::string& command) override;

	bool operationStarted() const override;

	void undo() override;
	void redo() override;

	void clear() override;

    sigc::signal<void(EventType, const std::string&)>& signal_undoEvent() override;

private:
	void startUndo();
	bool finishUndo(const std::string& command);

	void startRedo();
	bool finishRedo(const std::string& command);

	// Assigns the given stack to all of the Undoables listed in the map
	void setActiveUndoStack(UndoStack* stack);
};

}
