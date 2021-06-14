#include "MergeOperation.h"
#include "MergeAction.h"
#include "SelectionGroupMerger.h"
#include "LayerMerger.h"

namespace scene
{

namespace merge
{

MergeOperation::Ptr MergeOperation::CreateFromComparisonResult(const ComparisonResult& result)
{
    auto operation = std::make_shared<MergeOperation>(result.getSourceRootNode(), result.getBaseRootNode());

    for (const auto& difference : result.differingEntities)
    {
        operation->createActionsForEntity(difference, result.getBaseRootNode());
    }

    return operation;
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
