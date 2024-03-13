#pragma once

#include "ientity.h"
#include "ObservedUndoable.h"
#include "string/string.h"
#include <vector>

class SpawnArgs;

/**
 * @brief Object representing a single keyvalue (spawnarg) on an entity.
 *
 * This class exists so that each spawnarg can have its own independent set of
 * KeyObservers responding to changes in its value. For most purposes it is
 * simpler to use Entity::Observer::onKeyChange, Entity::setKeyValue and
 * Entity::getKeyValue to interact with key values.
 */
class EntityKeyValue final: public NameObserver
{
    typedef std::vector<KeyObserver*> KeyObservers;
    KeyObservers _observers;

    std::string _value;
    std::string _emptyValue;
    undo::ObservedUndoable<std::string> _undo;

    // This is a specialised callback pointing to the owning SpawnArgs
    std::function<void(const std::string&)> _valueChanged;

public:
    EntityKeyValue(const std::string& value, const std::string& empty, const std::function<void(const std::string&)>& valueChanged);

    EntityKeyValue(const EntityKeyValue& other) = delete;
    EntityKeyValue& operator=(const EntityKeyValue& other) = delete;

    ~EntityKeyValue();

    void connectUndoSystem(IUndoSystem& undoSystem);
    void disconnectUndoSystem(IUndoSystem& undoSystem);

    /// Attaches a callback to get notified about the key change.
    void attach(KeyObserver& observer);

    /**
     * @brief Detach the given observer from this key value.
     *
     * @param observer
     * Observer to detach. No action will be taken if this observer is not
     * already attached.
     *
     * @param sendEmptyValue
     * If true (the default), the observer will be invoked with an empty value
     * before being detached. If false, no final value will be sent.
     */
    void detach(KeyObserver& observer, bool sendEmptyValue = true);

    /// Retrieves the actual value of this key
    const std::string& get() const;

    /// Sets the value of this key
    void assign(const std::string& other);

    void notify();

    void importState(const std::string& string);

    // NameObserver implementation
    void onNameChange(const std::string& oldName, const std::string& newName) override;

private:
    // Gets called after a undo/redo operation is fully completed.
    // This triggers a keyobserver refresh, to allow for reconnection to Namespaces and such.
    void onUndoRedoOperationFinished();
};

using EntityKeyValuePtr = std::shared_ptr<EntityKeyValue>;
