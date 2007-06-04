#ifndef DOOM3ENTITY_H_
#define DOOM3ENTITY_H_

#include "string/pooledstring.h"
#include "generic/referencecounted.h"
#include "KeyValue.h"

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

	IEntityClassConstPtr m_eclass;

	class KeyContext {};
	typedef Static<StringPool, KeyContext> KeyPool;
	typedef PooledString<KeyPool> Key;
	typedef SmartPointer<KeyValue> KeyValuePtr;
	typedef UnsortedMap<Key, KeyValuePtr> KeyValues;
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
	void setIsContainer(bool isContainer);

private:
	void notifyInsert(const char* key, KeyValue& value);
	void notifyErase(const char* key, KeyValue& value);
	void forEachKeyValue_notifyInsert();
	void forEachKeyValue_notifyErase();

	void insert(const char* key, const KeyValuePtr& keyValue);

	void insert(const char* key, const char* value);

	void erase(KeyValues::iterator i);

	void erase(const char* key);
};

} // namespace entity

#endif /*DOOM3ENTITY_H_*/
