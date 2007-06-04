#include "Doom3Entity.h"

#include "iradiant.h"
#include "icounter.h"
#include "ieclass.h"

namespace entity {

Doom3Entity::Doom3Entity(IEntityClassPtr eclass) :
	m_eclass(eclass),
	m_undo(m_keyValues, UndoImportCaller(*this)),
	m_instanced(false),
	m_observerMutex(false),
	m_isContainer(!eclass->isFixedSize())
{}

Doom3Entity::Doom3Entity(const Doom3Entity& other) :
	Entity(other),
	m_eclass(other.getEntityClass()),
	m_undo(m_keyValues, UndoImportCaller(*this)),
	m_instanced(false),
	m_observerMutex(false),
	m_isContainer(other.m_isContainer)
{
	for (KeyValues::const_iterator i = other.m_keyValues.begin(); 
		 i != other.m_keyValues.end(); 
		 ++i)
	{
		insert(i->first, i->second->get());
	}
}

Doom3Entity::~Doom3Entity() {
	for(Observers::iterator i = m_observers.begin(); i != m_observers.end();) {
		// post-increment to allow current element to be removed safely
		(*i++)->clear();
	}
	ASSERT_MESSAGE(m_observers.empty(), "EntityKeyValues::~EntityKeyValues: observers still attached");
}

// Static
void Doom3Entity::setKeyValueChangedFunc(EntityCreator::KeyValueChangedFunc func) {
	m_entityKeyValueChanged = func;
	KeyValue::setKeyValueChangedFunc(func);
}

void Doom3Entity::importState(const KeyValues& keyValues) {
	for(KeyValues::iterator i = m_keyValues.begin(); i != m_keyValues.end();) {
		erase(i++);
	}

	for (KeyValues::const_iterator i = keyValues.begin(); i != keyValues.end(); ++i) {
		insert(i->first, i->second);
	}

	m_entityKeyValueChanged();
}

void Doom3Entity::attach(Observer& observer) {
	ASSERT_MESSAGE(!m_observerMutex, "observer cannot be attached during iteration");
	m_observers.insert(&observer);
	for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
		observer.insert(i->first.c_str(), *i->second);
	}
}

void Doom3Entity::detach(Observer& observer) {
	ASSERT_MESSAGE(!m_observerMutex, "observer cannot be detached during iteration");
	m_observers.erase(&observer);
	for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
		observer.erase(i->first.c_str(), *i->second);
	}
}

void Doom3Entity::forEachKeyValue_instanceAttach(MapFile* map) {
	for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
		i->second->instanceAttach(map);
	}
}

void Doom3Entity::forEachKeyValue_instanceDetach(MapFile* map) {
	for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
		i->second->instanceDetach(map);
	}
}

void Doom3Entity::instanceAttach(MapFile* map) {
	GlobalRadiant().getCounter(counterEntities).increment();
	
	m_instanced = true;
	forEachKeyValue_instanceAttach(map);
	m_undo.instanceAttach(map);
}

void Doom3Entity::instanceDetach(MapFile* map) {
	GlobalRadiant().getCounter(counterEntities).decrement();

	m_undo.instanceDetach(map);
	forEachKeyValue_instanceDetach(map);
	m_instanced = false;
}

/** Return the EntityClass associated with this entity.
 */
IEntityClassConstPtr Doom3Entity::getEntityClass() const {
	return m_eclass;
}

void Doom3Entity::forEachKeyValue(Visitor& visitor) const {
	for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
		visitor.visit(i->first, i->second->get());
	}
}

/** Set a keyvalue on the entity.
 */
void Doom3Entity::setKeyValue(const std::string& key, const std::string& value) {
	if (value.empty()) {
		// Empty value means: delete the key
		erase(key);
	}
	else {
		// Non-empty value, "insert" it (will overwrite existing keys - no duplicates)
		insert(key, value);
	}
	// Notify the global observer (usually the EntityInspector)
	m_entityKeyValueChanged();
}

/** Retrieve a keyvalue from the entity.
 */
std::string Doom3Entity::getKeyValue(const std::string& key) const {

	// Lookup the key in the map
	KeyValues::const_iterator i = find(key);

	// If key is found, return it, otherwise lookup the default value on
	// the entity class
	if(i != m_keyValues.end()) {
		return i->second->c_str();
	}
	else {
		return m_eclass->getValueForKey(key);
	}
}

bool Doom3Entity::isContainer() const {
	return m_isContainer;
}

void Doom3Entity::setIsContainer(bool isContainer) {
	m_isContainer = isContainer;
}

void Doom3Entity::notifyInsert(const std::string& key, KeyValue& value) {
	m_observerMutex = true;
	for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
		(*i)->insert(key.c_str(), value);
	}
	m_observerMutex = false;
}

void Doom3Entity::notifyErase(const std::string& key, KeyValue& value) {
	m_observerMutex = true;
	for(Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
		(*i)->erase(key.c_str(), value);
	}
	m_observerMutex = false;
}

void Doom3Entity::forEachKeyValue_notifyInsert() {
	for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
		notifyInsert(i->first, *i->second);
	}
}

void Doom3Entity::forEachKeyValue_notifyErase() {
	for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
		notifyErase(i->first, *i->second);
	}
}

void Doom3Entity::insert(const std::string& key, const KeyValuePtr& keyValue) {
	// Insert the new key at the end of the list
	KeyValues::iterator i = m_keyValues.insert(
		m_keyValues.end(), 
		KeyValuePair(key, keyValue)
	);
	// Dereference the iterator to get a KeyValue& reference and notify the observers
	notifyInsert(key, *i->second);

	if (m_instanced) {
		i->second->instanceAttach(m_undo.map());
	}
}

void Doom3Entity::insert(const std::string& key, const std::string& value) {
	// Try to lookup the key in the map
	KeyValues::iterator i = find(key);
	
	if (i != m_keyValues.end()) {
		// Key has been found
		i->second->assign(value);
	}
	else {
		// No key with that name found, create a new one
		m_undo.save();
		// Allocate a new KeyValue object and insert it into the map
		insert(key, KeyValuePtr(new KeyValue(value, m_eclass->getValueForKey(key))));
	}
}

void Doom3Entity::erase(KeyValues::iterator i) {
	if (m_instanced) {
		i->second->instanceDetach(m_undo.map());
	}

	// Retrieve the key and value from the vector before deletion
	std::string key(i->first);
	KeyValuePtr value(i->second);
	// Actually delete the object from the list
	m_keyValues.erase(i);
	
	// Notify about the deletion
	notifyErase(key, *value);
	// Scope ends here, the KeyValue object will be deleted automatically
	// as the boost::shared_ptr useCount will reach zero.
}

void Doom3Entity::erase(const std::string& key) {
	// Try to lookup the key
	KeyValues::iterator i = find(key);
	
	if (i != m_keyValues.end()) {
		m_undo.save();
		erase(i);
	}
}

Doom3Entity::KeyValues::const_iterator Doom3Entity::find(const std::string& key) const {
	for (KeyValues::const_iterator i = m_keyValues.begin(); 
		 i != m_keyValues.end(); 
		 i++)
	{
		if (i->first == key) {
			return i;
		}
	}
	// Not found
	return m_keyValues.end();
}

Doom3Entity::KeyValues::iterator Doom3Entity::find(const std::string& key) {
	for (KeyValues::iterator i = m_keyValues.begin(); 
		 i != m_keyValues.end(); 
		 i++)
	{
		if (i->first == key) {
			return i;
		}
	}
	// Not found
	return m_keyValues.end();
}

} // namespace entity
