#include "MergeOperation.h"
#include "MergeAction.h"
#include "SelectionGroupMerger.h"
#include "LayerMerger.h"

namespace scene
{

namespace merge
{

MergeOperation::~MergeOperation()
{
    clearActions();
}

void MergeOperation::createActionsForEntity(const ComparisonResult::EntityDifference& difference, const IMapRootNodePtr& targetRoot)
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
            addActionForKeyValueDiff(keyValueDiff, difference.baseNode);
        }

        for (const auto& primitiveDiff : difference.differingChildren)
        {
            addActionsForPrimitiveDiff(primitiveDiff, difference.baseNode);
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
        operation->createActionsForEntity(difference, result.getBaseRootNode());
    }

    return operation;
}

std::string MergeOperation::getSourcePath()
{
    return _sourceRoot->getRootNode()->name();
}

std::string MergeOperation::getBasePath()
{
    return std::string(); // no base
}

void MergeOperation::applyActions()
{
    MergeOperationBase::applyActions();

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
