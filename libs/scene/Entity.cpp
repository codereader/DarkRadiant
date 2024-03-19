#include "Entity.h"

#include "ieclass.h"
#include "debugging/debugging.h"
#include "string/predicate.h"
#include <functional>

Entity::Entity(const IEntityClassPtr& eclass) :
	_eclass(eclass),
	_undo(_keyValues, std::bind(&Entity::importState, this, std::placeholders::_1),
        std::function<void()>(), "EntityKeyValues"),
	_observerMutex(false),
	_isContainer(!eclass->isFixedSize()),
	_attachments(eclass->getDeclName())
{
    // Parse attachment keys
    parseAttachments();
}

Entity::Entity(const Entity& other) :
	_eclass(other.getEntityClass()),
	_undo(_keyValues, std::bind(&Entity::importState, this, std::placeholders::_1),
        std::function<void()>(), "EntityKeyValues"),
	_observerMutex(false),
	_isContainer(other._isContainer),
	_attachments(other._attachments)
{
    // Copy keyvalue strings, not actual KeyValue pointers
    for (const KeyValuePair& p : other._keyValues)
    {
        insert(p.first, p.second->get());
    }
}

void Entity::parseAttachments()
{
    // Parse the keys
    forEachKeyValue(
        [this](const std::string& k, const std::string& v) {
            _attachments.parseDefAttachKeys(k, v);
        },
        true /* includeInherited */);

    // Post validation step
    _attachments.validateAttachments();
}

bool Entity::isModel() const
{
	std::string name = getKeyValue("name");
	std::string model = getKeyValue("model");
	std::string classname = getKeyValue("classname");

	return (classname == "func_static" && !name.empty() && name != model);
}

bool Entity::isOfType(const std::string& className)
{
	return _eclass->isOfType(className);
}

void Entity::importState(const KeyValues& keyValues)
{
	// Remove the entity key values, one by one
	while (_keyValues.size() > 0)
	{
		erase(_keyValues.begin());
	}

	for (const auto& pair : keyValues)
	{
		insert(pair.first, pair.second);
	}
}

void Entity::attachObserver(Observer* observer)
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

void Entity::detachObserver(Observer* observer)
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

void Entity::connectUndoSystem(IUndoSystem& undoSystem)
{
	for (const auto& keyValue : _keyValues)
	{
		keyValue.second->connectUndoSystem(undoSystem);
	}

    _undo.connectUndoSystem(undoSystem);
}

void Entity::disconnectUndoSystem(IUndoSystem& undoSystem)
{
	_undo.disconnectUndoSystem(undoSystem);

	for (const auto& keyValue : _keyValues)
	{
		keyValue.second->disconnectUndoSystem(undoSystem);
	}
}

IEntityClassPtr Entity::getEntityClass() const
{
	return _eclass;
}

void Entity::forEachKeyValue(KeyValueVisitFunc func, bool includeInherited) const
{
    // Visit explicit spawnargs
    for (const KeyValuePair& pair : _keyValues)
	{
		func(pair.first, pair.second->get());
	}

    // If requested, visit inherited spawnargs from the entitydef
    if (includeInherited)
    {
        _eclass->forEachAttribute([&](const EntityClassAttribute& att, bool) {
            func(att.getName(), att.getValue());
        });
    }
}

void Entity::forEachEntityKeyValue(const EntityKeyValueVisitFunctor& func)
{
    for (const KeyValuePair& pair : _keyValues)
    {
        func(pair.first, *pair.second);
    }
}

void Entity::setKeyValue(const std::string& key, const std::string& value)
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

std::string Entity::getKeyValue(const std::string& key) const
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
		return _eclass->getAttributeValue(key);
	}
}

bool Entity::isInherited(const std::string& key) const
{
	// Check if we have the key in the local keyvalue map
	bool definedLocally = (find(key) != _keyValues.end());

	// The value is inherited, if it doesn't exist locally and the inherited one is not empty
	return (!definedLocally && !_eclass->getAttributeValue(key).empty());
}

void Entity::forEachAttachment(AttachmentFunc func) const
{
    _attachments.forEachAttachment(func);
}

EntityKeyValuePtr Entity::getEntityKeyValue(const std::string& key)
{
	KeyValues::const_iterator found = find(key);

	return (found != _keyValues.end()) ? found->second : EntityKeyValuePtr();
}

bool Entity::isWorldspawn() const
{
	return getKeyValue("classname") == "worldspawn";
}

bool Entity::isContainer() const
{
	return _isContainer;
}

void Entity::setIsContainer(bool isContainer)
{
	_isContainer = isContainer;
}

void Entity::notifyInsert(const std::string& key, EntityKeyValue& value)
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

void Entity::notifyChange(const std::string& k, const std::string& v)
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

void Entity::notifyErase(const std::string& key, EntityKeyValue& value)
{
	// Block the addition/removal of new Observers during this process
	_observerMutex = true;

	for(Observers::iterator i = _observers.begin(); i != _observers.end(); ++i)
	{
		(*i)->onKeyErase(key, value);
	}

	_observerMutex = false;
}

void Entity::insert(const std::string& key, const KeyValuePtr& keyValue)
{
	// Insert the new key at the end of the list
	auto& pair = _keyValues.emplace_back(key, keyValue);

	// Dereference the iterator to get a KeyValue& reference and notify the observers
	notifyInsert(key, *pair.second);

	if (_undo.isConnected())
	{
        pair.second->connectUndoSystem(_undo.getUndoSystem());
	}
}

void Entity::insert(const std::string& key, const std::string& value)
{
    // Try to lookup the key in the map
    auto i = find(key);

    if (i != _keyValues.end())
    {
        // Key has been found, assign the value
        i->second->assign(value);
        // Observer notification happens through the lambda callback we passed
        // to the KeyValue constructor
    }
    else
    {
        // No key with that name found, create a new one
        _undo.save();

        // Allocate a new KeyValue object and insert it into the map
        // Capture the key by value in the lambda
        insert(key, std::make_shared<EntityKeyValue>(value, _eclass->getAttributeValue(key),
            [key, this](const std::string& value) { notifyChange(key, value); }));
    }
}

void Entity::erase(const KeyValues::iterator& i)
{
	if (_undo.isConnected())
	{
		i->second->disconnectUndoSystem(_undo.getUndoSystem());
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

void Entity::erase(const std::string& key)
{
	// Try to lookup the key
	KeyValues::iterator i = find(key);

	if (i != _keyValues.end())
	{
		_undo.save();
		erase(i);
	}
}

Entity::KeyValues::const_iterator Entity::find(const std::string& key) const
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

Entity::KeyValues::iterator Entity::find(const std::string& key)
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
