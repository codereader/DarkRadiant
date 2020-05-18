#include "NamespaceManager.h"

#include "iregistry.h"
#include "itextstream.h"
#include "string/predicate.h"
#include "gamelib.h"

namespace entity 
{

// The registry key pointing towards the "name" spawnarg
const char* const GKEY_NAME_KEY("/defaults/nameKey");

NamespaceManager::NamespaceManager(Doom3Entity& entity) :
    _namespace(nullptr),
    _entity(entity),
    _updateMutex(false),
    _nameKey(game::current::getValue<std::string>(GKEY_NAME_KEY))
{
    // Attach <self> to the observed entity
    _entity.attachObserver(this);
}

NamespaceManager::~NamespaceManager() 
{
    // Detach <self> from the observed Entity
    _entity.detachObserver(this);

    if (_namespace != nullptr) 
    {
        // We're still attached to a namespace, break the connection
        disconnectNameObservers();
        detachNames();

        setNamespace(nullptr);
    }
}

// Gets/sets the namespace of this named object
void NamespaceManager::setNamespace(INamespace* space) 
{
    _namespace = space;
}

INamespace* NamespaceManager::getNamespace() const 
{
    return _namespace;
}

void NamespaceManager::detachNames() 
{
    if (_namespace == nullptr) return;

    // Detach all names of the observed D3Entity
    detachNameKeys();
}

void NamespaceManager::connectNameObservers() 
{
    if (_namespace == nullptr) return;

    // Setup the keyobservers
    attachKeyObservers();
}

void NamespaceManager::disconnectNameObservers() 
{
    if (_namespace == nullptr) return;

    // Remove all keyobservers
    detachKeyObservers();

    // The namekeyobservers should be gone at this point, all of them
    assert(_nameKeyObservers.empty());
}

std::string NamespaceManager::getName() const
{
    return _entity.getKeyValue(_nameKey);
}

void NamespaceManager::changeName(const std::string& newName) 
{
    // Set the value, this should trigger the nameChanged() event on all observers
    _entity.setKeyValue(_nameKey, newName);
}

void NamespaceManager::onKeyInsert(const std::string& key, EntityKeyValue& value) 
{
    // avoid double-updates when the keyvalue gets updated during this process
    if (_updateMutex) return;

    // Check if the key is relevant
    if (keyIsName(key)) 
    {
        // Key is a "name", remember that one
        _nameKeys.insert(std::make_pair(key, &value));

        // Now, register this key in the namespace, if we have one
        attachKeyToNamespace(key, value);
    }

    // Now setup the keyobservers
    attachKeyObserver(key, value);
}

void NamespaceManager::onKeyErase(const std::string& key, EntityKeyValue& value) 
{
    // avoid double-updates when the keyvalue gets updated during this process
    if (_updateMutex) return;

    // Check if the key is relevant
    if (keyIsName(key)) 
    {
        // Remove the key from the namespace
        detachKeyFromNamespace(key, value);

        // Remove they key from the map
        _nameKeys.erase(key);
    }

    // Remove the keyobserver from this key
    detachKeyObserver(key, value);
}

bool NamespaceManager::keyIsName(const std::string& key) 
{
    // In D3, only "name" spawnargs are actual names
    return key == _nameKey;
}

bool NamespaceManager::keyIsReferringToEntityDef(const std::string& key)
{
    return key == "classname" || string::starts_with(key, "def_");
}

// Freshly attaches all "names" to our namespace
void NamespaceManager::attachNames()
{
    if (!_namespace) return;

    for (const auto& pair : _nameKeys)
    {
        attachKeyToNamespace(pair.first, *pair.second);
    }
}

void NamespaceManager::detachNameKeys() 
{
    for (const auto& pair : _nameKeys)
    {
        detachKeyFromNamespace(pair.first, *pair.second);
    }
}

void NamespaceManager::attachKeyToNamespace(const std::string& key, EntityKeyValue& keyValue)
{
    if (!_namespace) return;

    std::string nameValue = keyValue.get();

    // Check if the name already exists in that namespace
    if (_namespace->nameExists(nameValue))
    {
        // We need to change our name, it seems, acquire a new one (and insert it)
        nameValue = _namespace->addUniqueName(nameValue);

        // Lock this class, to avoid this class from being called twice
        _updateMutex = true;

        // Update the entity keyvalue
        keyValue.assign(nameValue);

        _updateMutex = false;
    }
    // Name is valid and not yet known to this namespace, insert it
    else if (!_namespace->insert(nameValue))
    {
        rError() << "Could not insert name: " << nameValue << " into namespace!\n";
    }
}

void NamespaceManager::detachKeyFromNamespace(const std::string& key, EntityKeyValue& keyValue) 
{
    if (_namespace == nullptr) return;

    // Remove the key from the namespace
    _namespace->erase(keyValue.get());
}

void NamespaceManager::attachKeyObserver(const std::string& key, EntityKeyValue& keyValue)
{
    if (_namespace == nullptr) return;

    if (keyIsName(key))
    {
        // Instantiate a new observer
        auto observer = std::make_shared<NameKeyObserver>(keyValue, _namespace);

        // Store this observer object in the local map
        _nameKeyObservers.insert(std::make_pair(&keyValue, observer));
    }
    else if (!keyIsReferringToEntityDef(key))
    {
        // Instantiate a new observer
        auto observer = std::make_shared<KeyValueObserver>(keyValue, _namespace);

        // Store this observer object in the local map
        _keyValueObservers.insert(std::make_pair(&keyValue, observer));
    }
}

void NamespaceManager::attachKeyObservers()
{
    // May not be called with empty namespace
    assert(_namespace);

    // Traverse the entity
    _entity.forEachEntityKeyValue([this](const std::string& key, EntityKeyValue& value)
    {
        attachKeyObserver(key, value);
    });
}

void NamespaceManager::detachKeyObserver(const std::string& key, EntityKeyValue& keyValue) 
{
    if (_namespace == nullptr) return;

    if (keyIsName(key))
    {
        // Destroy the NameKeyObserver object
        _nameKeyObservers.erase(&keyValue);
    }
    else if (!keyIsReferringToEntityDef(key))
    {
        // Not a name key, destroy the KeyValueObserver
        _keyValueObservers.erase(&keyValue);
    }
}

void NamespaceManager::detachKeyObservers() 
{
    // May not be called with empty namespace
    assert(_namespace);

    // Traverse the entity
    _entity.forEachEntityKeyValue([this](const std::string& key, EntityKeyValue& value)
    {
        detachKeyObserver(key, value);
    });
}

} // namespace entity
