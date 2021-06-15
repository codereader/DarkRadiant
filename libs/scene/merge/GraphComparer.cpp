#include "GraphComparer.h"

#include <algorithm>
#include "ientity.h"
#include "i18n.h"
#include "itextstream.h"
#include "iselectiongroup.h"
#include "icomparablenode.h"
#include "math/Hash.h"
#include "scenelib.h"
#include "string/string.h"
#include "command/ExecutionNotPossible.h"
#include "NodeUtils.h"

namespace scene
{

namespace merge
{

ComparisonResult::Ptr GraphComparer::Compare(const IMapRootNodePtr& source, const IMapRootNodePtr& base)
{
    auto result = std::make_shared<ComparisonResult>(source, base);

    auto sourceEntities = NodeUtils::CollectEntityFingerprints(source);
    auto baseEntities = NodeUtils::CollectEntityFingerprints(base);

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
            result->equivalentEntities.emplace_back(ComparisonResult::Match{ sourceEntity.first, sourceEntity.second, matchingBaseNode->second });
        }
        else
        {
            auto entityName = NodeUtils::GetEntityName(sourceEntity.second);
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
            auto entityName = NodeUtils::GetEntityName(baseEntity.second);
            baseMismatches.emplace(entityName, EntityMismatch{ baseEntity.first, baseEntity.second, entityName });
        }
    }

    // Enter the second stage and try to match entities and detailing diffs
    processDifferingEntities(*result, sourceMismatches, baseMismatches);

    // Compare the group configurations of all nodes
    compareSelectionGroups(*result);

    return result;
}

void GraphComparer::processDifferingEntities(ComparisonResult& result, const EntityMismatchByName& sourceMismatches, const EntityMismatchByName& baseMismatches)
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
        auto sourceMismatch = sourceMismatches.find(match.second.entityName)->second;
        auto baseMismatch = baseMismatches.find(match.second.entityName)->second;

        auto& entityDiff = result.differingEntities.emplace_back(ComparisonResult::EntityDifference
        {
            sourceMismatch.node,
            baseMismatch.node,
            match.second.entityName,
            sourceMismatch.fingerPrint,
            baseMismatch.fingerPrint,
            ComparisonResult::EntityDifference::Type::EntityPresentButDifferent
        });

        // Analyse the key values
        entityDiff.differingKeyValues = compareKeyValues(sourceMismatch.node, baseMismatch.node);

        // Analyse the child nodes
        entityDiff.differingChildren = compareChildNodes(sourceMismatch.node, baseMismatch.node);
    }

    for (const auto& mismatch : missingInSource)
    {
        result.differingEntities.emplace_back(ComparisonResult::EntityDifference
        {
            INodePtr(), // source node is empty
            mismatch.second.node,
            mismatch.second.entityName,
            std::string(),// source fingerprint is empty
            mismatch.second.fingerPrint, // base fingerprint
            ComparisonResult::EntityDifference::Type::EntityMissingInSource
        });
    }

    for (const auto& mismatch : missingInBase)
    {
        result.differingEntities.emplace_back(ComparisonResult::EntityDifference
        {
            mismatch.second.node,
            INodePtr(), // base node is empty
            mismatch.second.entityName,
            mismatch.second.fingerPrint, // source fingerprint
            std::string(),// base fingerprint is empty
            ComparisonResult::EntityDifference::Type::EntityMissingInBase
        });
    }
}

namespace
{
    using KeyValueMap = std::map<std::string, std::string, string::ILess>;

    inline KeyValueMap loadKeyValues(const INodePtr& entityNode)
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

std::list<ComparisonResult::KeyValueDifference> GraphComparer::compareKeyValues(
    const INodePtr& sourceNode, const INodePtr& baseNode)
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

std::list<ComparisonResult::PrimitiveDifference> GraphComparer::compareChildNodes(
    const INodePtr& sourceNode, const INodePtr& baseNode)
{
    std::list<ComparisonResult::PrimitiveDifference> result;

    auto sourceChildren = NodeUtils::CollectPrimitiveFingerprints(sourceNode);
    auto baseChildren = NodeUtils::CollectPrimitiveFingerprints(baseNode);

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

void GraphComparer::compareSelectionGroups(ComparisonResult& result)
{
    // Compare all matching entities first, their primitives are matching
    for (const auto& matchingEntity : result.equivalentEntities)
    {
        compareSelectionGroups(result, matchingEntity.sourceNode, matchingEntity.baseNode);

        // Each node of the matching source entity must have a counter-part in the base entity
        compareSelectionGroupsOfPrimitives(result, matchingEntity.sourceNode, matchingEntity.baseNode);
    }

    // Compare mismatching entities that have a counterpart in the base map
    for (const auto& mismatchingEntity : result.differingEntities)
    {
        // We only compare entities which are present in both maps
        if (!mismatchingEntity.sourceNode || !mismatchingEntity.baseNode)
        {
            continue;
        }

        compareSelectionGroups(result, mismatchingEntity.sourceNode, mismatchingEntity.baseNode);

        // Check each child of the mismatching source entity, it might have counter-parts in the base map
        compareSelectionGroupsOfPrimitives(result, mismatchingEntity.sourceNode, mismatchingEntity.baseNode);
    }
}

void GraphComparer::compareSelectionGroupsOfPrimitives(ComparisonResult& result, const INodePtr& sourceNode, const INodePtr& baseNode)
{
    // Check each node of the mismatching source entity, it might have counter-parts in the base map
    auto sourcePrimitives = NodeUtils::CollectPrimitiveFingerprints(sourceNode);
    auto basePrimitives = NodeUtils::CollectPrimitiveFingerprints(baseNode);

    for (const auto& pair : sourcePrimitives)
    {
        // Look up the counterart in the base map and compare
        const auto& sourcePrimitive = pair.second;
        auto counterpart = basePrimitives.find(pair.first);

        if (counterpart != basePrimitives.end())
        {
            const auto& basePrimitive = counterpart->second;
            compareSelectionGroups(result, sourcePrimitive, basePrimitive);
        }
    }
}

void GraphComparer::compareSelectionGroups(ComparisonResult& result, const INodePtr& sourceNode, const INodePtr& baseNode)
{
    // The nodes should be group selectable
    assert(std::dynamic_pointer_cast<IGroupSelectable>(sourceNode));
    assert(std::dynamic_pointer_cast<IGroupSelectable>(baseNode));

    auto& baseManager = result.getBaseRootNode()->getSelectionGroupManager();
    auto& sourceManager = result.getSourceRootNode()->getSelectionGroupManager();

    auto baseSelectable = std::dynamic_pointer_cast<IGroupSelectable>(baseNode);
    auto sourceSelectable = std::dynamic_pointer_cast<IGroupSelectable>(sourceNode);

    if (!sourceSelectable || !baseSelectable)
    {
        return;
    }

    const auto& sourceGroupIds = sourceSelectable->getGroupIds();
    const auto& baseGroupIds = baseSelectable->getGroupIds();

    // Check the number of memberships, if that differs, we don't need to look further
    if (sourceGroupIds.size() != baseGroupIds.size())
    {
        rMessage() << "Source node " << sourceNode->name() << " has different number of groups than base node " << baseNode->name() << std::endl;

        result.selectionGroupDifferences.emplace_back(ComparisonResult::GroupDifference
        {
            sourceSelectable,
            baseSelectable,
            ComparisonResult::GroupDifference::Type::MembershipCountMismatch
        });

        return;
    }

    // Go through the group memberships of the source node and compare
    for (auto sourceGroupIter = sourceGroupIds.begin(), baseGroupIter = baseGroupIds.begin();
         sourceGroupIter != sourceGroupIds.end() && baseGroupIter != baseGroupIds.end();
         ++sourceGroupIter, ++baseGroupIter)
    {
        auto sourceGroup = sourceManager.getSelectionGroup(*sourceGroupIter);
        auto baseGroup = baseManager.getSelectionGroup(*baseGroupIter);

        if (!sourceGroup || !baseGroup) continue;

        auto sourceGroupFingerprint = calculateGroupFingerprint(sourceGroup);
        auto baseGroupFingerprint = calculateGroupFingerprint(baseGroup);

        if (sourceGroupFingerprint != baseGroupFingerprint)
        {
            rMessage() << "Source node " << sourceNode->name() << " groups are different than base node " << baseNode->name() << " groups" << std::endl;

            result.selectionGroupDifferences.emplace_back(ComparisonResult::GroupDifference
            {
                sourceSelectable,
                baseSelectable,
                ComparisonResult::GroupDifference::Type::GroupMemberMismatch
            });
            break;
        }
    }
}

std::string GraphComparer::calculateGroupFingerprint(const selection::ISelectionGroupPtr& group)
{
    std::vector<std::string> memberFingerprints(group->size());
    group->foreachNode([&](const INodePtr& member)
    {
        memberFingerprints.emplace_back(NodeUtils::GetGroupMemberFingerprint(member));
    });

    // Sort the fingerprints to be insensitive against actual member node ordering
    std::sort(memberFingerprints.begin(), memberFingerprints.end());

    // Combine all member hashes and we're done
    math::Hash hash;

    for (const auto& fingerprint : memberFingerprints)
    {
        hash.addString(fingerprint);
    }

    return hash;
}

}

}
