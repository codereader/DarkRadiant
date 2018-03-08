#pragma once

#include "ientity.h"
#include "ObservedUndoable.h"
#include "string/string.h"
#include <vector>
#include <sigc++/connection.h>
#include <sigc++/trackable.h>

namespace entity
{

/// \brief A key/value pair of strings.
///
/// - Notifies observers when value changes - value changes to "" on destruction.
/// - Provides undo support through the global undo system.
class KeyValue :
	public EntityKeyValue,
	public sigc::trackable
{
private:
	typedef std::vector<KeyObserver*> KeyObservers;
	KeyObservers _observers;

	std::string _value;
	std::string _emptyValue;
	undo::ObservedUndoable<std::string> _undo;
	sigc::connection _undoHandler;
	sigc::connection _redoHandler;

public:
	KeyValue(const std::string& value, const std::string& empty);

	~KeyValue();

    void connectUndoSystem(IMapFileChangeTracker& changeTracker);
    void disconnectUndoSystem(IMapFileChangeTracker& changeTracker);

	void attach(KeyObserver& observer);
	void detach(KeyObserver& observer);

	// Accessor method, retrieve the actual value
	const std::string& get() const;

	void assign(const std::string& other);

	void notify();

	void importState(const std::string& string);

	// NameObserver implementation
	void onNameChange(const std::string& oldName, const std::string& newName);

private:
	// Gets called after a undo/redo operation is fully completed.
	// This triggers a keyobserver refresh, to allow for reconnection to Namespaces and such.
	void onUndoRedoOperationFinished();
};

} // namespace entity
