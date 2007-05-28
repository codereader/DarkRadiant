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
	for(KeyValues::const_iterator i = other.m_keyValues.begin(); i != other.m_keyValues.end(); ++i) {
		insert((*i).first.c_str(), (*i).second->c_str());
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

	for(KeyValues::const_iterator i = keyValues.begin(); i != keyValues.end(); ++i) {
		insert((*i).first.c_str(), (*i).second);
	}

	m_entityKeyValueChanged();
}

void Doom3Entity::attach(Observer& observer) {
	ASSERT_MESSAGE(!m_observerMutex, "observer cannot be attached during iteration");
	m_observers.insert(&observer);
	for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
		observer.insert((*i).first.c_str(), *(*i).second);
	}
}

void Doom3Entity::detach(Observer& observer) {
	ASSERT_MESSAGE(!m_observerMutex, "observer cannot be detached during iteration");
	m_observers.erase(&observer);
	for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
		observer.erase((*i).first.c_str(), *(*i).second);
	}
}

void Doom3Entity::forEachKeyValue_instanceAttach(MapFile* map) {
	for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
		(*i).second->instanceAttach(map);
	}
}

void Doom3Entity::forEachKeyValue_instanceDetach(MapFile* map) {
	for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
		(*i).second->instanceDetach(map);
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
		visitor.visit((*i).first.c_str(), (*i).second->c_str());
	}
}

/** Set a keyvalue on the entity.
 */
void Doom3Entity::setKeyValue(const std::string& key, const std::string& value) {
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
std::string Doom3Entity::getKeyValue(const std::string& key) const {

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

bool Doom3Entity::isContainer() const {
	return m_isContainer;
}

void Doom3Entity::notifyInsert(const char* key, KeyValue& value) {
	m_observerMutex = true;
	for(Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
		(*i)->insert(key, value);
	}
	m_observerMutex = false;
}

void Doom3Entity::notifyErase(const char* key, KeyValue& value) {
	m_observerMutex = true;
	for(Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
		(*i)->erase(key, value);
	}
	m_observerMutex = false;
}

void Doom3Entity::forEachKeyValue_notifyInsert() {
	for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
		notifyInsert((*i).first.c_str(), *(*i).second);
	}
}

void Doom3Entity::forEachKeyValue_notifyErase() {
	for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
		notifyErase((*i).first.c_str(), *(*i).second);
	}
}

void Doom3Entity::insert(const char* key, const KeyValuePtr& keyValue) {
	KeyValues::iterator i = m_keyValues.insert(KeyValues::value_type(key, keyValue));
	notifyInsert(key, *(*i).second);

	if(m_instanced) {
		(*i).second->instanceAttach(m_undo.map());
	}
}

void Doom3Entity::insert(const char* key, const char* value) {
	KeyValues::iterator i = m_keyValues.find(key);
	if(i != m_keyValues.end()) {
		(*i).second->assign(value);
	}
	else {
		m_undo.save();
		insert(key, KeyValuePtr(new KeyValue(value, m_eclass->getValueForKey(key))));
	}
}

void Doom3Entity::erase(KeyValues::iterator i) {
	if(m_instanced) {
		(*i).second->instanceDetach(m_undo.map());
	}

	Key key((*i).first);
	KeyValuePtr value((*i).second);
	m_keyValues.erase(i);
	notifyErase(key.c_str(), *value);
}

void Doom3Entity::erase(const char* key) {
	KeyValues::iterator i = m_keyValues.find(key);
	if(i != m_keyValues.end()) {
		m_undo.save();
		erase(i);
	}
}

StringPool& Doom3Entity::getPool() {
	return Static<StringPool, KeyContext>::instance();
}

} // namespace entity
