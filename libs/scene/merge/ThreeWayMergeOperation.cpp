#include "ThreeWayMergeOperation.h"

#include "itextstream.h"

namespace scene
{

namespace merge
{

ThreeWayMergeOperation::ThreeWayMergeOperation(const scene::IMapRootNodePtr& baseRoot,
    const scene::IMapRootNodePtr& sourceRoot, const scene::IMapRootNodePtr& targetRoot) :
    _baseRoot(baseRoot),
    _sourceRoot(sourceRoot),
    _targetRoot(targetRoot)
{}

bool ThreeWayMergeOperation::KeyValueDiffHasConflicts(const ComparisonResult::KeyValueDifference& sourceKeyValueDiff, 
    const std::list<ComparisonResult::KeyValueDifference>& targetKeyValueDiffs)
{
    for (const auto& targetKeyValueDiff : targetKeyValueDiffs)
    {
        // Skip non-matching keys
        if (targetKeyValueDiff.key != sourceKeyValueDiff.key) continue;

        // Key is matching, there's still a chance that this is not a conflict
        switch (targetKeyValueDiff.type)
        {
        // If both are removing the key, that's fine
        case ComparisonResult::KeyValueDifference::Type::KeyValueRemoved:
            return targetKeyValueDiff.type != sourceKeyValueDiff.type;

        // On key value change or add, the value must be the same to not conflict
        case ComparisonResult::KeyValueDifference::Type::KeyValueAdded:
        case ComparisonResult::KeyValueDifference::Type::KeyValueChanged:
            return sourceKeyValueDiff.type != ComparisonResult::KeyValueDifference::Type::KeyValueRemoved &&
                sourceKeyValueDiff.value == targetKeyValueDiff.value;
        }

        return true;
    }

    return false; // no conflicts detected
}

void ThreeWayMergeOperation::processEntityModification(const ComparisonResult::EntityDifference& sourceDiff, 
    const ComparisonResult::EntityDifference& targetDiff)
{
    assert(sourceDiff.type == ComparisonResult::EntityDifference::Type::EntityPresentButDifferent);

    if (targetDiff.type == ComparisonResult::EntityDifference::Type::EntityMissingInBase)
    {
        // The target cannot possibly add this entity when in the source diff it's present in the base
        throw std::logic_error("Entity " + sourceDiff.entityName + " is marked as modified in source, but as added in the target.");
    }

    if (targetDiff.type == ComparisonResult::EntityDifference::Type::EntityMissingInSource)
    {
        // This is a conflicting change, the source modified it, the target removed it
        // TODO: Add a conflict node
        return;
    }

    // Both graphs modified this entity, do an in-depth comparison

    // Every primitive change that has been done to the target map can be applied
    // to the source map, since we can't detect whether one of them has been moved or retextured
    for (const auto& primitiveDiff : sourceDiff.differingChildren)
    {
        // TODO: Don't create duplicates though
        createActionsForPrimitiveDiff(primitiveDiff, targetDiff.sourceNode);
    }

    // The key value changes can be applied only if they are not targeting the same key
    // unless the change has actually the same outcome
    for (const auto& sourceKeyValueDiff : sourceDiff.differingKeyValues)
    {
        if (!KeyValueDiffHasConflicts(sourceKeyValueDiff, targetDiff.differingKeyValues))
        {
            // Accept this change
            createActionsForKeyValueDiff(sourceKeyValueDiff, targetDiff.sourceNode);
        }
        else
        {
            // Create a conflict node for this key value change
            // TODO
        }
    }
}

void ThreeWayMergeOperation::processEntityDifferences(const std::list<ComparisonResult::EntityDifference>& sourceDiffs,
    const std::list<ComparisonResult::EntityDifference>& targetDiffs)
{
    // Create source and target entity diff dictionaries (by entity name)
    for (auto it = sourceDiffs.begin(); it != sourceDiffs.end(); ++it)
    {
        _sourceDifferences[it->entityName] = it;
    }

    for (auto it = targetDiffs.begin(); it != targetDiffs.end(); ++it)
    {
        _targetDifferences[it->entityName] = it;
    }

    // Check each entity difference from the base to the source map
    // fully accept only those entities that are not altered in the target map, and detect conflicts
    for (const auto& pair : _sourceDifferences)
    {
        auto targetDiff = _targetDifferences.find(pair.first);

        if (targetDiff == _targetDifferences.end())
        {
            // Change is targeting an entity that has not been altered in the source map => accept
            createActionsForEntity(*pair.second, _targetRoot);
            continue;
        }

        // Check diff type (entity add/remove)
        switch (pair.second->type)
        {
        case ComparisonResult::EntityDifference::Type::EntityMissingInBase: // entity was added to source
            
            if (targetDiff->second->type == ComparisonResult::EntityDifference::Type::EntityMissingInSource ||
                targetDiff->second->type == ComparisonResult::EntityDifference::Type::EntityPresentButDifferent)
            {
                // The target cannot remove or modify an entity that has been marked as added in the source diff
                throw std::logic_error("Error " + pair.first + " was marked as added in source, but removed/modified in target");
            }

            // Both graphs had this entity added, mark this for inclusion (with namespace)
            // TODO
            break;

        case ComparisonResult::EntityDifference::Type::EntityMissingInSource: // entity was removed in source
            
            if (targetDiff->second->type == ComparisonResult::EntityDifference::Type::EntityMissingInBase)
            {
                // The target cannot add an entity that has been marked as removed in the source diff, it was already there
                throw std::logic_error("Error " + pair.first + " was marked as removed in source, but as added in target");
            }

            if (targetDiff->second->type == ComparisonResult::EntityDifference::Type::EntityMissingInSource)
            {
                // Entity is gone in the target too, nothing to do here
                continue;
            }

            // Entity has been removed in source, but modified in target, this is a conflict
            // TODO: Add a conflict node

            break;
        
        case ComparisonResult::EntityDifference::Type::EntityPresentButDifferent:
            // This entity has been modified in the source, check the target diff
            processEntityModification(*pair.second, *targetDiff->second);
            break;
        }
    }
}

ThreeWayMergeOperation::Ptr ThreeWayMergeOperation::CreateFromComparisonResults(
    const ComparisonResult& baseToSource, const ComparisonResult& baseToTarget)
{
    if (baseToSource.getBaseRootNode() != baseToTarget.getBaseRootNode())
    {
        throw std::runtime_error("The base scene of the two comparison results must be the same");
    }

    auto operation = std::make_shared<ThreeWayMergeOperation>(baseToSource.getBaseRootNode(), 
        baseToSource.getSourceRootNode(), baseToTarget.getSourceRootNode());

    operation->processEntityDifferences(baseToSource.differingEntities, baseToTarget.differingEntities);

    return operation;
}

void ThreeWayMergeOperation::setMergeSelectionGroups(bool enabled)
{
    // TODO
}

void ThreeWayMergeOperation::setMergeLayers(bool enabled)
{
    // TODO
}

}

}
