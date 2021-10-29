#pragma once

/// \file
/// \brief The undo-system interface. Uses the 'memento' pattern.

#include "imodule.h"
#include "imap.h"
#include <cstddef>
#include <memory>
#include <sigc++/signal.h>

/** 
 * greebo: An UndoMemento has to be allocated on the heap
 * and contains all the information that is needed to describe
 * the status of an Undoable.
 */
class IUndoMemento
{
public:
    virtual ~IUndoMemento() {}
};
typedef std::shared_ptr<IUndoMemento> IUndoMementoPtr;

/* greebo: This is the abstract base class for an Undoable object.
 * Derive from this class if your instance/object should be Undoable.
 *
 * The exportState method has to allocate and return a new UndoMemento 
 * with all the necessary data to restore the current state.
 *
 * The importState() method should re-import the values saved in the
 * UndoMemento
 */
class IUndoable
{
public:
    virtual ~IUndoable() {}
	virtual IUndoMementoPtr exportState() const = 0;
	virtual void importState(const IUndoMementoPtr& state) = 0;

    // Optional method that is invoked after the whole snapshot has been restored,
    // applicable to both undo or redo operations.
    // May be used by Undoable objects to perform a post-undo cleanup.
    virtual void onOperationRestored()
    {}
};

/**
 * Undoables request their associated StateSaver to save their current state.
 * The state saver might call the Undoable's exportState() method or not,
 * depending on whether the Undoable has already been saved during 
 * the current operation's lifetime.
 * To acquire an UndoStateSaver, use UndoSystem::getStateSaver().
 */
class IUndoStateSaver
{
public:
    virtual ~IUndoStateSaver() {}

    // Request a state save of the associated IUndoable
	virtual void saveState() = 0;

    // Returns the undo system this saver is associated to
    virtual IUndoSystem& getUndoSystem() = 0;
};

class IUndoSystem
{
public:
    using Ptr = std::shared_ptr<IUndoSystem>;

	// Undoable objects need to call this to get hold of a StateSaver instance
	// which will take care of exporting and saving the state.
    virtual IUndoStateSaver* getStateSaver(IUndoable& undoable) = 0;
	virtual void releaseStateSaver(IUndoable& undoable) = 0;

	virtual void start() = 0;
	virtual void finish(const std::string& command) = 0;
	virtual void undo() = 0;
	virtual void redo() = 0;
	virtual void clear() = 0;

	// Returns true if an operation is already started
	virtual bool operationStarted() const = 0;

	// greebo: This finishes the current operation and removes
	// it immediately from the stack, therefore it never existed.
	virtual void cancel() = 0;

    enum class EventType
    {
        OperationRecorded,
        OperationUndone,
        OperationRedone,
        AllOperationsCleared,
    };

    /**
     * Emitted on edit/undo/redo and clear events, passes the operation type and name
     * as arguments. Except for AllOperationsCleared, which will have an empty name argument.
     */
    virtual sigc::signal<void(EventType, const std::string&)>& signal_undoEvent() = 0;
};

class IUndoSystemFactory :
    public RegisterableModule
{
public:
    virtual ~IUndoSystemFactory() {}

    // Create a new UndoSystem instance for use in a map root node
    virtual IUndoSystem::Ptr createUndoSystem() = 0;
};

constexpr const char* const MODULE_UNDOSYSTEM_FACTORY("UndoSystemFactory");

inline IUndoSystemFactory& GlobalUndoSystemFactory()
{
    static module::InstanceReference<IUndoSystemFactory> _reference(MODULE_UNDOSYSTEM_FACTORY);
    return _reference;
}

// The accessor function to the main map's undo system
inline IUndoSystem& GlobalUndoSystem()
{
    return GlobalMapModule().getUndoSystem();
}

class UndoableCommand
{
	const std::string _command;
	bool _shouldFinish;
public:

	UndoableCommand(const std::string& command) :
		_command(command),
		_shouldFinish(false)
	{
		// Avoid double-starting undo operations
		if (!GlobalUndoSystem().operationStarted())
		{
			GlobalUndoSystem().start();
			_shouldFinish = true;
		}
	}

	~UndoableCommand()
	{
		if (_shouldFinish)
		{
			GlobalUndoSystem().finish(_command);
		}
	}
};
