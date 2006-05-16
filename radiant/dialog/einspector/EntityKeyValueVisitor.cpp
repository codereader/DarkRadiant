#include "EntityKeyValueVisitor.h"

#include <iostream>

EntityKeyValueVisitor::EntityKeyValueVisitor()
{
}

// Visit function

void EntityKeyValueVisitor::visit(const char* key, const char* value) {
    _keyValueMap.insert(std::pair<const std::string, const std::string>(key, value));
}
