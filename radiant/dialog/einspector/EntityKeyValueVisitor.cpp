#include "EntityKeyValueVisitor.h"

#include "exception/InvalidKeyException.h"

#include <iostream>

EntityKeyValueVisitor::EntityKeyValueVisitor()
{
}

// Visit function

void EntityKeyValueVisitor::visit(const char* key, const char* value) {
    _keyValueMap.insert(std::pair<const std::string, const std::string>(key, value));
}

// Static function to find the given key on the given Entity

const std::string& EntityKeyValueVisitor::getKeyValue(Entity* entity, const std::string& key) {
	EntityKeyValueVisitor vis;
	entity->forEachKeyValue(vis);
	KeyValueMap::iterator iter = vis.getMap().find(key);
	if (iter != vis.getMap().end()) {
		return iter->second;
	} else {
		throw InvalidKeyException();
	}
}
