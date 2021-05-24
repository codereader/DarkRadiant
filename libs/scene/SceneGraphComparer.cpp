#include "SceneGraphComparer.h"

#include <algorithm>
#include "ientity.h"
#include "i18n.h"
#include "itextstream.h"
#include "icomparablenode.h"
#include "string/string.h"
#include "command/ExecutionNotPossible.h"

namespace scene
{

namespace
{
    inline std::string getEntityName(const scene::INodePtr& node)
    {
        auto entity = Node_getEntity(node);
        
        return entity->isWorldspawn() ? "worldspawn" : entity->getKeyValue("name");
    }
}

void SceneGraphComparer::compare()
{
    auto sourceEntities = collectEntityFingerprints(_source);
    auto baseEntities = collectEntityFingerprints(_base);

    // Filter out all the matching nodes and store them in the result
    if (sourceEntities.empty())
    {
        // Cannot merge the source without any entities in it
        throw cmd::ExecutionNotPossible(_("The source map doesn't contain any entities, cannot merge"));
    }

    EntityMismatchByName sourceMismatches;

    for (const auto& sourceEntity : sourceEntities)
    {
        // Check each source node for an equivalent node in the base
        auto matchingBaseNode = baseEntities.find(sourceEntity.first);

        if (matchingBaseNode != baseEntities.end())
        {
            // Found an equivalent node
            _result->equivalentEntities.emplace_back(ComparisonResult::Match{ sourceEntity.first, sourceEntity.second, matchingBaseNode->second });
        }
        else
        {
            auto entityName = getEntityName(sourceEntity.second);
            sourceMismatches.emplace(entityName, EntityMismatch{ sourceEntity.first, sourceEntity.second, entityName });
        }
    }

    EntityMismatchByName baseMismatches;

    for (const auto& baseEntity : baseEntities)
    {
        // Check each source node for an equivalent node in the base
        // Matching nodes have already been checked in the above loop
        if (sourceEntities.count(baseEntity.first) == 0)
        {
            auto entityName = getEntityName(baseEntity.second);
            baseMismatches.emplace(entityName, EntityMismatch{ baseEntity.first, baseEntity.second, entityName });
        }
    }

    rMessage() << "Mismatching Source Entities: " << sourceMismatches.size() << std::endl;
    rMessage() << "Mismatching Base Entities: " << baseMismatches.size() << std::endl;

    // Enter the second stage and try to match entities and detailing diffs
    processDifferingEntities(sourceMismatches, baseMismatches);
}

void SceneGraphComparer::processDifferingEntities(const EntityMismatchByName& sourceMismatches, const EntityMismatchByName& baseMismatches)
{
    // Find all entities that are missing in either source or base (by name)
    std::list<EntityMismatchByName::value_type> missingInSource;
    std::list<EntityMismatchByName::value_type> missingInBase;

    std::list<EntityMismatchByName::value_type> matchingByName;

    auto compareEntityNames = [](const EntityMismatchByName::value_type& left, const EntityMismatchByName::value_type& right)
    {
        return left.first < right.first;
    };

    // Calculate intersection and two-way exclusives
    std::set_intersection(sourceMismatches.begin(), sourceMismatches.end(), baseMismatches.begin(), baseMismatches.end(),
        std::back_inserter(matchingByName), compareEntityNames);

    std::set_difference(sourceMismatches.begin(), sourceMismatches.end(), baseMismatches.begin(), baseMismatches.end(),
        std::back_inserter(missingInBase), compareEntityNames);

    std::set_difference(baseMismatches.begin(), baseMismatches.end(), sourceMismatches.begin(), sourceMismatches.end(),
        std::back_inserter(missingInSource), compareEntityNames);

    for (const auto& match : matchingByName)
    {
        auto& entityDiff = _result->differingEntities.emplace_back(ComparisonResult::EntityDifference
        {
            match.second.fingerPrint,
            match.second.node,
            match.second.entityName,
            ComparisonResult::EntityDifference::Type::EntityPresentButDifferent
        });

        auto sourceNode = sourceMismatches.find(match.second.entityName)->second.node;
        auto baseNode = baseMismatches.find(match.second.entityName)->second.node;

        // Analyse the key values
        entityDiff.differingKeyValues = compareKeyValues(sourceNode, baseNode);

        // Analyse the child nodes
        entityDiff.differingChildren = compareChildNodes(sourceNode, baseNode);
    }

    for (const auto& mismatch : missingInSource)
    {
        _result->differingEntities.emplace_back(ComparisonResult::EntityDifference
        {
            mismatch.second.fingerPrint,
            mismatch.second.node,
            mismatch.second.entityName,
            ComparisonResult::EntityDifference::Type::EntityMissingInSource
        });
    }

    for (const auto& mismatch : missingInBase)
    {
        _result->differingEntities.emplace_back(ComparisonResult::EntityDifference
        {
            mismatch.second.fingerPrint,
            mismatch.second.node,
            mismatch.second.entityName,
            ComparisonResult::EntityDifference::Type::EntityMissingInBase
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
    const scene::INodePtr& sourceNode, const scene::INodePtr& baseNode)
{
    std::list<ComparisonResult::KeyValueDifference> result;

    auto sourceKeyValues = loadKeyValues(sourceNode);
    auto baseKeyValues = loadKeyValues(baseNode);

    string::ILess icmp;
    auto compareKeysNoCase = [&](const KeyValueMap::value_type& left, const KeyValueMap::value_type& right)
    {
        return icmp(left.first, right.first);
    };

    std::vector<KeyValueMap::value_type> missingInSource;
    std::vector<KeyValueMap::value_type> missingInBase;
    std::vector<KeyValueMap::value_type> presentInBoth;

    std::set_intersection(sourceKeyValues.begin(), sourceKeyValues.end(),
        baseKeyValues.begin(), baseKeyValues.end(), std::back_inserter(presentInBoth), compareKeysNoCase);
    std::set_difference(sourceKeyValues.begin(), sourceKeyValues.end(),
        baseKeyValues.begin(), baseKeyValues.end(), std::back_inserter(missingInBase), compareKeysNoCase);
    std::set_difference(baseKeyValues.begin(), baseKeyValues.end(),
        sourceKeyValues.begin(), sourceKeyValues.end(), std::back_inserter(missingInSource), compareKeysNoCase);

    for (const auto& pair : missingInBase)
    {
        result.emplace_back(ComparisonResult::KeyValueDifference
        {
            pair.first,
            pair.second,
            ComparisonResult::KeyValueDifference::Type::KeyValueAdded
        });
    }

    for (const auto& pair : missingInSource)
    {
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
        if (sourceKeyValues[pair.first] == baseKeyValues[pair.first])
        {
            continue;
        }

        result.emplace_back(ComparisonResult::KeyValueDifference
        {
            pair.first,
            pair.second,
            ComparisonResult::KeyValueDifference::Type::KeyValueChanged
        });
    }

    return result;
}

std::list<ComparisonResult::PrimitiveDifference> SceneGraphComparer::compareChildNodes(
    const scene::INodePtr& sourceNode, const scene::INodePtr& baseNode)
{
    std::list<ComparisonResult::PrimitiveDifference> result;

    auto sourceChildren = collectPrimitiveFingerprints(sourceNode);
    auto baseChildren = collectPrimitiveFingerprints(baseNode);

    std::vector<Fingerprints::value_type> missingInSource;
    std::vector<Fingerprints::value_type> missingInBase;

    auto compareFingerprint = [](const Fingerprints::value_type& left, const Fingerprints::value_type& right)
    {
        return left.first < right.first;
    };

    std::set_difference(sourceChildren.begin(), sourceChildren.end(),
        baseChildren.begin(), baseChildren.end(), std::back_inserter(missingInBase), compareFingerprint);
    std::set_difference(baseChildren.begin(), baseChildren.end(),
        sourceChildren.begin(), sourceChildren.end(), std::back_inserter(missingInSource), compareFingerprint);

    for (const auto& pair : missingInBase)
    {
        result.emplace_back(ComparisonResult::PrimitiveDifference
        {
            pair.first,
            pair.second,
            ComparisonResult::PrimitiveDifference::Type::PrimitiveAdded
        });
    }

    for (const auto& pair : missingInSource)
    {
        result.emplace_back(ComparisonResult::PrimitiveDifference
        {
            pair.first,
            pair.second,
            ComparisonResult::PrimitiveDifference::Type::PrimitiveRemoved
        });
    }

    return result;
}

SceneGraphComparer::Fingerprints SceneGraphComparer::collectNodeFingerprints(const scene::INodePtr& parent, 
    const std::function<bool(const scene::INodePtr& node)>& nodePredicate)
{
    Fingerprints result;

    parent->foreachNode([&](const scene::INodePtr& node)
    {
        if (!nodePredicate(node)) return true; // predicate says "skip"

        auto comparable = std::dynamic_pointer_cast<scene::IComparableNode>(node);
        assert(comparable);

        if (!comparable) return true; // skip

        // Store the fingerprint and check for collisions
        auto insertResult = result.try_emplace(comparable->getFingerprint(), node);

        if (!insertResult.second)
        {
            rWarning() << "More than one node with the same fingerprint found in the parent node with name " << parent->name() << std::endl;
        }

        return true;
    });

    return result;
}

SceneGraphComparer::Fingerprints SceneGraphComparer::collectPrimitiveFingerprints(const scene::INodePtr& parent)
{
    return collectNodeFingerprints(parent, [](const scene::INodePtr& node)
    {
        return node->getNodeType() == scene::INode::Type::Brush || node->getNodeType() == scene::INode::Type::Patch;
    });
}

SceneGraphComparer::Fingerprints SceneGraphComparer::collectEntityFingerprints(const scene::INodePtr& root)
{
    return collectNodeFingerprints(root, [](const scene::INodePtr& node)
    {
        return node->getNodeType() == scene::INode::Type::Entity;
    });
}

}