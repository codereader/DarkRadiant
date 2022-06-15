#pragma once

#include <vector>
#include "ieclass.h"
#include "ientity.h"

namespace test
{

namespace algorithm
{

inline std::vector<std::pair<std::string, std::string>> getAllKeyValuePairs(Entity* entity)
{
    std::vector<std::pair<std::string, std::string>> existingKeyValues;

    entity->forEachKeyValue([&](const std::string& key, const std::string& value)
    {
        existingKeyValues.emplace_back(key, value);
    });

    return existingKeyValues;
}

// Create an entity from a simple classname string
inline IEntityNodePtr createEntityByClassName(const std::string& className)
{
    auto cls = GlobalEntityClassManager().findClass(className);
    return GlobalEntityModule().createEntity(cls);
}

}

}
