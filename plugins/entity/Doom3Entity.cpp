#include "Doom3Entity.h"

#include "iradiant.h"
#include "icounter.h"
#include "ieclass.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace entity {

Doom3Entity::Doom3Entity(const IEntityClassConstPtr& eclass) :
	_eclass(eclass),
	_undo(_keyValues, UndoImportCaller(*this)),
	_instanced(false),
	_observerMutex(false),
	_isContainer(!eclass->isFixedSize())
{}

Doom3Entity::Doom3Entity(const Doom3Entity& other) :
	Entity(other),
	_eclass(other.getEntityClass()),
	_undo(_keyValues, UndoImportCaller(*this)),
	_instanced(false),
	_observerMutex(false),
	_isContainer(other._isContainer)
{
	for (KeyValues::const_iterator i = other._keyValues.begin(); 
		 i != other._keyValues.end(); 
		 ++i)
	{
		insert(i->first, i->second->get());
	}
}

Doom3Entity::~Doom3Entity() {
	for (Observers::iterator i = _observers.begin(); i != _observers.end();) {
		// post-increment to allow current element to be removed safely
		(*i++)->onDestruct();
	}
	ASSERT_MESSAGE(_observers.empty(), "EntityKeyValues::~EntityKeyValues: observers still attached");
}

// Static
void Doom3Entity::setKeyValueChangedFunc(EntityCreator::KeyValueChangedFunc func) {
	_keyValueChangedNotify = func;
	KeyValue::setKeyValueChangedFunc(func);
}

bool Doom3Entity::isModel() const {
	std::string name = getKeyValue("name");
	std::string model = getKeyValue("model");
	std::string classname = getKeyValue("classname");
	
	return (classname == "func_static" && !name.empty() && name != model); 
}

void Doom3Entity::importState(const KeyValues& keyValues) {
	// Remove the entity key values, one by one
	while (_keyValues.size() > 0) {
		erase(_keyValues.begin());
	}
	
	/* greebo: This code somehow doesn't delete all the keys (only every second one)
	for(KeyValues::iterator i = _keyValues.begin(); i != _keyValues.end();) {
		erase(i++);
	}*/

	for (KeyValues::const_iterator i = keyValues.begin(); i != keyValues.end(); ++i) {
		insert(i->first, i->second);
	}

	_keyValueChangedNotify();
}

void Doom3Entity::attach(Observer& observer) {
	ASSERT_MESSAGE(!_observerMutex, "observer cannot be attached during iteration");
	
	// Add the observer to the internal list
	_observers.push_back(&observer);
	
	// Now notify the observer about all the existing keys
	for(KeyValues::const_iterator i = _keyValues.begin(); i != _keyValues.end(); ++i) {
		observer.onKeyInsert(i->first, *i->second);
	}
}

void Doom3Entity::detach(Observer& observer) {
	ASSERT_MESSAGE(!_observerMutex, "observer cannot be detached during iteration");
	
	// Remove the observer from the list, if it can be found
	Observers::iterator found = std::find(_observers.begin(), _observers.end(), &observer);
	if (found != _observers.end()) {
		_observers.erase(found);
	}
	else {
		// greebo: Observer was not found, no need to call onKeyErase()
		return;
	}
	
	// Now, call onKeyErase() for every spawnarg, so that the observer gets cleanly shut down 
	for(KeyValues::const_iterator i = _keyValues.begin(); i != _keyValues.end(); ++i) {
		observer.onKeyErase(i->first, *i->second);
	}
}

void Doom3Entity::forEachKeyValue_instanceAttach(MapFile* map) {
	for(KeyValues::const_iterator i = _keyValues.begin(); i != _keyValues.end(); ++i) {
		i->second->instanceAttach(map);
	}
}

void Doom3Entity::forEachKeyValue_instanceDetach(MapFile* map) {
	for(KeyValues::const_iterator i = _keyValues.begin(); i != _keyValues.end(); ++i) {
		i->second->instanceDetach(map);
	}
}

void Doom3Entity::instanceAttach(MapFile* map) {
	GlobalRadiant().getCounter(counterEntities).increment();
	
	_instanced = true;
	forEachKeyValue_instanceAttach(map);
	_undo.instanceAttach(map);
}

void Doom3Entity::instanceDetach(MapFile* map) {
	GlobalRadiant().getCounter(counterEntities).decrement();

	_undo.instanceDetach(map);
	forEachKeyValue_instanceDetach(map);
	_instanced = false;
}

/** Return the EntityClass associated with this entity.
 */
IEntityClassConstPtr Doom3Entity::getEntityClass() const {
	return _eclass;
}

void Doom3Entity::forEachKeyValue(Visitor& visitor) const {
	for(KeyValues::const_iterator i = _keyValues.begin(); i != _keyValues.end(); ++i) {
		visitor.visit(i->first, i->second->get());
	}
}

void Doom3Entity::forEachKeyValue(KeyValueVisitor& visitor) {
	for(KeyValues::iterator i = _keyValues.begin(); i != _keyValues.end(); ++i) {
		visitor.visit(i->first, *i->second);
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
	_keyValueChangedNotify();
}

/** Retrieve a keyvalue from the entity.
 */
std::string Doom3Entity::getKeyValue(const std::string& key) const {

	// Lookup the key in the map
	KeyValues::const_iterator i = find(key);

	// If key is found, return it, otherwise lookup the default value on
	// the entity class
	if(i != _keyValues.end()) {
		return i->second->get();
	}
	else {
		return _eclass->getAttribute(key).value;
	}
}

bool Doom3Entity::isInherited(const std::string& key) const {
	// Check if we have the key in the local keyvalue map
	bool definedLocally = (find(key) != _keyValues.end());

	// The value is inherited, if it doesn't exist locally and the inherited one is not empty
	return (!definedLocally && !_eclass->getAttribute(key).value.empty());
}

Entity::KeyValuePairs Doom3Entity::getKeyValuePairs(const std::string& prefix) const {
	KeyValuePairs list;

	for (KeyValues::const_iterator i = _keyValues.begin(); i != _keyValues.end(); i++) {
		// If the prefix matches, add to list
		if (boost::algorithm::istarts_with(i->first, prefix)) {
			list.push_back(
				std::pair<std::string, std::string>(i->first, i->second->get())
			);
		}
	}

	return list;
}

bool Doom3Entity::isContainer() const {
	return _isContainer;
}

void Doom3Entity::setIsContainer(bool isContainer) {
	_isContainer = isContainer;
}

void Doom3Entity::notifyInsert(const std::string& key, KeyValue& value) {
	// Block the addition/removal of new Observers during this process
	_observerMutex = true;
	
	// Notify all the Observers about the new keyvalue
	for (Observers::iterator i = _observers.begin(); i != _observers.end(); ++i) {
		(*i)->onKeyInsert(key, value);
	}
	
	_observerMutex = false;
}

void Doom3Entity::notifyErase(const std::string& key, KeyValue& value) {
	// Block the addition/removal of new Observers during this process
	_observerMutex = true;
	
	for(Observers::iterator i = _observers.begin(); i != _observers.end(); ++i) {
		(*i)->onKeyErase(key, value);
	}
	
	_observerMutex = false;
}

void Doom3Entity::forEachKeyValue_notifyInsert() {
	for(KeyValues::const_iterator i = _keyValues.begin(); i != _keyValues.end(); ++i) {
		notifyInsert(i->first, *i->second);
	}
}

void Doom3Entity::forEachKeyValue_notifyErase() {
	for(KeyValues::const_iterator i = _keyValues.begin(); i != _keyValues.end(); ++i) {
		notifyErase(i->first, *i->second);
	}
}

void Doom3Entity::insert(const std::string& key, const KeyValuePtr& keyValue) {
	// Insert the new key at the end of the list
	KeyValues::iterator i = _keyValues.insert(
		_keyValues.end(), 
		KeyValuePair(key, keyValue)
	);
	// Dereference the iterator to get a KeyValue& reference and notify the observers
	notifyInsert(key, *i->second);

	if (_instanced) {
		i->second->instanceAttach(_undo.map());
	}
}

void Doom3Entity::insert(const std::string& key, const std::string& value) {
	// Try to lookup the key in the map
	KeyValues::iterator i = find(key);
	
	if (i != _keyValues.end()) {
		// Key has been found
		i->second->assign(value);
	}
	else {
		// No key with that name found, create a new one
		_undo.save();
		// Allocate a new KeyValue object and insert it into the map
		insert(
			key, 
			KeyValuePtr(new KeyValue(value, _eclass->getAttribute(key).value))
		);
	}
}

void Doom3Entity::erase(KeyValues::iterator i) {
	if (_instanced) {
		i->second->instanceDetach(_undo.map());
	}

	// Retrieve the key and value from the vector before deletion
	std::string key(i->first);
	KeyValuePtr value(i->second);
	// Actually delete the object from the list
	_keyValues.erase(i);
	
	// Notify about the deletion
	notifyErase(key, *value);
	// Scope ends here, the KeyValue object will be deleted automatically
	// as the boost::shared_ptr useCount will reach zero.
}

void Doom3Entity::erase(const std::string& key) {
	// Try to lookup the key
	KeyValues::iterator i = find(key);
	
	if (i != _keyValues.end()) {
		_undo.save();
		erase(i);
	}
}

Doom3Entity::KeyValues::const_iterator Doom3Entity::find(const std::string& key) const {
	for (KeyValues::const_iterator i = _keyValues.begin(); 
		 i != _keyValues.end(); 
		 i++)
	{
		if (i->first == key) {
			return i;
		}
	}
	// Not found
	return _keyValues.end();
}

Doom3Entity::KeyValues::iterator Doom3Entity::find(const std::string& key) {
	for (KeyValues::iterator i = _keyValues.begin(); 
		 i != _keyValues.end(); 
		 i++)
	{
		if (i->first == key) {
			return i;
		}
	}
	// Not found
	return _keyValues.end();
}

} // namespace entity
