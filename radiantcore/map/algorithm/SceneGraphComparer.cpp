#include "SceneGraphComparer.h"

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
            _result->differingEntities.emplace_back(ComparisonResult::Mismatch{ sourceEntity.first, sourceEntity.second });
        }
    }

    for (const auto& targetEntity : targetEntities)
    {
        // Check each source node for an equivalent node in the target
        // Matching nodes have already been checked in the above loop
        if (sourceEntities.count(targetEntity.first) == 0)
        {
            _result->differingEntities.emplace_back(ComparisonResult::Mismatch{ targetEntity.first, scene::INodePtr(), targetEntity.second });
        }
    }

    rMessage() << "Equivalent Entities " << _result->equivalentEntities.size() << std::endl;

    for (const auto& match : _result->equivalentEntities)
    {
        rMessage() << " - Equivalent Entity: " << match.sourceNode->name() << std::endl;
    }

    rMessage() << "Mismatching Entities: " << _result->differingEntities.size() << std::endl;

    for (const auto& mismatch : _result->differingEntities)
    {
        if (mismatch.sourceNode)
        {
            rMessage() << " - No match found for source entity: " << mismatch.sourceNode->name() << std::endl;
        }
        else if (mismatch.targetNode)
        {
            rMessage() << " - No match found for target entity " << mismatch.targetNode->name() << std::endl;
        }
    }
}

SceneGraphComparer::Fingerprints SceneGraphComparer::collectEntityFingerprints(const scene::INodePtr& root)
{
    Fingerprints result;

    root->foreachNode([&](const scene::INodePtr& node)
    {
        if (node->getNodeType() != scene::INode::Type::Entity)
        {
            return false;
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