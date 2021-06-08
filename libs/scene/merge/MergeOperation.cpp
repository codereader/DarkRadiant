#include "MergeOperation.h"
#include "MergeAction.h"
#include "SelectionGroupMerger.h"
#include "LayerMerger.h"

namespace scene
{

namespace merge
{

void MergeOperation::addAction(const MergeAction::Ptr& action)
{
    _actions.push_back(action);
}

void MergeOperation::createActionsForKeyValueDiff(const ComparisonResult::KeyValueDifference& difference, 
    const scene::INodePtr& targetEntity)
{
    switch (difference.type)
    {
    case ComparisonResult::KeyValueDifference::Type::KeyValueAdded:
        addAction(std::make_shared<AddEntityKeyValueAction>(targetEntity, difference.key, difference.value));
        break;

    case ComparisonResult::KeyValueDifference::Type::KeyValueRemoved:
        addAction(std::make_shared<RemoveEntityKeyValueAction>(targetEntity, difference.key));
        break;

    case ComparisonResult::KeyValueDifference::Type::KeyValueChanged:
        addAction(std::make_shared<ChangeEntityKeyValueAction>(targetEntity, difference.key, difference.value));
        break;
    }
}

void MergeOperation::createActionsForPrimitiveDiff(const ComparisonResult::PrimitiveDifference& difference,
    const scene::INodePtr& targetEntity)
{
    switch (difference.type)
    {
    case ComparisonResult::PrimitiveDifference::Type::PrimitiveAdded:
        addAction(std::make_shared<AddChildAction>(difference.node, targetEntity));
        break;

    case ComparisonResult::PrimitiveDifference::Type::PrimitiveRemoved:
        addAction(std::make_shared<RemoveChildAction>(difference.node));
        break;
    }
}

void MergeOperation::createActionsForEntity(const ComparisonResult::EntityDifference& difference)
{
    switch (difference.type)
    {
    case ComparisonResult::EntityDifference::Type::EntityMissingInSource:
        addAction(std::make_shared<RemoveEntityAction>(difference.baseNode));
        break;

    case ComparisonResult::EntityDifference::Type::EntityMissingInBase:
        addAction(std::make_shared<AddEntityAction>(difference.sourceNode, _baseRoot));
        break;

    case ComparisonResult::EntityDifference::Type::EntityPresentButDifferent:
    {
        for (const auto& keyValueDiff : difference.differingKeyValues)
        {
            createActionsForKeyValueDiff(keyValueDiff, difference.baseNode);
        }

        for (const auto& primitiveDiff : difference.differingChildren)
        {
            createActionsForPrimitiveDiff(primitiveDiff, difference.baseNode);
        }
        break;
    }
    };
}

MergeOperation::Ptr MergeOperation::CreateFromComparisonResult(const ComparisonResult& result)
{
    auto operation = std::make_shared<MergeOperation>(result.getSourceRootNode(), result.getBaseRootNode());

    for (const auto& difference : result.differingEntities)
    {
        operation->createActionsForEntity(difference);
    }

    return operation;
}

void MergeOperation::applyActions()
{
    for (auto& action : _actions)
    {
        try
        {
            action->applyChanges();
        }
        catch (const std::runtime_error& ex)
        {
            rError() << "Failed to apply action: " << ex.what() << std::endl;
        }
    }

    if (_mergeSelectionGroups)
    {
        SelectionGroupMerger merger(_sourceRoot, _baseRoot);

        merger.adjustBaseGroups();
    }

    if (_mergeLayers)
    {
        LayerMerger merger(_sourceRoot, _baseRoot);

        merger.adjustBaseLayers();
    }
}

void MergeOperation::foreachAction(const std::function<void(const IMergeAction::Ptr&)>& visitor)
{
    for (const auto& action : _actions)
    {
        visitor(action);
    }
}

void MergeOperation::setMergeSelectionGroups(bool enabled)
{
    _mergeSelectionGroups = enabled;
}

void MergeOperation::setMergeLayers(bool enabled)
{
    _mergeLayers = enabled;
}

}

}
