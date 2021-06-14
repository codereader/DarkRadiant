#include "ThreeWayMergeOperation.h"

#include "itextstream.h"

namespace scene
{

namespace merge
{

ThreeWayMergeOperation::Ptr ThreeWayMergeOperation::CreateFromComparisonResults(
    const ComparisonResult& baseToSource, const ComparisonResult& baseToTarget)
{
    if (baseToSource.getBaseRootNode() != baseToTarget.getBaseRootNode())
    {
        throw std::runtime_error("The base scene of the two comparison results must be the same");
    }

    // TODO
}

void ThreeWayMergeOperation::applyActions()
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

void ThreeWayMergeOperation::foreachAction(const std::function<void(const IMergeAction::Ptr&)>& visitor)
{
    for (const auto& action : _actions)
    {
        visitor(action);
    }
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
