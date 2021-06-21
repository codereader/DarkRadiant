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

bool MergeOperationBase::hasActions()
{
    return !_actions.empty();
}

void MergeOperationBase::foreachAction(const std::function<void(const IMergeAction::Ptr&)>& visitor)
{
    for (const auto& action : _actions)
    {
        visitor(action);
    }
}

void MergeOperationBase::clearActions()
{
    _actions.clear();
}

void MergeOperationBase::addActionForKeyValueDiff(const ComparisonResult::KeyValueDifference& difference, 
    const scene::INodePtr& targetEntity)
{
    addAction(createActionForKeyValueDiff(difference, targetEntity));
}

MergeAction::Ptr MergeOperationBase::createActionForKeyValueDiff(const ComparisonResult::KeyValueDifference& difference,
    const scene::INodePtr& targetEntity)
{
    switch (difference.type)
    {
    case ComparisonResult::KeyValueDifference::Type::KeyValueAdded:
        return std::make_shared<AddEntityKeyValueAction>(targetEntity, difference.key, difference.value);

    case ComparisonResult::KeyValueDifference::Type::KeyValueRemoved:
        return std::make_shared<RemoveEntityKeyValueAction>(targetEntity, difference.key);

    case ComparisonResult::KeyValueDifference::Type::KeyValueChanged:
        return std::make_shared<ChangeEntityKeyValueAction>(targetEntity, difference.key, difference.value);
    }

    throw std::logic_error("Unhandled key value diff type in MergeOperationBase::createActionForKeyValueDiff");
}

void MergeOperationBase::addActionsForPrimitiveDiff(const ComparisonResult::PrimitiveDifference& difference,
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

}

}
