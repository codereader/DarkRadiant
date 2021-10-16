#pragma once

#include "ientity.h"
#include <map>
#include <set>
#include <string>
#include <sigc++/signal.h>

namespace selection
{

/**
 * Container keeping track of entity key value sets
 * of one or more selected entities in the scene.
 */
class CollectiveSpawnargs
{
private:
    using KeyValues = std::map<std::string, std::string>;

    // Map Entity instances to key value pairs
    std::map<Entity*, KeyValues> _keyValuesByEntity;

    // Map Keys to Entities
    std::map<std::string, std::set<Entity*>> _entitiesByKey;

    sigc::signal<void(const std::string&, const std::string&)> _sigKeyAdded;
    sigc::signal<void(const std::string&)> _sigKeyRemoved;
    sigc::signal<void(const std::string&, const std::string&)> _sigKeyValueSetChanged;

public:
    // Signal emitted when a new key is added to the collection
    sigc::signal<void(const std::string&, const std::string&)>& signal_KeyAdded()
    {
        return _sigKeyAdded;
    }

    // Emitted when the value set of the given key changes. The second argument will contain the value
    // provided it is the same for all entities
    sigc::signal<void(const std::string&, const std::string&)>& signal_KeyValueSetChanged()
    {
        return _sigKeyValueSetChanged;
    }

    sigc::signal<void(const std::string&)>& signal_KeyRemoved()
    {
        return _sigKeyRemoved;
    }

    void foreachKey(const std::function<void(const std::string&, const std::set<Entity*>&)>& functor)
    {
        for (const auto& pair : _entitiesByKey)
        {
            functor(pair.first, pair.second);
        }
    }

    void onKeyInsert(Entity* entity, const std::string& key, EntityKeyValue& value)
    {
        auto kv = _keyValuesByEntity.try_emplace(entity);
        kv.first->second.emplace(key, value.get());

        auto entityList = _entitiesByKey.try_emplace(key);
        auto result = entityList.first->second.emplace(entity);

        if (result.second)
        {
            // The set was newly created, this was a new (and therefore unique) keyvalue
            _sigKeyAdded.emit(key, value.get());
        }
        else
        {
            // The set was already present, check if the values are unique
            // TODO
        }
    }

    void onKeyChange(Entity* entity, const std::string& key, const std::string& value)
    {
        auto kv = _keyValuesByEntity.try_emplace(entity);
        kv.first->second[key] = value;

        // On key change, we don't need to update the entitiesByKey map
    }

    void onKeyErase(Entity* entity, const std::string& key, EntityKeyValue& value)
    {
        auto kv = _keyValuesByEntity.try_emplace(entity);
        kv.first->second.erase(key);

        auto entityList = _entitiesByKey.find(key);

        if (entityList != _entitiesByKey.end())
        {
            entityList->second.erase(entity);

            if (entityList->second.empty())
            {
                _entitiesByKey.erase(entityList);
                // This was the last occurrence of this key, remove it
                _sigKeyRemoved.emit(key);
            }
        }
    }

    void cleanupEntity(Entity* entity)
    {
        _keyValuesByEntity.erase(entity);

        // Remove the entity from all key-mapped lists
        for (auto& entityList : _entitiesByKey)
        {
            entityList.second.erase(entity);
        }
    }
};

}
