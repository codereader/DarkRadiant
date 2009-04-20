#include "NamespaceManager.h"

#include "iregistry.h"

namespace entity {

// The registry key pointing towards the "name" spawnarg
const std::string RKEY_NAME_KEY("game/defaults/nameKey");

NamespaceManager::NamespaceManager(Doom3Entity& entity) :
	_namespace(NULL),
	_entity(entity), 
	_updateMutex(false)
{
	// Attach <self> to the observed entity
	_entity.attachObserver(this);
}

NamespaceManager::~NamespaceManager() {
	// Detach <self> from the observed Entity
	_entity.detachObserver(this);

	if (_namespace != NULL) {
		// We're still attached to a namespace, break the connection
		disconnectNameObservers();
		detachNames();

		setNamespace(NULL);
	}
}

// Gets/sets the namespace of this named object
void NamespaceManager::setNamespace(INamespace* space) {
	_namespace = space;
}

INamespace* NamespaceManager::getNamespace() const {
	return _namespace;
}

void NamespaceManager::attachNames() {
	if (_namespace == NULL) return;

	// Attach all names of the observed D3Entity
	attachNameKeys();
}

void NamespaceManager::detachNames() {
	if (_namespace == NULL) return;
	
	// Detach all names of the observed D3Entity
	detachNameKeys();
}

void NamespaceManager::connectNameObservers() {
	if (_namespace == NULL) return;
	
	// Setup the keyobservers
	attachKeyObservers();
}

void NamespaceManager::disconnectNameObservers() {
	if (_namespace == NULL) return;
	
	// Remove all keyobservers
	detachKeyObservers();

	// The namekeyobservers should be gone at this point, all of them
	assert(_nameKeyObservers.empty());
}

std::string NamespaceManager::getName() {
	static std::string nameKey = GlobalRegistry().get(RKEY_NAME_KEY);
	return _entity.getKeyValue(nameKey);
}

void NamespaceManager::changeName(const std::string& newName) {
	// Find out what the name key is
	static std::string nameKey = GlobalRegistry().get(RKEY_NAME_KEY);
	// Set the value, this should trigger the nameChanged() event on all observers
	_entity.setKeyValue(nameKey, newName);
}

void NamespaceManager::onKeyInsert(const std::string& key, EntityKeyValue& value) {
	// avoid double-updates when the keyvalue gets updated during this process
	if (_updateMutex) return;

	// Check if the key is relevant
	if (keyIsName(key)) {
		// Key is a "name", remember that one
		_nameKeys.insert(KeyValues::value_type(key, &value));

		// Now, register this key in the namespace, if we have one
		attachKeyToNamespace(key, value);
	}
	
	// Now setup the keyobservers
	attachKeyObserver(key, value);
}

void NamespaceManager::onKeyErase(const std::string& key, EntityKeyValue& value) {
	// avoid double-updates when the keyvalue gets updated during this process
	if (_updateMutex) return;

	// Check if the key is relevant
	if (keyIsName(key)) {
		// Remove the key from the namespace
		detachKeyFromNamespace(key, value);

		// Remove they key from the map
		_nameKeys.erase(key);
	}
	
	// Remove the keyobserver from this key
	detachKeyObserver(key, value);
}

bool NamespaceManager::keyIsName(const std::string& key) {
	// In D3, only "name" spawnargs are actual names
	static std::string nameKey = GlobalRegistry().get(RKEY_NAME_KEY);
	return (key == nameKey);
}

// Freshly attaches all "names" to our namespace
void NamespaceManager::attachNameKeys() {
	for (KeyValues::iterator i = _nameKeys.begin(); i != _nameKeys.end(); ++i) {
		attachKeyToNamespace(i->first, *i->second);
	}
}

void NamespaceManager::detachNameKeys() {
	for (KeyValues::iterator i = _nameKeys.begin(); i != _nameKeys.end(); ++i) {
		detachKeyFromNamespace(i->first, *i->second);
	}
}

void NamespaceManager::attachKeyToNamespace(const std::string& key, EntityKeyValue& keyValue) {
	if (_namespace == NULL) return;

	std::string nameValue = keyValue.get();

	// Check if the name already exists in that namespace
	if (_namespace->nameExists(nameValue)) {
		// We need to change our name, it seems, acquire a new one (and insert it)
		nameValue = _namespace->makeUniqueAndInsert(nameValue);

		// Lock this class, to avoid this class from being called twice
		_updateMutex = true;

		// Update the entity keyvalue
		keyValue.assign(nameValue);

		_updateMutex = false;
	}
	else {
		// Name is valid and not yet known to this namespace, insert it
		if (!_namespace->insert(nameValue)) {
			globalErrorStream() << "Could not insert name: " << nameValue << " into namespace!\n";
		}
	}
}

void NamespaceManager::detachKeyFromNamespace(const std::string& key, EntityKeyValue& keyValue) {
	if (_namespace == NULL) return;

	// Remove the key from the namespace
	_namespace->erase(keyValue.get());
}

void NamespaceManager::attachKeyObserver(const std::string& key, EntityKeyValue& keyValue) {
	if (_namespace == NULL) return;

	if (keyIsName(key)) {
		// Instantiate a new observer
		NameKeyObserverPtr observer(new NameKeyObserver(keyValue, _namespace));

		// Store this observer object in the local map
		_nameKeyObservers.insert(
			NameKeyObserverMap::value_type(&keyValue, observer)
		);
	}
	else {
		// Instantiate a new observer
		KeyValueObserverPtr observer(new KeyValueObserver(keyValue, _namespace));

		// Store this observer object in the local map
		_keyValueObservers.insert(
			KeyValueObserverMap::value_type(&keyValue, observer)
		);
	}
}

void NamespaceManager::attachKeyObservers() {
	// May not be called with empty namespace
	assert(_namespace != NULL);

	// Local helper class used to traverse the keyvalues
	class Attacher :
		public Entity::KeyValueVisitor
	{
		NamespaceManager& _self;

	public:
		Attacher(NamespaceManager& self) : 
			_self(self)
		{}

		void visit(const std::string& key, EntityKeyValue& value) {
			_self.attachKeyObserver(key, value);
		}

	} attacher(*this);

	// Traverse the entity
	_entity.forEachKeyValue(attacher);
}

void NamespaceManager::detachKeyObserver(const std::string& key, EntityKeyValue& keyValue) {
	if (_namespace == NULL) return;

	if (keyIsName(key)) {
		// Destroy the NameKeyObserver object
		_nameKeyObservers.erase(&keyValue);
	}
	else {
		// Not a name key, destroy the KeyValueObserver
		_keyValueObservers.erase(&keyValue);
	}
}

void NamespaceManager::detachKeyObservers() {
	// May not be called with empty namespace
	assert(_namespace != NULL);

	// Local helper class used to traverse the keyvalues
	class Detacher :
		public Entity::KeyValueVisitor
	{
		NamespaceManager& _self;

	public:
		Detacher(NamespaceManager& self) : 
			_self(self)
		{}

		void visit(const std::string& key, EntityKeyValue& value) {
			_self.detachKeyObserver(key, value);
		}

	} detacher(*this);

	// Traverse the entity
	_entity.forEachKeyValue(detacher);
}

} // namespace entity
