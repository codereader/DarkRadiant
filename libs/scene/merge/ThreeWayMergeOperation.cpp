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

void ThreeWayMergeOperation::processEntityDifference(const ComparisonResult::EntityDifference& diff)
{
    
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
    // accept only those that are not contained in the target map, and detect conflicts
    for (const auto& pair : _sourceDifferences)
    {
        auto targetDiff = _targetDifferences.find(pair.first);

        if (targetDiff == _targetDifferences.end())
        {
            // Change is targeting an entity that has not been altered in the source map => accept
            continue;
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
