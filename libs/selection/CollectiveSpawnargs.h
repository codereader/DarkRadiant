#pragma once

#include "ientity.h"
#include <map>
#include <set>
#include <algorithm>
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
public:
    struct KeyValueSet
    {
        KeyValueSet() :
            valueIsEqualOnAllEntities(false)
        {}

        bool valueIsEqualOnAllEntities;
        std::set<Entity*> entities;
    };

private:
    using KeyValues = std::map<std::string, std::string>;

    // Map Entity instances to key value pairs
    std::map<Entity*, KeyValues> _keyValuesByEntity;

    // Map Keys to KeyValueSets
    std::map<std::string, KeyValueSet> _entitiesByKey;

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

    void foreachKey(const std::function<void(const std::string&, const KeyValueSet&)>& functor)
    {
        for (const auto& pair : _entitiesByKey)
        {
            functor(pair.first, pair.second);
        }
    }

    void onKeyInsert(Entity* entity, const std::string& key, EntityKeyValue& value)
    {
        const auto& valueString = value.get();

        auto kv = _keyValuesByEntity.try_emplace(entity);
        kv.first->second.emplace(key, valueString);

        auto entityList = _entitiesByKey.try_emplace(key);

        auto& keyValueSet = entityList.first->second;
        keyValueSet.entities.emplace(entity);

        if (keyValueSet.entities.size() == 1)
        {
            // The set was newly created, this was a new (and therefore unique) keyvalue
            keyValueSet.valueIsEqualOnAllEntities = true;
            _sigKeyAdded.emit(key, value.get());
        }
        // This was not the first entity using this key, check if the values are unique
        // We only bother checking if the already existing set had the same value
        else if (keyValueSet.valueIsEqualOnAllEntities)
        {
            // Value was the shared before, let's check it now
            auto valueIsUnique = std::all_of(
                keyValueSet.entities.begin(), keyValueSet.entities.end(), [&](Entity* entity)
                {
                    return entity->getKeyValue(key) == valueString;
                });

            if (!valueIsUnique)
            {
                keyValueSet.valueIsEqualOnAllEntities = false;
                _sigKeyValueSetChanged.emit(key, "");
            }
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
        const auto& valueString = value.get();

        auto kv = _keyValuesByEntity.find(entity);

        if (kv != _keyValuesByEntity.end())
        {
            kv->second.erase(key);

            if (kv->second.empty())
            {
                // The entity-specific map is now empty, remove it
                _keyValuesByEntity.erase(kv);
            }
        }

        auto entityList = _entitiesByKey.find(key);

        if (entityList != _entitiesByKey.end())
        {
            auto& keyValueSet = entityList->second;

            keyValueSet.entities.erase(entity);

            if (keyValueSet.entities.empty())
            {
                // This was the last occurrence of this key, remove it
                _entitiesByKey.erase(entityList);
                _sigKeyRemoved.emit(key);
            }
            // If the value was not shared before, this could be the case now
            else if (!keyValueSet.valueIsEqualOnAllEntities)
            {
                auto firstEntity = keyValueSet.entities.begin();
                auto remainingValue = (*firstEntity)->getKeyValue(key);

                // Skip the first entity and check the others for uniqueness
                // std::all_of will still return true if the range is empty
                bool valueIsUnique = std::all_of(
                    ++firstEntity, keyValueSet.entities.end(), [&](Entity* entity)
                {
                    return entity->getKeyValue(key) == remainingValue;
                });

                if (valueIsUnique)
                {
                    keyValueSet.valueIsEqualOnAllEntities = true;
                    _sigKeyValueSetChanged.emit(key, remainingValue);
                }
            }
            // The value was shared on all entities before, but maybe that's no longer the case
            // it must still be present on *all* involved entities
            else if (keyValueSet.entities.size() != _keyValuesByEntity.size())
            {
                // Size differs, we have more entities in play than we have entities for this key
                keyValueSet.valueIsEqualOnAllEntities = false;
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
            entityList.second.entities.erase(entity);

            // TODO: Check uniqueness
        }
    }
};

}
