#pragma once

#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <sigc++/signal.h>
#include "ientity.h"
#include "i18n.h"

namespace selection
{

/**
 * Helper class to keep the Entity Inspector key/value list view up to date 
 * when one or more entities are selected in the scene.
 * 
 * It not only knows which entity contributes which key value pair,
 * it also tracks the uniqueness of all the values for a given key.
 * 
 * The signals emitted by this class help the client code (i.e. Entity Inspector)
 * to update only those rows that actually changed their meaning.
 * 
 * Keys that are now shared but were not listed before => signal_KeyAdded
 * Values that are no longer shared will disappear from the list => signal_KeyRemoved
 * Keys changing their value (shared or not) => signal_KeyValueSetChanged
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
    using KeyValuePair = std::pair<const std::string, std::string>;
    using KeyValues = std::map<KeyValuePair::first_type, KeyValuePair::second_type>;
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

    bool containsKey(const std::string& key) const
    {
        return _entitiesByKey.count(key) > 0;
    }

    std::string getSharedKeyValue(const std::string& key)
    {
        auto existingKeySet = _entitiesByKey.find(key);

        if (existingKeySet == _entitiesByKey.end() ||
            !existingKeySet->second.valueIsEqualOnAllEntities ||
            existingKeySet->second.entities.empty())
        {
            return {};
        }

        return getCachedKeyValuePairForEntity(*existingKeySet->second.entities.begin(), key).second;
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

            // Get the stored key values for this other entity, this must succeed
            const auto& otherKeyValuePair = getCachedKeyValuePairForEntity(*otherEntity, key);

            // If the value differs, the value set changes state
            auto valueIsUnique = otherKeyValuePair.second == valueString;

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
                keyValueSet.entities.begin(), keyValueSet.entities.end(), [&](Entity* existing)
                {
                    if (entity == existing) return true; // don't check against self

                    const auto& kv = getCachedKeyValuePairForEntity(existing, key);
                    return kv.second == valueString;
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

        // On key value change, we don't need to update the entity set in _entitiesByKey
        // But since the value changed its uniqueness might have changed with it
        auto e = _entitiesByKey.find(key);

        if (e != _entitiesByKey.end())
        {
            if (e->second.entities.size() > 1)
            {
                // We have more than one entity check the set
                checkKeyValueSetAfterChange(entity, key, value, e->second);
            }
            else
            {
                // We only have one entity for this key, it should have been unique in the first place
                assert(e->second.valueIsEqualOnAllEntities);

                // Signal will be emitted nonetheless, the value got changed
                _sigKeyValueSetChanged.emit(key, value);
            }
        }
    }

    void onKeyErase(Entity* entity, const std::string& key, EntityKeyValue& value)
    {
        removeKey(entity, key);
    }

    void onEntityAdded(Entity* entity)
    {
        // There are cases that cannot be tracked on the spawnarg level only
        // like keys that are not present on the new set of selected entities
        // In these cases, those keys should disappear from the set
        for (auto& pair : _entitiesByKey)
        {
            auto numEntities = _keyValuesByEntity.size();
            auto numEntitiesForKey = pair.second.entities.size();

            // If the new entity makes an existing set incomplete, remove the key
            if (numEntities > 1 && numEntitiesForKey == numEntities - 1)
            {
                // We got more entities in the tracked set than we have values for this key
                // which was not the case before this entity was here, which means
                // we should remove it from the visible set
                pair.second.valueIsEqualOnAllEntities = false;
                _sigKeyRemoved.emit(pair.first);
            }
        }
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
        auto foundKeyValues = _keyValuesByEntity.find(entity);

        if (foundKeyValues == _keyValuesByEntity.end())
        {
            return;
        }

        // Remove the entity from the entity-to-kv mapping first
        // The entity count should be reduced when we invoke removeKey()
        // We do this by move-extracting the value pairs from the map
        KeyValues keyValues(std::move(_keyValuesByEntity.extract(foundKeyValues).mapped()));

        // Remove the entity from all key-mapped lists
        for (const auto& pair : keyValues)
        {
            removeKey(entity, pair.first);
        }

        // Removing an entity might render existing keys visible
        // if the entity was the last one lacking that key
        for (auto& pair : _entitiesByKey)
        {
            const auto& key = pair.first;
            auto& keyValueSet = pair.second;

            // Only check the keys that were lacking on the removed entity,
            // these are the only ones that might have been hidden before
            if (keyValues.count(key) > 0 ||
                keyValueSet.entities.size() != _keyValuesByEntity.size())
            {
                continue;
            }

            // The key was lacking on the entity and the key is now present
            // on all entities, check if we have a unique key
            auto sharedValue = getKeySharedByAllEntities(key);

            keyValueSet.valueIsEqualOnAllEntities = !sharedValue.empty();
            
            // Fire the signal to make that value re-appear
            _sigKeyAdded.emit(key, sharedValue);

            // In case the key has differing values, fire another signal to make it appear non-unique
            if (!keyValueSet.valueIsEqualOnAllEntities)
            {
                _sigKeyValueSetChanged.emit(key, "");
            }
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
                return;
            }

            checkKeyValueSetAfterRemoval(key, keyValueSet);
        }
    }

    void checkKeyValueSetAfterChange(Entity* entity, const std::string& key, const std::string& newValue, KeyValueSet& keyValueSet)
    {
        assert(keyValueSet.entities.size() > 1);

        // If the value was not shared before, it might have changed to be the same for all entities now
        if (!keyValueSet.valueIsEqualOnAllEntities)
        {
            // std::all_of will still return true if the range is empty
            bool valueIsUnique = std::all_of(_keyValuesByEntity.begin(), _keyValuesByEntity.end(),
                [&](const KeyValuesByEntity::value_type& pair)
            {
                // No need to compare the value of the entity that got changed
                if (entity == pair.first) return true;

                auto existingKey = pair.second.find(key);
                return existingKey != pair.second.end() && existingKey->second == newValue;
            });

            if (valueIsUnique)
            {
                keyValueSet.valueIsEqualOnAllEntities = true;
                _sigKeyValueSetChanged.emit(key, newValue);
            }
        }
        // The value was shared on all entities before, but maybe that's no longer the case
        // If the value is different to any one other existing entity, the set diverged
        else
        {
            // Pick an entity that is not the same as the entity firing this event
            auto otherEntity = std::find_if(keyValueSet.entities.begin(), keyValueSet.entities.end(),
                [&](Entity* existing) { return entity != existing; });

            // We must find such an entity, the entity set size is > 1
            assert(otherEntity != keyValueSet.entities.end());

            // Get the stored key values for this other entity, this must succeed
            const auto& otherKeyValuePair = getCachedKeyValuePairForEntity(*otherEntity, key);

            if (otherKeyValuePair.second != newValue)
            {
                // Value differs, the set is no longer sharing a single value
                keyValueSet.valueIsEqualOnAllEntities = false;
                _sigKeyValueSetChanged.emit(key, "");
            }
        }
    }

    std::string getKeySharedByAllEntities(const std::string& key) const
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

        return valueIsUnique ? remainingValue : "";
    }

    void checkKeyValueSetAfterRemoval(const std::string& key, KeyValueSet& keyValueSet)
    {
        // If the value was not shared before, this could be the case now
        if (!keyValueSet.valueIsEqualOnAllEntities)
        {
            auto sharedValue = getKeySharedByAllEntities(key);
            
            if (!sharedValue.empty())
            {
                keyValueSet.valueIsEqualOnAllEntities = true;
                _sigKeyValueSetChanged.emit(key, sharedValue);
            }
        }
        // If the value was shared on all entities before, that may be no longer the case;
        // it must still be present on *all* involved entities
        else if (keyValueSet.entities.size() != _keyValuesByEntity.size())
        {
            // Size differs, we have more entities in play than we have entities for this key
            keyValueSet.valueIsEqualOnAllEntities = false;
            _sigKeyRemoved.emit(key);
        }
    }

    // The given entity must have been tracked
    const KeyValuePair& getCachedKeyValuePairForEntity(Entity* entity, const std::string& key) const
    {
        const auto& keyValues = _keyValuesByEntity.at(entity);
        auto keyValuePair = keyValues.find(key);

        // This must also succeed, it was mapped
        assert(keyValuePair != keyValues.end());

        return *keyValuePair;
    }
};

}
