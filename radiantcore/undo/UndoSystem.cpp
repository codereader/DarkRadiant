#include "UndoSystem.h"

#include "itextstream.h"
#include "iscenegraph.h"

#include <iostream>

#include "Operation.h"
#include "StackFiller.h"

namespace undo 
{

UndoSystem::UndoSystem() :
	_activeUndoStack(nullptr),
	_undoLevels(RKEY_UNDO_QUEUE_SIZE)
{}

UndoSystem::~UndoSystem()
{
	clear();
}

IUndoStateSaver* UndoSystem::getStateSaver(IUndoable& undoable)
{
    auto result = _undoables.try_emplace(&undoable, *this, undoable);

	// If we're in the middle of an active undo operation, assign this to the tracker (#4861)
	if (_activeUndoStack != nullptr)
	{
		result.first->second.setStack(_activeUndoStack);
	}

    return &(result.first->second);
}

void UndoSystem::releaseStateSaver(IUndoable& undoable)
{
	_undoables.erase(&undoable);
}

void UndoSystem::start()
{
	_redoStack.clear();
	if (_undoStack.size() == _undoLevels.get())
	{
		_undoStack.pop_front();
	}
	startUndo();
}

bool UndoSystem::operationStarted() const
{
	return _activeUndoStack != nullptr;
}

void UndoSystem::cancel()
{
	// Try to add the last operation as "temp"
	if (finishUndo("$TEMPORARY"))
	{
		// Instantly remove the added operation
		_undoStack.pop_back();
	}
}

void UndoSystem::finish(const std::string& command)
{
	if (finishUndo(command))
    {
		rMessage() << command << std::endl;
        for (auto tracker : _trackers) { tracker->onOperationRecorded(command); }
	}
}

void UndoSystem::undo()
{
	if (_undoStack.empty())
	{
		rMessage() << "Undo: no undo available" << std::endl;
		return;
	}

    if (operationStarted())
    {
        rWarning() << "Undo not available while an operation is still in progress" << std::endl;
        return;
    }
		
	const auto& operation = _undoStack.back();
    auto operationName = operation->getName(); // copy this name, we need it after op destruction
	rMessage() << "Undo: " << operationName << std::endl;

	startRedo();
	operation->restoreSnapshot();
	finishRedo(operationName);
	_undoStack.pop_back();
    for (auto tracker : _trackers) { tracker->onOperationUndone(operationName); }

	_signalPostUndo.emit();

	// Trigger the onPostUndo event on all scene nodes
	GlobalSceneGraph().foreachNode([&] (const scene::INodePtr& node)->bool
	{
		node->onPostUndo();
		return true;
	});

	GlobalSceneGraph().sceneChanged();
}

void UndoSystem::redo()
{
	if (_redoStack.empty())
	{
		rMessage() << "Redo: no redo available" << std::endl;
		return;
	}

    if (operationStarted())
    {
        rWarning() << "Redo not available while an operation is still in progress" << std::endl;
        return;
    }
		
	const auto& operation = _redoStack.back();
    auto operationName = operation->getName(); // copy this name, we need it after op destruction
	rMessage() << "Redo: " << operationName << std::endl;

	startUndo();
	operation->restoreSnapshot();
	finishUndo(operationName);
	_redoStack.pop_back();
    for (auto tracker : _trackers) { tracker->onOperationRedone(operationName); }

	_signalPostRedo.emit();

	// Trigger the onPostRedo event on all scene nodes
	GlobalSceneGraph().foreachNode([&] (const scene::INodePtr& node)->bool
	{
		node->onPostRedo();
		return true;
	});

	GlobalSceneGraph().sceneChanged();
}

void UndoSystem::clear()
{
	setActiveUndoStack(nullptr);
	_undoStack.clear();
	_redoStack.clear();
    for (auto tracker : _trackers) { tracker->onAllOperationsCleared(); }

	// greebo: This is called on map shutdown, so don't clear the observers,
	// there are some "persistent" observers like EntityInspector and ShaderClipboard
}

sigc::signal<void>& UndoSystem::signal_postUndo()
{
	return _signalPostUndo;
}

// Emitted after a redo operation is fully completed, allows objects to refresh their state
sigc::signal<void>& UndoSystem::signal_postRedo()
{
	return _signalPostRedo;
}

void UndoSystem::attachTracker(Tracker& tracker)
{
	ASSERT_MESSAGE(_trackers.count(&tracker) == 0, "undo tracker already attached");
	_trackers.insert(&tracker);
}

void UndoSystem::detachTracker(Tracker& tracker)
{
	ASSERT_MESSAGE(_trackers.count(&tracker) > 0, "undo tracker cannot be detached");
	_trackers.erase(&tracker);
}

void UndoSystem::startUndo()
{
	_undoStack.start("unnamedCommand");
	setActiveUndoStack(&_undoStack);
}

bool UndoSystem::finishUndo(const std::string& command)
{
	bool changed = _undoStack.finish(command);
	setActiveUndoStack(nullptr);
	return changed;
}

void UndoSystem::startRedo()
{
	_redoStack.start("unnamedCommand");
	setActiveUndoStack(&_redoStack);
}

bool UndoSystem::finishRedo(const std::string& command)
{
	bool changed = _redoStack.finish(command);
	setActiveUndoStack(nullptr);
	return changed;
}

// Assigns the given stack to all of the Undoables listed in the map
void UndoSystem::setActiveUndoStack(UndoStack* stack)
{
	_activeUndoStack = stack;

	for (auto& pair : _undoables)
	{
		pair.second.setStack(_activeUndoStack);
	}
}

} // namespace undo
