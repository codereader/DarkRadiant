#include "SpawnArgs.h"

#include "ieclass.h"
#include "debugging/debugging.h"
#include "string/predicate.h"
#include <functional>

namespace entity
{

SpawnArgs::SpawnArgs(const IEntityClassPtr& eclass) :
	_eclass(eclass),
	_undo(_keyValues, std::bind(&SpawnArgs::importState, this, std::placeholders::_1), "EntityKeyValues"),
	_instanced(false),
	_observerMutex(false),
	_isContainer(!eclass->isFixedSize()),
	_attachments(eclass->getName())
{}

SpawnArgs::SpawnArgs(const SpawnArgs& other) :
	Entity(other),
	_eclass(other.getEntityClass()),
	_undo(_keyValues, std::bind(&SpawnArgs::importState, this, std::placeholders::_1), "EntityKeyValues"),
	_instanced(false),
	_observerMutex(false),
	_isContainer(other._isContainer),
	_attachments(other._attachments)
{
	for (KeyValues::const_iterator i = other._keyValues.begin();
		 i != other._keyValues.end();
		 ++i)
	{
		insert(i->first, i->second->get());
	}
}

bool SpawnArgs::isModel() const
{
	std::string name = getKeyValue("name");
	std::string model = getKeyValue("model");
	std::string classname = getKeyValue("classname");

	return (classname == "func_static" && !name.empty() && name != model);
}

bool SpawnArgs::isOfType(const std::string& className)
{
	return _eclass->isOfType(className);
}

void SpawnArgs::importState(const KeyValues& keyValues)
{
	// Remove the entity key values, one by one
	while (_keyValues.size() > 0)
	{
		erase(_keyValues.begin());
	}

	/* greebo: This code somehow doesn't delete all the keys (only every second one)
	for(KeyValues::iterator i = _keyValues.begin(); i != _keyValues.end();) {
		erase(i++);
	}*/

	for (KeyValues::const_iterator i = keyValues.begin(); i != keyValues.end(); ++i)
	{
		insert(i->first, i->second);
	}
}

void SpawnArgs::attachObserver(Observer* observer)
{
	ASSERT_MESSAGE(!_observerMutex, "observer cannot be attached during iteration");

	// Add the observer to the internal list
	_observers.insert(observer);

	// Now notify the observer about all the existing keys
	for(KeyValues::const_iterator i = _keyValues.begin(); i != _keyValues.end(); ++i)
    {
		observer->onKeyInsert(i->first, *i->second);
	}
}

void SpawnArgs::detachObserver(Observer* observer)
{
	ASSERT_MESSAGE(!_observerMutex, "observer cannot be detached during iteration");

	// Remove the observer from the list, if it can be found
	Observers::iterator found = _observers.find(observer);

	if (found == _observers.end())
	{
		// greebo: Observer was not found, no need to call onKeyErase()
		return;
	}

	// Remove the observer
	_observers.erase(found);

	// Call onKeyErase() for every spawnarg, so that the observer gets cleanly shut down
	for(KeyValues::const_iterator i = _keyValues.begin(); i != _keyValues.end(); ++i)
    {
		observer->onKeyErase(i->first, *i->second);
	}
}

void SpawnArgs::connectUndoSystem(IMapFileChangeTracker& changeTracker)
{
	_instanced = true;

	for (const auto& keyValue : _keyValues)
	{
		keyValue.second->connectUndoSystem(changeTracker);
	}

    _undo.connectUndoSystem(changeTracker);
}

void SpawnArgs::disconnectUndoSystem(IMapFileChangeTracker& changeTracker)
{
	_undo.disconnectUndoSystem(changeTracker);

	for (const auto& keyValue : _keyValues)
	{
		keyValue.second->disconnectUndoSystem(changeTracker);
	}

	_instanced = false;
}

/** Return the EntityClass associated with this entity.
 */
IEntityClassPtr SpawnArgs::getEntityClass() const
{
	return _eclass;
}

void SpawnArgs::forEachKeyValue(const KeyValueVisitFunctor& func) const
{
    for (const KeyValuePair& pair : _keyValues)
	{
		func(pair.first, pair.second->get());
	}
}

void SpawnArgs::forEachEntityKeyValue(const EntityKeyValueVisitFunctor& func)
{
    for (const KeyValuePair& pair : _keyValues)
    {
        func(pair.first, *pair.second);
    }
}

/** Set a keyvalue on the entity.
 */
void SpawnArgs::setKeyValue(const std::string& key, const std::string& value)
{
	if (value.empty())
	{
		// Empty value means: delete the key
		erase(key);
	}
	else
	{
		// Non-empty value, "insert" it (will overwrite existing keys - no duplicates)
		insert(key, value);
	}
}

/** Retrieve a keyvalue from the entity.
 */
std::string SpawnArgs::getKeyValue(const std::string& key) const
{
	// Lookup the key in the map
	KeyValues::const_iterator i = find(key);

	// If key is found, return it, otherwise lookup the default value on
	// the entity class
	if (i != _keyValues.end())
	{
		return i->second->get();
	}
	else
	{
		return _eclass->getAttribute(key).getValue();
	}
}

bool SpawnArgs::isInherited(const std::string& key) const
{
	// Check if we have the key in the local keyvalue map
	bool definedLocally = (find(key) != _keyValues.end());

	// The value is inherited, if it doesn't exist locally and the inherited one is not empty
	return (!definedLocally && !_eclass->getAttribute(key).getValue().empty());
}

void SpawnArgs::forEachAttachment(AttachmentFunc func) const
{
    _attachments.forEachAttachment(func);
}

Entity::KeyValuePairs SpawnArgs::getKeyValuePairs(const std::string& prefix) const
{
	KeyValuePairs list;

	for (KeyValues::const_iterator i = _keyValues.begin(); i != _keyValues.end(); ++i)
	{
		// If the prefix matches, add to list
		if (string::istarts_with(i->first, prefix))
		{
			list.push_back(
				std::pair<std::string, std::string>(i->first, i->second->get())
			);
		}
	}

	return list;
}

EntityKeyValuePtr SpawnArgs::getEntityKeyValue(const std::string& key)
{
	KeyValues::const_iterator found = find(key);

	return (found != _keyValues.end()) ? found->second : EntityKeyValuePtr();
}

bool SpawnArgs::isWorldspawn() const
{
	return getKeyValue("classname") == "worldspawn";
}

bool SpawnArgs::isContainer() const
{
	return _isContainer;
}

void SpawnArgs::setIsContainer(bool isContainer)
{
	_isContainer = isContainer;
}

void SpawnArgs::notifyInsert(const std::string& key, KeyValue& value)
{
	// Block the addition/removal of new Observers during this process
	_observerMutex = true;

	// Notify all the Observers about the new keyvalue
	for (Observers::iterator i = _observers.begin(); i != _observers.end(); ++i)
	{
		(*i)->onKeyInsert(key, value);
	}

	_observerMutex = false;
}

void SpawnArgs::notifyChange(const std::string& k, const std::string& v)
{
    _observerMutex = true;

    for (Observers::iterator i = _observers.begin();
         i != _observers.end();
         ++i)
    {
		(*i)->onKeyChange(k, v);
	}

    _observerMutex = false;
}

void SpawnArgs::notifyErase(const std::string& key, KeyValue& value)
{
	// Block the addition/removal of new Observers during this process
	_observerMutex = true;

	for(Observers::iterator i = _observers.begin(); i != _observers.end(); ++i)
	{
		(*i)->onKeyErase(key, value);
	}

	_observerMutex = false;
}

void SpawnArgs::insert(const std::string& key, const KeyValuePtr& keyValue)
{
	// Insert the new key at the end of the list
	KeyValues::iterator i = _keyValues.insert(
		_keyValues.end(),
		KeyValuePair(key, keyValue)
	);

	// Dereference the iterator to get a KeyValue& reference and notify the observers
	notifyInsert(key, *i->second);

	if (_instanced)
	{
		i->second->connectUndoSystem(_undo.getUndoChangeTracker());
	}
}

void SpawnArgs::insert(const std::string& key, const std::string& value)
{
	// Try to lookup the key in the map
	KeyValues::iterator i = find(key);

	if (i != _keyValues.end())
    {
		// Key has been found
		i->second->assign(value);

        // Notify observers of key change, using the found key as argument
		// as the case of the incoming "key" might be different
        notifyChange(i->first, value);
	}
	else
	{
		// No key with that name found, create a new one
		_undo.save();

		// Allocate a new KeyValue object and insert it into the map
		insert(
			key,
			KeyValuePtr(new KeyValue(value, _eclass->getAttribute(key).getValue()))
		);
	}
}

void SpawnArgs::erase(const KeyValues::iterator& i)
{
	if (_instanced)
	{
		i->second->disconnectUndoSystem(_undo.getUndoChangeTracker());
	}

	// Retrieve the key and value from the vector before deletion
	std::string key(i->first);
	KeyValuePtr value(i->second);

	// Actually delete the object from the list
	_keyValues.erase(i);

	// Notify about the deletion
	notifyErase(key, *value);

	// Scope ends here, the KeyValue object will be deleted automatically
	// as the std::shared_ptr useCount will reach zero.
}

void SpawnArgs::erase(const std::string& key)
{
	// Try to lookup the key
	KeyValues::iterator i = find(key);

	if (i != _keyValues.end())
	{
		_undo.save();
		erase(i);
	}
}

SpawnArgs::KeyValues::const_iterator SpawnArgs::find(const std::string& key) const
{
	for (KeyValues::const_iterator i = _keyValues.begin();
		 i != _keyValues.end();
		 ++i)
	{
		if (string::iequals(i->first, key))
		{
			return i;
		}
	}

	// Not found
	return _keyValues.end();
}

SpawnArgs::KeyValues::iterator SpawnArgs::find(const std::string& key)
{
	for (KeyValues::iterator i = _keyValues.begin();
		 i != _keyValues.end();
		 ++i)
	{
		if (string::iequals(i->first, key))
		{
			return i;
		}
	}

	// Not found
	return _keyValues.end();
}

} // namespace entity
