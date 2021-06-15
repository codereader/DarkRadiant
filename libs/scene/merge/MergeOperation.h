#pragma once

#include <list>
#include <memory>
#include "ComparisonResult.h"
#include "MergeAction.h"
#include "MergeOperationBase.h"

namespace scene
{

namespace merge
{

// A MergeOperation groups one or more merge actions
// together in order to apply a set of changes from source => base
class MergeOperation :
    public MergeOperationBase
{
private:
    scene::IMapRootNodePtr _sourceRoot;
    scene::IMapRootNodePtr _baseRoot;

    bool _mergeSelectionGroups;
    bool _mergeLayers;

public:
    using Ptr = std::shared_ptr<MergeOperation>;

    MergeOperation(scene::IMapRootNodePtr sourceRoot, scene::IMapRootNodePtr baseRoot) :
        _sourceRoot(sourceRoot),
        _baseRoot(baseRoot),
        _mergeSelectionGroups(true),
        _mergeLayers(true)
    {}

    virtual ~MergeOperation();

    // Creates the merge operation from the given comparison result. 
    // The operation will (on application) change the base map such that it matches the source map.
    static Ptr CreateFromComparisonResult(const ComparisonResult& comparisonResult);

    // Executes all active actions defined in this operation
    void applyActions() override;

    void setMergeSelectionGroups(bool enabled) override;
    void setMergeLayers(bool enabled) override;

private:
    void createActionsForEntity(const ComparisonResult::EntityDifference& difference, const IMapRootNodePtr& targetRoot);
};

}

}
