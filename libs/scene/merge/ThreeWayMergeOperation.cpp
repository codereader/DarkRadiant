#include "ThreeWayMergeOperation.h"

#include "itextstream.h"
#include "inamespace.h"
#include "NodeUtils.h"
#include "GraphComparer.h"
#include "ThreeWaySelectionGroupMerger.h"
#include "ThreeWayLayerMerger.h"

namespace scene
{

namespace merge
{

// Contains lookup tables needed during analysis of the two scenes
struct ThreeWayMergeOperation::ComparisonData
{
    std::map<std::string, std::list<ComparisonResult::EntityDifference>::const_iterator> sourceDifferences;
    std::map<std::string, std::list<ComparisonResult::EntityDifference>::const_iterator> targetDifferences;
    std::map<std::string, INodePtr> targetEntities;

    ComparisonResult::Ptr baseToSource;
    ComparisonResult::Ptr baseToTarget;

    ComparisonData(const IMapRootNodePtr& baseRoot, const IMapRootNodePtr& sourceRoot, const IMapRootNodePtr& targetRoot)
    {
        baseToSource = GraphComparer::Compare(sourceRoot, baseRoot);
        baseToTarget = GraphComparer::Compare(targetRoot, baseRoot);

        // Create source and target entity diff dictionaries (by entity name)
        for (auto it = baseToSource->differingEntities.begin(); it != baseToSource->differingEntities.end(); ++it)
        {
            sourceDifferences[it->entityName] = it;
        }

        for (auto it = baseToTarget->differingEntities.begin(); it != baseToTarget->differingEntities.end(); ++it)
        {
            targetDifferences[it->entityName] = it;
        }

        // Collect a map of all target entities for faster lookup later
        targetRoot->foreachNode([&](const INodePtr& candidate)
        {
            if (candidate->getNodeType() == INode::Type::Entity)
            {
                targetEntities.emplace(NodeUtils::GetEntityName(candidate), candidate);
            }

            return true;
        });
    }

    INodePtr findTargetEntityByName(const std::string& name) const
    {
        auto found = targetEntities.find(name);
        return found != targetEntities.end() ? found->second : INodePtr();
    }
};

ThreeWayMergeOperation::ThreeWayMergeOperation(const scene::IMapRootNodePtr& baseRoot,
    const scene::IMapRootNodePtr& sourceRoot, const scene::IMapRootNodePtr& targetRoot) :
    _baseRoot(baseRoot),
    _sourceRoot(sourceRoot),
    _targetRoot(targetRoot),
    _mergeSelectionGroups(true),
    _mergeLayers(true)
{}

ThreeWayMergeOperation::~ThreeWayMergeOperation()
{
    // Clear the actions held by the base class before the root nodes are cleared
    clearActions();
}

std::list<ComparisonResult::KeyValueDifference>::const_iterator ThreeWayMergeOperation::FindTargetDiffByKey(
    const std::list<ComparisonResult::KeyValueDifference>& targetKeyValueDiffs, const std::string& key)
{
    return std::find_if(targetKeyValueDiffs.begin(), targetKeyValueDiffs.end(),
        [&](const ComparisonResult::KeyValueDifference& diff)
    {
        return string::iequals(diff.key, key);
    });
}

ConflictType ThreeWayMergeOperation::GetKeyValueConflictType(const ComparisonResult::KeyValueDifference& sourceKeyValueDiff,
    const ComparisonResult::KeyValueDifference& targetKeyValueDiff)
{
    assert(string::iequals(targetKeyValueDiff.key, sourceKeyValueDiff.key));

    // Key is matching, there's still a chance that this is not a conflict
    switch (targetKeyValueDiff.type)
    {
    case ComparisonResult::KeyValueDifference::Type::KeyValueRemoved:
    {
        // Target had the key removed
        if (sourceKeyValueDiff.type == ComparisonResult::KeyValueDifference::Type::KeyValueAdded)
        {
            throw std::logic_error("Source cannot add a key that has been removed in target, as it was present in base.");
        }

        // If both are removing the key, that's fine, otherwise it's a conflict
        return sourceKeyValueDiff.type == ComparisonResult::KeyValueDifference::Type::KeyValueChanged ?
            ConflictType::ModificationOfRemovedKeyValue : ConflictType::NoConflict;
    }

    // On key value change or add, the value must be the same to not conflict
    case ComparisonResult::KeyValueDifference::Type::KeyValueAdded:
    {
        if (sourceKeyValueDiff.type != ComparisonResult::KeyValueDifference::Type::KeyValueAdded)
        {
            throw std::logic_error("Source cannot remove or modify a key that has been added in target, as it was present in base.");
        }

        // Value must match
        return sourceKeyValueDiff.value != targetKeyValueDiff.value ? ConflictType::SettingKeyToDifferentValue : ConflictType::NoConflict;
    }
    case ComparisonResult::KeyValueDifference::Type::KeyValueChanged:
    {
        if (sourceKeyValueDiff.type == ComparisonResult::KeyValueDifference::Type::KeyValueAdded)
        {
            throw std::logic_error("Source cannot add a key that has been modified in target, as it was present in base.");
        }

        if (sourceKeyValueDiff.type == ComparisonResult::KeyValueDifference::Type::KeyValueRemoved)
        {
            return ConflictType::RemovalOfModifiedKeyValue;
        }

        // Both maps changes this value, compare the strings
        return sourceKeyValueDiff.value != targetKeyValueDiff.value ? ConflictType::SettingKeyToDifferentValue : ConflictType::NoConflict;
    }
    }

    throw std::logic_error("Unhandled key value diff type in ThreeWayMergeOperation::KeyValueDiffHasConflicts");
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
        // When the user chooses to import the change, it will be an AddEntity action
        auto addEntityAction = std::make_shared<AddEntityAction>(sourceDiff.sourceNode, _targetRoot);

        addAction(std::make_shared<EntityConflictResolutionAction>(
            ConflictType::ModificationOfRemovedEntity,
            addEntityAction->getAffectedNode(), // the clone of the conflicting source entity
            INodePtr(), // the target entity (is no longer present)
            addEntityAction
        ));
        return;
    }

    // Both graphs modified this entity, do an in-depth comparison
    auto targetChildren = NodeUtils::CollectPrimitiveFingerprints(targetDiff.sourceNode);

    // Every primitive change that has been done to the target map can be applied
    // to the source map, since we can't detect whether one of them has been moved or retextured
    for (const auto& primitiveDiff : sourceDiff.differingChildren)
    {
        bool primitivePresentInTargetMap = targetChildren.count(primitiveDiff.fingerprint) != 0;

        switch (primitiveDiff.type)
        {
        case ComparisonResult::PrimitiveDifference::Type::PrimitiveAdded:
        {
            // Add this primitive if it isn't there already
            if (!primitivePresentInTargetMap)
            {
                addAction(std::make_shared<AddChildAction>(primitiveDiff.node, targetDiff.sourceNode));
            }
            break;
        }

        case ComparisonResult::PrimitiveDifference::Type::PrimitiveRemoved:
            // Check if this primitive is still present in the target map, otherwise we can't remove it
            if (primitivePresentInTargetMap)
            {
                addAction(std::make_shared<RemoveChildAction>(targetChildren[primitiveDiff.fingerprint]));
            }
            break;
        }
    }

    // The key value changes can be applied only if they are not targeting the same key
    // unless the change has actually the same outcome
    for (const auto& sourceKeyValueDiff : sourceDiff.differingKeyValues)
    {
        auto targetKeyValueDiff = FindTargetDiffByKey(targetDiff.differingKeyValues, sourceKeyValueDiff.key);

        if (targetKeyValueDiff == targetDiff.differingKeyValues.end())
        {
            // Not a key that changed in the target, accept this change
            addActionForKeyValueDiff(sourceKeyValueDiff, targetDiff.sourceNode);
            continue;
        }

        // Ignore NOP changes, when the target obviously made the same change
        if (sourceKeyValueDiff == *targetKeyValueDiff)
        {
            continue;
        }

        // Check if this key change is conflicting with the target change
        auto conflictType = GetKeyValueConflictType(sourceKeyValueDiff, *targetKeyValueDiff);

        if (conflictType == ConflictType::NoConflict)
        {
            // Accept this change
            addActionForKeyValueDiff(sourceKeyValueDiff, targetDiff.sourceNode);
        }
        else
        {
            // Create a conflict resolution action for this key value change
            addAction(std::make_shared<EntityKeyValueConflictResolutionAction>(
                conflictType,
                sourceDiff.sourceNode, // conflicting source entity
                targetDiff.sourceNode, // conflicting target entity
                createActionForKeyValueDiff(sourceKeyValueDiff, targetDiff.sourceNode), // conflicting source change 
                createActionForKeyValueDiff(*targetKeyValueDiff, targetDiff.sourceNode) // conflicting target change
            ));
        }
    }
}

void ThreeWayMergeOperation::compareAndCreateActions()
{
    ComparisonData data(_baseRoot, _sourceRoot, _targetRoot);

    // Check each entity difference from the base to the source map
    // fully accept only those entities that are not altered in the target map, and detect conflicts
    for (const auto& pair : data.sourceDifferences)
    {
        auto targetDiff = data.targetDifferences.find(pair.first);

        if (targetDiff == data.targetDifferences.end())
        {
            // Change is targeting an entity that has not been altered in the source map => accept
            switch (pair.second->type)
            {
            case ComparisonResult::EntityDifference::Type::EntityMissingInSource:
                {
                    auto entityToRemove = data.findTargetEntityByName(pair.first);
                    assert(entityToRemove);
                    addAction(std::make_shared<RemoveEntityAction>(entityToRemove));
                }
                break;

            case ComparisonResult::EntityDifference::Type::EntityMissingInBase:
                addAction(std::make_shared<AddEntityAction>(pair.second->sourceNode, _targetRoot));
                break;

            case ComparisonResult::EntityDifference::Type::EntityPresentButDifferent:
                {
                    auto entityToModify = data.findTargetEntityByName(pair.first);
                    assert(entityToModify);

                    for (const auto& keyValueDiff : pair.second->differingKeyValues)
                    {
                        addActionForKeyValueDiff(keyValueDiff, entityToModify);
                    }

                    // Collect fingerprints for all primitives of the target entity
                    auto targetChildren = NodeUtils::CollectPrimitiveFingerprints(entityToModify);

                    for (const auto& primitiveDiff : pair.second->differingChildren)
                    {
                        switch (primitiveDiff.type)
                        {
                        case ComparisonResult::PrimitiveDifference::Type::PrimitiveAdded:
                            addAction(std::make_shared<AddChildAction>(primitiveDiff.node, entityToModify));
                            break;

                        case ComparisonResult::PrimitiveDifference::Type::PrimitiveRemoved:
                            // The primitiveDiff at hand references the node in the source map,
                            // we need to find its counter-part in the target map, look it up by its fingerprint
                            auto targetPrimitive = targetChildren.find(primitiveDiff.fingerprint);

                            if (targetPrimitive != targetChildren.end())
                            {
                                // Located the matching primitive on the target entity, mark for removal
                                addAction(std::make_shared<RemoveChildAction>(targetPrimitive->second));
                            }
                            break;
                        }
                    }
                }
                break;
            };
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

            // Both graphs had this entity added, mark this for inclusion
            // unless it turns out this added entity in the source is 100% the same as in the target
            if (pair.second->sourceFingerprint != targetDiff->second->sourceFingerprint)
            {
                addAction(std::make_shared<AddEntityAction>(pair.second->sourceNode, _targetRoot));
            }
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
            addAction(std::make_shared<EntityConflictResolutionAction>(
                ConflictType::RemovalOfModifiedEntity,
                INodePtr(), // conflicting source entity (is not present anymore)
                targetDiff->second->sourceNode, // conflicting target entity
                std::make_shared<RemoveEntityAction>(targetDiff->second->sourceNode) // conflicting change 
            ));

            break;
        
        case ComparisonResult::EntityDifference::Type::EntityPresentButDifferent:
            // This entity has been modified in the source, check the target diff
            processEntityModification(*pair.second, *targetDiff->second);
            break;
        }
    }
}

ThreeWayMergeOperation::Ptr ThreeWayMergeOperation::Create(const IMapRootNodePtr& baseRoot, 
    const IMapRootNodePtr& sourceRoot, const IMapRootNodePtr& targetRoot)
{
    if (baseRoot == sourceRoot || baseRoot == targetRoot || sourceRoot == targetRoot)
    {
        throw std::runtime_error("None of the root nodes must be equal to any of the other two");
    }

    auto operation = std::make_shared<ThreeWayMergeOperation>(baseRoot, sourceRoot, targetRoot);

    // Phase 1 is to detect any entity additions from the source to the target that might cause name conflicts
    // After this pass some key values might have been changed
    operation->adjustSourceEntitiesWithNameConflicts();

    // Phase 2 will run another comparison of the graphs (since key values might have been modified)
    operation->compareAndCreateActions();

    return operation;
}

void ThreeWayMergeOperation::adjustSourceEntitiesWithNameConflicts()
{
    ComparisonData data(_baseRoot, _sourceRoot, _targetRoot);

    std::set<INodePtr> sourceEntitiesToBeRenamed;

    // Check each entity difference from the base to the source map
    for (const auto& pair : data.sourceDifferences)
    {
        if (pair.second->type != ComparisonResult::EntityDifference::Type::EntityMissingInBase)
        {
            continue; // we're only looking for additions
        }

        // Find an entity change in the target map that affects the same entity name
        // These are the source entities we have to rename first
        auto targetDiff = data.targetDifferences.find(pair.first);

        if (targetDiff == data.targetDifferences.end() || targetDiff->second->type != pair.second->type)
        {
            continue; // no target diff or not a target addition
        }

        // There's a chance this target entity is exactly the same as in the source
        if (targetDiff->second->sourceFingerprint == pair.second->sourceFingerprint)
        {
            continue; // The entity turns out to be the same
        }

        rMessage() << "Source entity needs to be renamed." << std::endl;
        sourceEntitiesToBeRenamed.insert(pair.second->sourceNode);
    }

    rMessage() << "Got " << sourceEntitiesToBeRenamed.size() << " entities to be renamed in "
        "the source before they can be imported in the target" << std::endl;

    // Let the target namespace detect any conflicts and assign new names to the source nodes
    // This might change a few key values across the source scene
    _targetRoot->getNamespace()->ensureNoConflicts(_sourceRoot, sourceEntitiesToBeRenamed);
}

std::string ThreeWayMergeOperation::getSourcePath()
{
    return _sourceRoot->getRootNode()->name();
}

std::string ThreeWayMergeOperation::getBasePath()
{
    return _baseRoot->getRootNode()->name();
}

void ThreeWayMergeOperation::applyActions()
{
    MergeOperationBase::applyActions();

    if (_mergeSelectionGroups)
    {
        ThreeWaySelectionGroupMerger merger(_baseRoot, _sourceRoot, _targetRoot);

        merger.adjustTargetGroups();
    }

    if (_mergeLayers)
    {
        ThreeWayLayerMerger merger(_baseRoot, _sourceRoot, _targetRoot);
        merger.adjustTargetLayers();
    }
}

void ThreeWayMergeOperation::setMergeSelectionGroups(bool enabled)
{
    _mergeSelectionGroups = enabled;
}

void ThreeWayMergeOperation::setMergeLayers(bool enabled)
{
    _mergeLayers = enabled;
}

}

}
