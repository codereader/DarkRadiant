#include "Doom3Entity.h"

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
	// Copy the key/values from the <other> entity
	for (KeyValues::const_iterator i = other.m_keyValues.begin(); 
		 i != other.m_keyValues.end(); 
		 ++i)
	{
		// Call insert with (key, KeyValuePtr) as argument
		insert(i->first, i->second);
	}
}

Doom3Entity::~Doom3Entity() {
	for (Observers::iterator i = m_observers.begin(); i != m_observers.end();) {
		// post-increment to allow current element to be removed safely
		(*i++)->clear();
	}
	ASSERT_MESSAGE(m_observers.empty(), "~Doom3Entity: observers still attached");
}

// Static
void Doom3Entity::setKeyValueChangedFunc(EntityCreator::KeyValueChangedFunc func) {
	m_entityKeyValueChanged = func;
	KeyValue::setKeyValueChangedFunc(func);
}

// Static
void Doom3Entity::setCounter(Counter* counter) {
	m_counter = counter;
}

void Doom3Entity::importState(const KeyValues& keyValues) {
	// First, safely remove everything from the current map
	for (KeyValues::iterator i = m_keyValues.begin(); i != m_keyValues.end();) {
		// Post-increment to prevent the iterator from becoming invalid in the loop
		erase(i++);
	}

	for (KeyValues::const_iterator i = keyValues.begin(); i != keyValues.end(); ++i) {
		// Copy the Key and the KeyValuePtr and pass it to insert()
		insert(i->first, i->second);
	}

	// Call the global notify callback
	m_entityKeyValueChanged();
}

void Doom3Entity::attach(Observer& observer) {
	ASSERT_MESSAGE(!m_observerMutex, "observer cannot be attached during iteration");
	
	// Store the observer pointer locally
	m_observers.insert(&observer);
	
	// Initialise the observer by calling insert() passing every key/value
	for (KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
		observer.insert(i->first.c_str(), *(i->second));
	}
}

void Doom3Entity::detach(Observer& observer) {
	ASSERT_MESSAGE(!m_observerMutex, "observer cannot be detached during iteration");
	
	// Erase the observer from the list
	m_observers.erase(&observer);
	
	// Now notify the observer by calling erase() for every key/value
	for(KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
		observer.erase(i->first.c_str(), *(i->second));
	}
}

void Doom3Entity::forEachKeyValue_instanceAttach(MapFile* map) {
	// attach the MapFile to every single KeyValue 
	for (KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
		i->second->instanceAttach(map);
	}
}

void Doom3Entity::forEachKeyValue_instanceDetach(MapFile* map) {
	// Detach the MapFile from every single KeyValue
	for (KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
		i->second->instanceDetach(map);
	}
}

void Doom3Entity::instanceAttach(MapFile* map) {
	if (m_counter != NULL) {
		m_counter->increment();
	}

	m_instanced = true;
	forEachKeyValue_instanceAttach(map);
	m_undo.instanceAttach(map);
}

void Doom3Entity::instanceDetach(MapFile* map) {
	if(m_counter != NULL) {
		m_counter->decrement();
	}

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
	for (KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
		visitor.visit(i->first, i->second->get());
	}
}

/** Set a keyvalue on the entity.
 */
void Doom3Entity::setKeyValue(const std::string& key, const std::string& value) {
	if (value.empty()) {
		// Value is empty, erase the key from the map
		erase(key);
	}
	else {
		// Store the non-empty value (creates if it doesn't exist yet)
		insert(key, value);
	}
	
	// Call the global notify callback
	m_entityKeyValueChanged();
}

/** Retrieve a keyvalue from the entity.
 */
std::string Doom3Entity::getKeyValue(const std::string& key) const {

	// Lookup the key in the map
	KeyValues::const_iterator i = find(key);

	// If key is found, return it, otherwise lookup the default value on
	// the entity class
	if (i != m_keyValues.end()) {
		return i->second->get();
	}
	else {
		return m_eclass->getValueForKey(key);
	}
}

bool Doom3Entity::isContainer() const {
	return m_isContainer;
}

void Doom3Entity::notifyInsert(const std::string& key, KeyValue& value) {
	m_observerMutex = true; // Disable observer attaching/detaching
	
	// Cycle through all observers and notify them by calling insert()
	for (Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
		(*i)->insert(key.c_str(), value);
	}
	m_observerMutex = false; // Enable observer attaching/detaching
}

void Doom3Entity::notifyErase(const std::string& key, KeyValue& value) {
	m_observerMutex = true; // Disable observer attaching/detaching
	
	// Cycle through all observers and notify them by calling erase()
	for(Observers::iterator i = m_observers.begin(); i != m_observers.end(); ++i) {
		(*i)->erase(key.c_str(), value);
	}
	m_observerMutex = false; // Enable observer attaching/detaching
}

void Doom3Entity::forEachKeyValue_notifyInsert() {
	for (KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
		notifyInsert(i->first, *(i->second));
	}
}

void Doom3Entity::forEachKeyValue_notifyErase() {
	for (KeyValues::const_iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
		notifyErase(i->first, *(i->second));
	}
}

void Doom3Entity::insert(const std::string& key, const KeyValuePtr& keyValue) {
	// Insert the key/value pair (the existance-check has already been performed!) 
	m_keyValues.push_back(
		KeyValuePair(key, keyValue)
	);
	
	// Notify the observers about the insertion
	notifyInsert(key, *keyValue);

	// Attach the map to the KeyValue if we're already instanced
	if (m_instanced) {
		keyValue->instanceAttach(m_undo.map());
	}
}

void Doom3Entity::insert(const std::string& key, const std::string& value) {
	// Check if the key already exists
	KeyValues::const_iterator i = find(key.c_str());
	
	if (i != m_keyValues.end()) {
		i->second->assign(value);
	}
	else {
		m_undo.save();
		insert(key, KeyValuePtr(new KeyValue(value, m_eclass->getValueForKey(key))));
	}
}

void Doom3Entity::erase(KeyValues::iterator i) {
	if (m_instanced) {
		// Detach the map from the KeyValue as it's about to be deleted
		i->second->instanceDetach(m_undo.map());
	}

	// Retrieve the values before deleting them
	std::string key = i->first;
	KeyValuePtr value = i->second;
	
	// Delete them from the list
	m_keyValues.erase(i);
	
	// Notify the observers, the objects will be deleted and the end of scope
	notifyErase(key, *value);
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

void Doom3Entity::erase(const std::string& key) {
	// Try to lookup the key
	KeyValues::iterator i = find(key);
	
	if (i != m_keyValues.end()) {
		// Key found, delete it
		m_undo.save();
		erase(i);
	}
}

} // namespace entity
