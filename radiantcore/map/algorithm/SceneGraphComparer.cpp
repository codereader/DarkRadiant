#include "SceneGraphComparer.h"

#include <algorithm>
#include "icomparablenode.h"
#include "string/string.h"
#include "command/ExecutionNotPossible.h"

namespace map
{

namespace algorithm
{

void SceneGraphComparer::compare()
{
    auto sourceEntities = collectEntityFingerprints(_source);
    auto targetEntities = collectEntityFingerprints(_target);

    rMessage() << "Source Node Types: " << sourceEntities.size() << std::endl;
    rMessage() << "Target Node Types: " << targetEntities.size() << std::endl;

    // Filter out all the matching nodes and store them in the result
    if (sourceEntities.empty())
    {
        // Cannot merge the source without any entities in it
        throw cmd::ExecutionNotPossible(_("The source map doesn't contain any entities, cannot merge"));
    }

    rMessage() << "Entity Fingerprints in source: " << sourceEntities.size() << std::endl;

    EntityMismatchByName sourceMismatches;

    for (const auto& sourceEntity : sourceEntities)
    {
        // Check each source node for an equivalent node in the target
        auto matchingTargetNode = targetEntities.find(sourceEntity.first);

        if (matchingTargetNode != targetEntities.end())
        {
            // Found an equivalent node
            _result->equivalentEntities.emplace_back(ComparisonResult::Match{ sourceEntity.first, sourceEntity.second, matchingTargetNode->second });
        }
        else
        {
            auto entityName = Node_getEntity(sourceEntity.second)->getKeyValue("name");

            sourceMismatches.emplace(entityName, EntityMismatch{ sourceEntity.first, sourceEntity.second, entityName });
        }
    }

    EntityMismatchByName targetMismatches;

    for (const auto& targetEntity : targetEntities)
    {
        // Check each source node for an equivalent node in the target
        // Matching nodes have already been checked in the above loop
        if (sourceEntities.count(targetEntity.first) == 0)
        {
            auto entityName = Node_getEntity(targetEntity.second)->getKeyValue("name");

            targetMismatches.emplace(entityName, EntityMismatch{ targetEntity.first, targetEntity.second, entityName });
        }
    }

    rMessage() << "Equivalent Entities " << _result->equivalentEntities.size() << std::endl;

    for (const auto& match : _result->equivalentEntities)
    {
        rMessage() << " - Equivalent Entity: " << match.sourceNode->name() << std::endl;
    }

    rMessage() << "Mismatching Source Entities: " << sourceMismatches.size() << std::endl;
    rMessage() << "Mismatching Target Entities: " << targetMismatches.size() << std::endl;

    // Enter the second stage and try to match entities and detailing diffs
    processDifferingEntities(sourceMismatches, targetMismatches);
}

void SceneGraphComparer::processDifferingEntities(const EntityMismatchByName& sourceMismatches, const EntityMismatchByName& targetMismatches)
{
    // Find all entities that are missing in either source or target (by name)
    std::list<EntityMismatchByName::value_type> missingInSource;
    std::list<EntityMismatchByName::value_type> missingInTarget;

    std::list<EntityMismatchByName::value_type> matchingByName;

    auto compareEntityNames = [](const EntityMismatchByName::value_type& left, const EntityMismatchByName::value_type& right)
    {
        return left.first < right.first;
    };

    // Calculate intersection and two-way exclusives
    std::set_intersection(sourceMismatches.begin(), sourceMismatches.end(), targetMismatches.begin(), targetMismatches.end(),
        std::back_inserter(matchingByName), compareEntityNames);

    std::set_difference(sourceMismatches.begin(), sourceMismatches.end(), targetMismatches.begin(), targetMismatches.end(),
        std::back_inserter(missingInTarget), compareEntityNames);

    std::set_difference(targetMismatches.begin(), targetMismatches.end(), sourceMismatches.begin(), sourceMismatches.end(),
        std::back_inserter(missingInSource), compareEntityNames);

    for (const auto& match : matchingByName)
    {
        rMessage() << " - EntityPresentButDifferent: " << match.first << std::endl;

        auto& entityDiff = _result->differingEntities.emplace_back(ComparisonResult::EntityDifference
        {
            match.second.fingerPrint,
            match.second.node,
            match.second.entityName,
            ComparisonResult::EntityDifference::Type::EntityPresentButDifferent
        });

        auto sourceNode = sourceMismatches.find(match.second.entityName)->second.node;
        auto targetNode = targetMismatches.find(match.second.entityName)->second.node;

        // Analyse the key values
        entityDiff.differingKeyValues = compareKeyValues(sourceNode, targetNode);
    }

    for (const auto& mismatch : missingInSource)
    {
        rMessage() << " - EntityMissingInSource: " << mismatch.first << std::endl;

        _result->differingEntities.emplace_back(ComparisonResult::EntityDifference
        {
            mismatch.second.fingerPrint,
            mismatch.second.node,
            mismatch.second.entityName,
            ComparisonResult::EntityDifference::Type::EntityMissingInSource
        });
    }

    for (const auto& mismatch : missingInTarget)
    {
        rMessage() << " - EntityMissingInTarget: " << mismatch.first << std::endl;

        _result->differingEntities.emplace_back(ComparisonResult::EntityDifference
        {
            mismatch.second.fingerPrint,
            mismatch.second.node,
            mismatch.second.entityName,
            ComparisonResult::EntityDifference::Type::EntityMissingInTarget
        });
    }
}

namespace
{
    using KeyValueMap = std::map<std::string, std::string, string::ILess>;

    inline KeyValueMap loadKeyValues(const scene::INodePtr& entityNode)
    {
        KeyValueMap result;

        auto entity = Node_getEntity(entityNode);

        entity->forEachKeyValue([&](const std::string& key, const std::string& value)
        {
            result.emplace(key, value);
        }, false);

        return result;
    }
}

std::list<ComparisonResult::KeyValueDifference> SceneGraphComparer::compareKeyValues(
    const scene::INodePtr& sourceNode, const scene::INodePtr& targetNode)
{
    std::list<ComparisonResult::KeyValueDifference> result;

    auto sourceKeyValues = loadKeyValues(sourceNode);
    auto targetKeyValues = loadKeyValues(targetNode);

    string::ILess icmp;
    auto compareKeysNoCase = [&](const KeyValueMap::value_type& left, const KeyValueMap::value_type& right)
    {
        return icmp(left.first, right.first);
    };

    std::vector<KeyValueMap::value_type> missingInSource;
    std::vector<KeyValueMap::value_type> missingInTarget;
    std::vector<KeyValueMap::value_type> presentInBoth;

    std::set_intersection(sourceKeyValues.begin(), sourceKeyValues.end(),
        targetKeyValues.begin(), targetKeyValues.end(), std::back_inserter(presentInBoth), compareKeysNoCase);
    std::set_difference(sourceKeyValues.begin(), sourceKeyValues.end(),
        targetKeyValues.begin(), targetKeyValues.end(), std::back_inserter(missingInTarget), compareKeysNoCase);
    std::set_difference(targetKeyValues.begin(), targetKeyValues.end(),
        sourceKeyValues.begin(), sourceKeyValues.end(), std::back_inserter(missingInSource), compareKeysNoCase);

    for (const auto& pair : missingInTarget)
    {
        rMessage() << " - Key " << pair.first << " not present in target entity" << std::endl;

        result.emplace_back(ComparisonResult::KeyValueDifference
        {
            pair.first,
            pair.second,
            ComparisonResult::KeyValueDifference::Type::KeyValueAdded
        });
    }

    for (const auto& pair : missingInSource)
    {
        rMessage() << " - Key " << pair.first << " not present in source entity" << std::endl;

        result.emplace_back(ComparisonResult::KeyValueDifference
        {
            pair.first,
            pair.second,
            ComparisonResult::KeyValueDifference::Type::KeyValueRemoved
        });
    }

    // Compare each value which is present on both entities
    for (const auto& pair : presentInBoth)
    {
        if (sourceKeyValues[pair.first] == targetKeyValues[pair.first])
        {
            continue;
        }

        rMessage() << " - Key " << pair.first << " changed in source to value " << pair.second << std::endl;

        result.emplace_back(ComparisonResult::KeyValueDifference
        {
            pair.first,
            pair.second,
            ComparisonResult::KeyValueDifference::Type::KeyValueChanged
        });
    }

    return result;
}

SceneGraphComparer::Fingerprints SceneGraphComparer::collectEntityFingerprints(const scene::INodePtr& root)
{
    Fingerprints result;

    root->foreachNode([&](const scene::INodePtr& node)
    {
        if (node->getNodeType() != scene::INode::Type::Entity)
        {
            return true;
        }

        auto comparable = std::dynamic_pointer_cast<scene::IComparableNode>(node);

        if (!comparable) return true; // skip

        // Store the fingerprint and check for collisions
        auto insertResult = result.try_emplace(comparable->getFingerprint(), node);

        if (!insertResult.second)
        {
            rWarning() << "More than one entity with the same fingerprint found in the map " << node->name() << std::endl;
        }

        return true;
    });

    return result;
}

}

}