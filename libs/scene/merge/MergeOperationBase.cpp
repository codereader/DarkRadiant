#include "MergeOperationBase.h"

#include "itextstream.h"

namespace scene
{

namespace merge
{

void MergeOperationBase::addAction(const MergeAction::Ptr& action)
{
    _actions.push_back(action);
}

void MergeOperationBase::applyActions()
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
}

void MergeOperationBase::foreachAction(const std::function<void(const IMergeAction::Ptr&)>& visitor)
{
    for (const auto& action : _actions)
    {
        visitor(action);
    }
}

void MergeOperationBase::createActionsForKeyValueDiff(const ComparisonResult::KeyValueDifference& difference, 
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

void MergeOperationBase::createActionsForPrimitiveDiff(const ComparisonResult::PrimitiveDifference& difference,
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

void MergeOperationBase::createActionsForEntity(const ComparisonResult::EntityDifference& difference, const IMapRootNodePtr& targetRoot)
{
    switch (difference.type)
    {
    case ComparisonResult::EntityDifference::Type::EntityMissingInSource:
        addAction(std::make_shared<RemoveEntityAction>(difference.baseNode));
        break;

    case ComparisonResult::EntityDifference::Type::EntityMissingInBase:
        addAction(std::make_shared<AddEntityAction>(difference.sourceNode, targetRoot));
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

}

}
