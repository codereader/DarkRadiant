#ifndef DOOM3ENTITY_H_
#define DOOM3ENTITY_H_

#include "KeyValue.h"

/** greebo: This is the implementation of the class IEntity.
 * 
 * A Doom3Entity basically just keeps track of all the
 * spawnargs and delivers them on request, taking the 
 * inheritance tree (EntityClasses) into account.
 */

namespace entity {

/// \brief An unsorted list of key/value pairs.
///
/// - Notifies observers when a pair is inserted or removed.
/// - Provides undo support through the global undo system.
/// - New keys are appended to the end of the list.
class Doom3Entity :
	public Entity {
public:
	typedef KeyValue Value;

	static StringPool& getPool() {
		return Static<StringPool, KeyContext>::instance();
	}
private:
	static EntityCreator::KeyValueChangedFunc m_entityKeyValueChanged;
	static Counter* m_counter;

	IEntityClassConstPtr m_eclass;

	class KeyContext {}
	;
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

	void notifyInsert(const char* key, Value& value) {
		m_observerMutex = true;
		for(Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
			(*i)->insert(key, value);
		}
		m_observerMutex = false;
	}
	void notifyErase(const char* key, Value& value) {
		m_observerMutex = true;
		for(Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
			(*i)->erase(key, value);
		}
		m_observerMutex = false;
	}
	void forEachKeyValue_notifyInsert() {
		for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
			notifyInsert((*i).first.c_str(), *(*i).second);
		}
	}
	void forEachKeyValue_notifyErase() {
		for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
			notifyErase((*i).first.c_str(), *(*i).second);
		}
	}

	void insert(const char* key, const KeyValuePtr& keyValue) {
		KeyValues::iterator i = m_keyValues.insert(KeyValues::value_type(key, keyValue));
		notifyInsert(key, *(*i).second);

		if(m_instanced) {
			(*i).second->instanceAttach(m_undo.map());
		}
	}

	void insert(const char* key, const char* value) {
		KeyValues::iterator i = m_keyValues.find(key);
		if(i != m_keyValues.end()) {
			(*i).second->assign(value);
		}
		else {
			m_undo.save();
			insert(key, KeyValuePtr(new KeyValue(value, m_eclass->getValueForKey(key).c_str())));
		}
	}

	void erase(KeyValues::iterator i) {
		if(m_instanced) {
			(*i).second->instanceDetach(m_undo.map());
		}

		Key key((*i).first);
		KeyValuePtr value((*i).second);
		m_keyValues.erase(i);
		notifyErase(key.c_str(), *value);
	}

	void erase(const char* key) {
		KeyValues::iterator i = m_keyValues.find(key);
		if(i != m_keyValues.end()) {
			m_undo.save();
			erase(i);
		}
	}

public:
	bool m_isContainer;

	Doom3Entity(IEntityClassPtr eclass) :
			m_eclass(eclass),
			m_undo(m_keyValues, UndoImportCaller(*this)),
			m_instanced(false),
			m_observerMutex(false),
	m_isContainer(!eclass->isFixedSize()) {}
	Doom3Entity(const Doom3Entity& other) :
			Entity(other),
			m_eclass(other.getEntityClass()),
			m_undo(m_keyValues, UndoImportCaller(*this)),
			m_instanced(false),
			m_observerMutex(false),
	m_isContainer(other.m_isContainer) {
		for(KeyValues::const_iterator i = other.m_keyValues.begin(); i != other.m_keyValues.end(); ++i) {
			insert((*i).first.c_str(), (*i).second->c_str());
		}
	}
	~Doom3Entity() {
		for(Observers::iterator i = m_observers.begin(); i != m_observers.end();) {
			// post-increment to allow current element to be removed safely
			(*i++)->clear();
		}
		ASSERT_MESSAGE(m_observers.empty(), "EntityKeyValues::~EntityKeyValues: observers still attached");
	}

	static void setKeyValueChangedFunc(EntityCreator::KeyValueChangedFunc func) {
		m_entityKeyValueChanged = func;
		KeyValue::setKeyValueChangedFunc(func);
	}
	static void setCounter(Counter* counter) {
		m_counter = counter;
	}

	void importState(const KeyValues& keyValues) {
		for(KeyValues::iterator i = m_keyValues.begin(); i != m_keyValues.end();) {
			erase(i++);
		}

		for(KeyValues::const_iterator i = keyValues.begin(); i != keyValues.end(); ++i) {
			insert((*i).first.c_str(), (*i).second);
		}

		m_entityKeyValueChanged();
	}
	typedef MemberCaller1<Doom3Entity, const KeyValues&, &Doom3Entity::importState> UndoImportCaller;

	void attach(Observer& observer) {
		ASSERT_MESSAGE(!m_observerMutex, "observer cannot be attached during iteration");
		m_observers.insert(&observer);
		for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
			observer.insert((*i).first.c_str(), *(*i).second);
		}
	}
	void detach(Observer& observer) {
		ASSERT_MESSAGE(!m_observerMutex, "observer cannot be detached during iteration");
		m_observers.erase(&observer);
		for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
			observer.erase((*i).first.c_str(), *(*i).second);
		}
	}

	void forEachKeyValue_instanceAttach(MapFile* map) {
		for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
			(*i).second->instanceAttach(map);
		}
	}
	void forEachKeyValue_instanceDetach(MapFile* map) {
		for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
			(*i).second->instanceDetach(map);
		}
	}

	void instanceAttach(MapFile* map) {
		if(m_counter != 0) {
			m_counter->increment();
		}

		m_instanced = true;
		forEachKeyValue_instanceAttach(map);
		m_undo.instanceAttach(map);
	}
	void instanceDetach(MapFile* map) {
		if(m_counter != 0) {
			m_counter->decrement();
		}

		m_undo.instanceDetach(map);
		forEachKeyValue_instanceDetach(map);
		m_instanced = false;
	}

	/** Return the EntityClass associated with this entity.
	 */
	IEntityClassConstPtr getEntityClass() const {
		return m_eclass;
	}

	void forEachKeyValue(Visitor& visitor) const {
		for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
			visitor.visit((*i).first.c_str(), (*i).second->c_str());
		}
	}

	/** Set a keyvalue on the entity.
	 */
	void setKeyValue(const std::string& key, const std::string& value) {
		if (value.empty()) {
			erase(key.c_str());
		}
		else {
			insert(key.c_str(), value.c_str());
		}
		m_entityKeyValueChanged();
	}

	/** Retrieve a keyvalue from the entity.
	 */
	std::string getKeyValue(const std::string& key) const {

		// Lookup the key in the map
		KeyValues::const_iterator i = m_keyValues.find(key.c_str());

		// If key is found, return it, otherwise lookup the default value on
		// the entity class
		if(i != m_keyValues.end()) {
			return i->second->c_str();
		}
		else {
			return m_eclass->getValueForKey(key);
		}
	}

	bool isContainer() const {
		return m_isContainer;
	}
};

} // namespace entity

#endif /*DOOM3ENTITY_H_*/
