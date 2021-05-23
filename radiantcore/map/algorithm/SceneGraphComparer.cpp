#include "SceneGraphComparer.h"

#include <algorithm>
#include "icomparablenode.h"
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

    std::set_intersection(sourceMismatches.begin(), sourceMismatches.end(), targetMismatches.begin(), targetMismatches.end(),
        std::back_inserter(matchingByName), compareEntityNames);

    std::set_difference(sourceMismatches.begin(), sourceMismatches.end(), targetMismatches.begin(), targetMismatches.end(),
        std::back_inserter(missingInTarget), compareEntityNames);

    std::set_difference(targetMismatches.begin(), targetMismatches.end(), sourceMismatches.begin(), sourceMismatches.end(),
        std::back_inserter(missingInSource), compareEntityNames);

    for (const auto& match : matchingByName)
    {
        rMessage() << " - EntityPresentButDifferent: " << match.first << std::endl;

        _result->differingEntities.emplace_back(ComparisonResult::EntityDifference
        {
            match.second.fingerPrint,
            match.second.node,
            match.second.entityName,
            ComparisonResult::EntityDifference::Type::EntityPresentButDifferent
        });
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