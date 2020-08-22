#pragma once

#include <vector>
#include "KeyValue.h"
#include <memory>

/** greebo: This is the implementation of the class Entity.
 *
 * A Doom3Entity basically just keeps track of all the
 * spawnargs and delivers them on request, taking the
 * inheritance tree (EntityClasses) into account.
 *
 * It's possible to attach observers to this entity to get
 * notified upon key/value changes.
 */
namespace entity {

/// \brief An unsorted list of key/value pairs.
///
/// - Notifies observers when a pair is inserted or removed.
/// - Provides undo support through the global undo system.
/// - New keys are appended to the end of the list.
class Doom3Entity :
	public Entity
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
	bool _instanced;

	bool _observerMutex;

	bool _isContainer;

public:
	// Constructor, pass the according entity class
	Doom3Entity(const IEntityClassPtr& eclass);

	// Copy constructor
	Doom3Entity(const Doom3Entity& other);

	void importState(const KeyValues& keyValues);

    /* Entity implementation */
	void attachObserver(Observer* observer) override;
	void detachObserver(Observer* observer) override;

	void connectUndoSystem(IMapFileChangeTracker& changeTracker);
    void disconnectUndoSystem(IMapFileChangeTracker& changeTracker);

	/** Return the EntityClass associated with this entity.
	 */
	IEntityClassPtr getEntityClass() const override;

	void forEachKeyValue(const KeyValueVisitFunctor& func) const override;
    void forEachEntityKeyValue(const EntityKeyValueVisitFunctor& visitor) override;

	/** Set a keyvalue on the entity.
	 */
	void setKeyValue(const std::string& key, const std::string& value) override;

	/** Retrieve a keyvalue from the entity.
	 */
	std::string getKeyValue(const std::string& key) const override;

	// Returns true if the given key is inherited
	bool isInherited(const std::string& key) const override;

	// Get all KeyValues matching the given prefix.
	KeyValuePairs getKeyValuePairs(const std::string& prefix) const override;

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
