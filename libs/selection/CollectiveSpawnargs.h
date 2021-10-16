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
    using KeyValuesByEntity = std::map<Entity*, KeyValues>;

    // Map Entity instances to key value pairs
    KeyValuesByEntity _keyValuesByEntity;

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
            // Pick an entity that is not the same as the entity firing this event
            auto otherEntity = std::find_if(keyValueSet.entities.begin(), keyValueSet.entities.end(),
                [&](Entity* existing) { return entity != existing; });

            // We must find such an entity, the entity set size is > 1
            assert(otherEntity != keyValueSet.entities.end());

            // If the value differs, the value set changes state
            auto valueIsUnique = (*otherEntity)->getKeyValue(key) == valueString;

            if (!valueIsUnique)
            {
                keyValueSet.valueIsEqualOnAllEntities = false;
                _sigKeyValueSetChanged.emit(key, "");
            }
        }
        // The value was not equal for all entities up till now, 
        // but may be this entity is completing the set
        // Only bother checking if the entity count is the same as the value count
        else if (keyValueSet.entities.size() == _keyValuesByEntity.size())
        {
            auto valueIsUnique = std::all_of(
                keyValueSet.entities.begin(), keyValueSet.entities.end(), [&](Entity* entity)
                {
                    return entity->getKeyValue(key) == valueString;
                });

            if (valueIsUnique)
            {
                keyValueSet.valueIsEqualOnAllEntities = true;
                _sigKeyValueSetChanged.emit(key, valueString);
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
        removeKey(entity, key);
    }

    void onEntityCountChanged()
    {
        // There are cases that cannot be tracked on the spawnarg level only
        // like keys that are not present on the new set of selected entities
        // In these cases, those keys should disappear from the set
        for (auto& pair : _entitiesByKey)
        {
            if (pair.second.valueIsEqualOnAllEntities &&
                pair.second.entities.size() != _keyValuesByEntity.size())
            {
                // We got more entities in the tracked set than we have values for this key
                // This means it should be removed from the visible set
                pair.second.valueIsEqualOnAllEntities = false;
                _sigKeyRemoved.emit(pair.first);
            }
        }
    }

    void onEntityRemoved(Entity* entity)
    {
        // Don't de-reference the entity pointer, it might have been erased already
        
        // Remove the entity from the entity-to-kv mapping first
        // The entity count should be reduced when we invoke removeKey()
        // We do this by move-extracting the value pairs from the map
        KeyValues keyValues(std::move(_keyValuesByEntity.extract(entity).mapped()));

        // Remove the entity from all key-mapped lists
        for (const auto& pair : keyValues)
        {
            removeKey(entity, pair.first);
        }
    }

private:
    void removeKey(Entity* entity, const std::string& key)
    {
        // The incoming Entity* pointer is only used as key and should not be de-referenced
        // as the owning scene::Node might already have turned its toes up to the daisies

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
                // Get the key value of the first remaining entity
                auto firstEntity = _keyValuesByEntity.begin();
                auto firstKey = firstEntity->second.find(key);

                // For comparison it's enough to fall back to an empty value if the key is not present
                auto remainingValue = firstKey != firstEntity->second.end() ? firstKey->second : "";

                // Skip beyond the first entity and check the rest for uniqueness
                // std::all_of will still return true if the range is empty
                bool valueIsUnique = std::all_of(++firstEntity, _keyValuesByEntity.end(),
                    [&](const KeyValuesByEntity::value_type& pair)
                    {
                        auto existingKey = pair.second.find(key);
                        return existingKey != pair.second.end() && existingKey->second == remainingValue;
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
};

}
