#ifndef DOOM3ENTITY_H_
#define DOOM3ENTITY_H_

#include <vector>
#include "KeyValue.h"
#include <boost/shared_ptr.hpp>

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
	IEntityClassConstPtr _eclass;

	typedef boost::shared_ptr<KeyValue> KeyValuePtr;
	
	// A key value pair using a dynamically allocated value
	typedef std::pair<std::string, KeyValuePtr> KeyValuePair;
	
	// The unsorted list of KeyValue pairs
	typedef std::vector<KeyValuePair> KeyValues;
	KeyValues _keyValues;

	typedef std::vector<Observer*> Observers;
	Observers _observers;

	ObservedUndoableObject<KeyValues> _undo;
	bool _instanced;

	bool _observerMutex;

	bool _isContainer;

public:
	// Constructor, pass the according entity class
	Doom3Entity(const IEntityClassConstPtr& eclass);
	
	// Copy constructor
	Doom3Entity(const Doom3Entity& other);
	
	void importState(const KeyValues& keyValues);
	typedef MemberCaller1<Doom3Entity, const KeyValues&, &Doom3Entity::importState> UndoImportCaller;

    /* Entity implementation */
	void attachObserver(Observer* observer);
	void detachObserver(Observer* observer);

	void instanceAttach(MapFile* map);
	void instanceDetach(MapFile* map);

	/** Return the EntityClass associated with this entity.
	 */
	IEntityClassConstPtr getEntityClass() const;

	void forEachKeyValue(Visitor& visitor) const;
	void forEachKeyValue(KeyValueVisitor& visitor);

	/** Set a keyvalue on the entity.
	 */
	void setKeyValue(const std::string& key, const std::string& value);

	/** Retrieve a keyvalue from the entity.
	 */
	std::string getKeyValue(const std::string& key) const;

	// Returns true if the given key is inherited
	bool isInherited(const std::string& key) const;

	// Get all KeyValues matching the given prefix.
	KeyValuePairs getKeyValuePairs(const std::string& prefix) const;

	bool isContainer() const;
	void setIsContainer(bool isContainer);
	
	bool isModel() const;

private:

    // Notification functions
	void notifyInsert(const std::string& key, KeyValue& value);
    void notifyChange(const std::string& k, const std::string& v);
	void notifyErase(const std::string& key, KeyValue& value);

	void insert(const std::string& key, const KeyValuePtr& keyValue);

	void insert(const std::string& key, const std::string& value);

	void erase(KeyValues::iterator i);

	void erase(const std::string& key);
	
	KeyValues::iterator find(const std::string& key);
	KeyValues::const_iterator find(const std::string& key) const;
	
	void forEachKeyValue_instanceAttach(MapFile* map);
	void forEachKeyValue_instanceDetach(MapFile* map);
};

} // namespace entity

#endif /*DOOM3ENTITY_H_*/
