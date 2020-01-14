#pragma once

#include <map>
#include <set>
#include <functional>

#include "iundo.h"
#include "icommandsystem.h"
#include "imap.h"

#include "Stack.h"
#include "StackFiller.h"

namespace undo
{

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
class UndoSystem :
	public IUndoSystem
{
private:
	// The undo and redo stacks
	UndoStack _undoStack;
	UndoStack _redoStack;

	UndoStack* _activeUndoStack;

	typedef std::map<IUndoable*, UndoStackFiller> UndoablesMap;
	UndoablesMap _undoables;

	std::size_t _undoLevels;

	typedef std::set<Tracker*> Trackers;
	Trackers _trackers;

	sigc::signal<void> _signalPostUndo;
	sigc::signal<void> _signalPostRedo;

public:
	// Constructor
	UndoSystem();

	virtual ~UndoSystem();

	IUndoStateSaver* getStateSaver(IUndoable& undoable, IMapFileChangeTracker& tracker) override;

	void releaseStateSaver(IUndoable& undoable) override;

	std::size_t size() const override;

	void start() override;

	// greebo: This finishes the current operation and
	// removes it instantly from the stack
	void cancel() override;

	void finish(const std::string& command) override;

	void undo() override;
	void redo() override;

	void clear() override;

	sigc::signal<void>& signal_postUndo() override;

	// Emitted after a redo operation is fully completed, allows objects to refresh their state
	sigc::signal<void>& signal_postRedo() override;

	void attachTracker(Tracker& tracker) override;
	void detachTracker(Tracker& tracker) override;

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;

private:
	// This is connected to the CommandSystem
	void undoCmd(const cmd::ArgumentList& args);

	// This is connected to the CommandSystem
	void redoCmd(const cmd::ArgumentList& args);

	// Gets called as soon as the observed registry key is changed
	void keyChanged();

	void onMapEvent(IMap::MapEvent ev);

	// Sets the size of the undoStack
	void setLevels(std::size_t levels);

	std::size_t getLevels() const;

	void startUndo();
	bool finishUndo(const std::string& command);

	void startRedo();
	bool finishRedo(const std::string& command);

	// Assigns the given stack to all of the Undoables listed in the map
	void setActiveUndoStack(UndoStack* stack);

	void foreachTracker(const std::function<void(Tracker&)>& functor) const;

	void trackersClear() const;
	void trackersBegin() const;
	void trackersUndo() const;
	void trackersRedo() const;

	void constructPreferences();
};

}
