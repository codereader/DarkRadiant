#pragma once

#include <vector>
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

}

}
