#pragma once

/// \file
/// \brief The undo-system interface. Uses the 'memento' pattern.

#include "imodule.h"
#include <cstddef>
#include <memory>
#include <sigc++/signal.h>

class IMapFileChangeTracker;

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
	virtual void save(IUndoable& undoable) = 0;
};

const char* const MODULE_UNDOSYSTEM("UndoSystem");

class IUndoSystem :
	public RegisterableModule
{
public:
	// Undoable objects need to call this to get hold of a StateSaver instance
	// which will take care of exporting and saving the state. The passed map file change 
    // tracker will be notified when the state is saved.
    virtual IUndoStateSaver* getStateSaver(IUndoable& undoable, IMapFileChangeTracker& tracker) = 0;
	virtual void releaseStateSaver(IUndoable& undoable) = 0;

	virtual std::size_t size() const = 0;
	virtual void start() = 0;
	virtual void finish(const std::string& command) = 0;
	virtual void undo() = 0;
	virtual void redo() = 0;
	virtual void clear() = 0;

	// Emitted after an undo operation is fully completed, allows objects to refresh their state
	virtual sigc::signal<void>& signal_postUndo() = 0;

	// Emitted after a redo operation is fully completed, allows objects to refresh their state
	virtual sigc::signal<void>& signal_postRedo() = 0;

	// greebo: This finishes the current operation and removes
	// it immediately from the stack, therefore it never existed.
	virtual void cancel() = 0;

    /**
    * Observer implementation which gets notified
    * on undo/redo perations.
    */
    class Tracker
    {
    public:
        virtual ~Tracker() {}

        virtual void clear() = 0;
        virtual void begin() = 0;
        virtual void undo() = 0;
        virtual void redo() = 0;
    };

	virtual void attachTracker(Tracker& tracker) = 0;
	virtual void detachTracker(Tracker& tracker) = 0;
};

// The accessor function
inline IUndoSystem& GlobalUndoSystem() 
{
	// Cache the reference locally
	static IUndoSystem& _undoSystem(
		*std::static_pointer_cast<IUndoSystem>(
			module::GlobalModuleRegistry().getModule(MODULE_UNDOSYSTEM)
		)
	);
	return _undoSystem;
}

class UndoableCommand
{
	const std::string _command;
public:

	UndoableCommand(const std::string& command) :
		_command(command)
	{
		GlobalUndoSystem().start();
	}

	~UndoableCommand() {
		GlobalUndoSystem().finish(_command);
	}
};
