#pragma once

#include "ientity.h"
#include "ObservedUndoable.h"
#include "string/string.h"
#include <vector>

namespace entity {

/// \brief A key/value pair of strings.
///
/// - Notifies observers when value changes - value changes to "" on destruction.
/// - Provides undo support through the global undo system.
class KeyValue :
	public EntityKeyValue,
	public UndoSystem::Observer
{
	typedef std::vector<KeyObserver*> KeyObservers;
	KeyObservers _observers;

	std::string _value;
	std::string _emptyValue;
	undo::ObservedUndoable<std::string> _undo;

public:
	KeyValue(const std::string& value, const std::string& empty);

	~KeyValue();

	void instanceAttach(MapFile* map);
	void instanceDetach(MapFile* map);

	void attach(KeyObserver& observer);
	void detach(KeyObserver& observer);

	// Accessor method, retrieve the actual value
	const std::string& get() const;

	void assign(const std::string& other);

	void notify();

	void importState(const std::string& string);

	// NameObserver implementation
	void onNameChange(const std::string& oldName, const std::string& newName);

	// Gets called after a undo/redo operation is fully completed.
	// This triggers a keyobserver refresh, to allow for reconnection to Namespaces and such.
	void postUndo();
	void postRedo();
};

} // namespace entity
