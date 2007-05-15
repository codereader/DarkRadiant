#ifndef DOOM3ENTITY_H_
#define DOOM3ENTITY_H_

#include "string/pooledstring.h"
#include "KeyValue.h"
#include <boost/shared_ptr.hpp>

/** greebo: This is the implementation of the class Entity.
 * 
 * A Doom3Entity basically just keeps track of all the
 * spawnargs and delivers them on request, taking the 
 * inheritance tree (EntityClasses) into account.
 * 
 * It's possible to attach observers to this entity to get
 * notified upon key/value changes. This is currently implemented
 * by an UnsortedSet of Callback<const char*> and should be refactored.
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
	static EntityCreator::KeyValueChangedFunc m_entityKeyValueChanged;
	static Counter* m_counter;

	IEntityClassConstPtr m_eclass;

	// Begin deprecated
	/*class KeyContext {};
	typedef Static<StringPool, KeyContext> KeyPool;
	typedef PooledString<KeyPool> Key;
	typedef SmartPointer<KeyValue> KeyValuePtr;
	typedef UnsortedMap<Key, KeyValuePtr> KeyValues;*/
	// End deprecated
	
	typedef boost::shared_ptr<KeyValue> KeyValuePtr;
	typedef std::pair<std::string, KeyValuePtr> KeyValuePair;
	
	// The unsorted list of KeyValue Pairs
	typedef std::vector<KeyValuePair> KeyValues;
	
	KeyValues m_keyValues;

	typedef UnsortedSet<Observer*> Observers;
	Observers m_observers;

	ObservedUndoableObject<KeyValues> m_undo;
	bool m_instanced;

	bool m_observerMutex;

public:
	bool m_isContainer;

	// Constructor, pass the according entity class
	Doom3Entity(IEntityClassPtr eclass);
	
	// Copy constructor
	Doom3Entity(const Doom3Entity& other);
	
	~Doom3Entity();

	static void setKeyValueChangedFunc(EntityCreator::KeyValueChangedFunc func);
	static void setCounter(Counter* counter);

	void importState(const KeyValues& keyValues);
	typedef MemberCaller1<Doom3Entity, const KeyValues&, &Doom3Entity::importState> UndoImportCaller;

	void attach(Observer& observer);
	void detach(Observer& observer);

	void forEachKeyValue_instanceAttach(MapFile* map);
	void forEachKeyValue_instanceDetach(MapFile* map);

	void instanceAttach(MapFile* map);
	void instanceDetach(MapFile* map);

	/** Return the EntityClass associated with this entity.
	 */
	IEntityClassConstPtr getEntityClass() const;

	void forEachKeyValue(Visitor& visitor) const;

	/** Set a keyvalue on the entity.
	 */
	void setKeyValue(const std::string& key, const std::string& value);

	/** Retrieve a keyvalue from the entity.
	 */
	std::string getKeyValue(const std::string& key) const;

	bool isContainer() const;

private:
	/** greebo: Notifies the attached observers about key/value
	 * 			insertions and deletions.
	 */
	void notifyInsert(const std::string& key, KeyValue& value);
	void notifyErase(const std::string& key, KeyValue& value);
	
	void forEachKeyValue_notifyInsert();
	void forEachKeyValue_notifyErase();

	/** greebo: This checks whether the key already exists and
	 * 			acts accordingly (creates key if it's non-existent)
	 */
	void insert(const std::string& key, const std::string& value);

	/** greebo: This is called by the above and actually inserts
	 * 			the key/value pair into the map.
	 * 
	 * Note: This does NOT check for existing keys, it justs inserts the value 
	 */
	void insert(const std::string& key, const KeyValuePtr& keyValue);
	
	/** greebo: Tries to lookup the given key in the list.
	 */
	KeyValues::const_iterator find(const std::string& key) const;
	KeyValues::iterator find(const std::string& key);

	void erase(KeyValues::iterator i);
	void erase(const std::string& key);
};

} // namespace entity

#endif /*DOOM3ENTITY_H_*/
