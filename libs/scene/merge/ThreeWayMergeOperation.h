#pragma once

#include <map>
#include <list>
#include "imapmerge.h"
#include "ComparisonResult.h"
#include "MergeAction.h"
#include "MergeOperationBase.h"

namespace scene
{

namespace merge
{

/**
 * A Three-Way Merge Operation uses a common ancestor (base)
 * to define the actions needed to integrate the missing changes 
 * from the source map into the target map.
 */
class ThreeWayMergeOperation :
    public MergeOperationBase
{
private:
    IMapRootNodePtr _baseRoot;   // a common ancestor of the two maps
    IMapRootNodePtr _sourceRoot; // the map to be merged
    IMapRootNodePtr _targetRoot; // the map where elements are going to be merged into

private:
    // Volatile data only needed to construct the actions
    std::map<std::string, std::list<ComparisonResult::EntityDifference>::const_iterator> _sourceDifferences;
    std::map<std::string, std::list<ComparisonResult::EntityDifference>::const_iterator> _targetDifferences;

public:
    using Ptr = std::shared_ptr<ThreeWayMergeOperation>;

    ThreeWayMergeOperation(const IMapRootNodePtr& baseRoot,
        const IMapRootNodePtr& sourceRoot, const IMapRootNodePtr& targetRoot);

    virtual ~ThreeWayMergeOperation();

    // Creates the merge operation from the given comparison results. 
    // The operation will apply the missing changes in the source map to the target map 
    static Ptr CreateFromComparisonResults(const ComparisonResult& baseToSource, const ComparisonResult& baseToTarget);

    void setMergeSelectionGroups(bool enabled) override;
    void setMergeLayers(bool enabled) override;

    const IMapRootNodePtr& getSourceRoot() const
    {
        return _sourceRoot;
    }

    const IMapRootNodePtr& getTargetRoot() const
    {
        return _targetRoot;
    }

private:
    void processEntityDifferences(const std::list<ComparisonResult::EntityDifference>& sourceDiffs, 
        const std::list<ComparisonResult::EntityDifference>& targetDiffs);
    void processEntityModification(const ComparisonResult::EntityDifference& sourceDiff,
        const ComparisonResult::EntityDifference& targetDiff);

    static bool KeyValueDiffHasConflicts(const ComparisonResult::KeyValueDifference& sourceKeyValueDiff,
        const ComparisonResult::KeyValueDifference& targetKeyValueDiff);
    static std::list<ComparisonResult::KeyValueDifference>::const_iterator FindTargetDiffByKey(
        const std::list<ComparisonResult::KeyValueDifference>& targetKeyValueDiffs, const std::string& key);
};

}

}