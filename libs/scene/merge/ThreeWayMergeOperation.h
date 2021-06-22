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

    // Volatile data only needed during analysis
    struct ComparisonData;

    bool _mergeSelectionGroups;
    bool _mergeLayers;

public:
    using Ptr = std::shared_ptr<ThreeWayMergeOperation>;

    ThreeWayMergeOperation(const IMapRootNodePtr& baseRoot,
        const IMapRootNodePtr& sourceRoot, const IMapRootNodePtr& targetRoot);

    virtual ~ThreeWayMergeOperation();

    // Creates the merge operation from the given root nodes. The base root is the common ancestor of source and target,
    // and none of the three must be equal to any of them.
    static Ptr Create(const IMapRootNodePtr& baseRoot, const IMapRootNodePtr& sourceRoot, const IMapRootNodePtr& targetRoot);

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

    void applyActions() override;

private:
    void adjustSourceEntitiesWithNameConflicts();
    
    void compareAndCreateActions();
    void processEntityModification(const ComparisonResult::EntityDifference& sourceDiff,
        const ComparisonResult::EntityDifference& targetDiff);

    static ConflictType GetKeyValueConflictType(const ComparisonResult::KeyValueDifference& sourceKeyValueDiff,
        const ComparisonResult::KeyValueDifference& targetKeyValueDiff);
    static std::list<ComparisonResult::KeyValueDifference>::const_iterator FindTargetDiffByKey(
        const std::list<ComparisonResult::KeyValueDifference>& targetKeyValueDiffs, const std::string& key);
};

}

}