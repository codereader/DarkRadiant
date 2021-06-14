#pragma once

#include <list>
#include "imapmerge.h"
#include "ComparisonResult.h"
#include "MergeAction.h"

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
    public IMergeOperation
{
private:
    scene::IMapRootNodePtr _sourceRoot; // the map to be merged
    scene::IMapRootNodePtr _targetRoot; // the map where elements are going to be merged into
    scene::IMapRootNodePtr _baseRoot;   // a common ancestor of the two above maps

    std::list<MergeAction::Ptr> _actions;

public:
    using Ptr = std::shared_ptr<ThreeWayMergeOperation>;

    // Creates the merge operation from the given comparison results. 
    // The operation will apply the missing changes in the source map to the target map 
    static Ptr CreateFromComparisonResults(const ComparisonResult& baseToSource, const ComparisonResult& baseToTarget);

    // Executes all active actions defined in this operation
    void applyActions() override;

    void foreachAction(const std::function<void(const IMergeAction::Ptr&)>& visitor) override;

    void setMergeSelectionGroups(bool enabled) override;
    void setMergeLayers(bool enabled) override;
};

}

}