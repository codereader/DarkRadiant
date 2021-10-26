#pragma once

#include "AttachmentData.h"

#include <vector>
#include "KeyValue.h"
#include <memory>

class IUndoSystem;

namespace entity {

/**
 * \brief Implementation of the class Entity.
 *
 * A SpawnArgs basically just keeps track of all the spawnargs and delivers
 * them on request, taking the inheritance tree (EntityClasses) into account.
 * The actual rendering and entity behaviour is handled by the EntityNode.
 *
 * It's possible to attach observers to this entity to get notified upon
 * key/value changes.
 */
class SpawnArgs: public Entity
{
	IEntityClassPtr _eclass;

	typedef std::shared_ptr<KeyValue> KeyValuePtr;

	// A key value pair using a dynamically allocated value
	typedef std::pair<std::string, KeyValuePtr> KeyValuePair;

	// The unsorted list of KeyValue pairs
	typedef std::vector<KeyValuePair> KeyValues;
	KeyValues _keyValues;

	typedef std::set<Observer*> Observers;
	Observers _observers;

	undo::ObservedUndoable<KeyValues> _undo;

	bool _observerMutex;

	bool _isContainer;

    // Store attachment information
    AttachmentData _attachments;

public:
	// Constructor, pass the according entity class
	SpawnArgs(const IEntityClassPtr& eclass);

	// Copy constructor
	SpawnArgs(const SpawnArgs& other);

	void importState(const KeyValues& keyValues);

    /* Entity implementation */
	void attachObserver(Observer* observer) override;
	void detachObserver(Observer* observer) override;
	void connectUndoSystem(IUndoSystem& undoSystem);
    void disconnectUndoSystem(IUndoSystem& undoSystem);
	IEntityClassPtr getEntityClass() const override;
    void forEachKeyValue(KeyValueVisitFunc func,
                         bool includeInherited) const override;
    void forEachEntityKeyValue(const EntityKeyValueVisitFunctor& visitor) override;
	void setKeyValue(const std::string& key, const std::string& value) override;
	std::string getKeyValue(const std::string& key) const override;
	bool isInherited(const std::string& key) const override;
    void forEachAttachment(AttachmentFunc func) const override;

	bool isWorldspawn() const override;
	bool isContainer() const override;
	void setIsContainer(bool isContainer);

	bool isModel() const override;

	// Returns the actual pointer to a KeyValue (or NULL if not found),
	// not just the string like getKeyValue() does.
	// Only returns non-NULL for non-inherited keyvalues.
	EntityKeyValuePtr getEntityKeyValue(const std::string& key);

	bool isOfType(const std::string& className) override;

private:

    // Parse attachment information from def_attach and related keys (which are
    // most likely on the entity class, not the entity itself)
    void parseAttachments();

    // Notification functions
	void notifyInsert(const std::string& key, KeyValue& value);
    void notifyChange(const std::string& k, const std::string& v);
	void notifyErase(const std::string& key, KeyValue& value);

	void insert(const std::string& key, const KeyValuePtr& keyValue);
	void insert(const std::string& key, const std::string& value);

	void erase(const KeyValues::iterator& i);
	void erase(const std::string& key);

	KeyValues::iterator find(const std::string& key);
	KeyValues::const_iterator find(const std::string& key) const;
};

} // namespace entity
