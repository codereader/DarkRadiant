#pragma once

#include "ientity.h"
#include "ObservedUndoable.h"
#include "string/string.h"
#include <vector>

namespace entity
{

class SpawnArgs;

/// \brief
/// Represents the string value of an entity key.
///
/// - Notifies observers when value changes - value changes to "" on destruction.
/// - Provides undo support through the map's undo system.
class KeyValue final: public EntityKeyValue
{
private:
	typedef std::vector<KeyObserver*> KeyObservers;
	KeyObservers _observers;

	std::string _value;
	std::string _emptyValue;
	undo::ObservedUndoable<std::string> _undo;

    // This is a specialised callback pointing to the owning SpawnArgs
    std::function<void(const std::string&)> _valueChanged;

public:
	KeyValue(const std::string& value, const std::string& empty, const std::function<void(const std::string&)>& valueChanged);

    KeyValue(const KeyValue& other) = delete;
    KeyValue& operator=(const KeyValue& other) = delete;

	~KeyValue();

    void connectUndoSystem(IUndoSystem& undoSystem);
    void disconnectUndoSystem(IUndoSystem& undoSystem);

    // EntityKeyValue implementation
	void attach(KeyObserver& observer) override;
	void detach(KeyObserver& observer, bool sendEmptyValue) override;
	const std::string& get() const override;
	void assign(const std::string& other) override;

	void notify();

	void importState(const std::string& string);

	// NameObserver implementation
	void onNameChange(const std::string& oldName, const std::string& newName) override;

private:
    // Gets called after a undo/redo operation is fully completed.
    // This triggers a keyobserver refresh, to allow for reconnection to Namespaces and such.
    void onUndoRedoOperationFinished();
};

} // namespace entity
