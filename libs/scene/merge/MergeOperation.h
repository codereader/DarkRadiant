#pragma once

#include <list>
#include <memory>
#include "ComparisonResult.h"
#include "MergeAction.h"

namespace scene
{

namespace merge
{

// A MergeOperation groups one or more merge actions
// together in order to apply a set of changes to an existing map
class MergeOperation
{
private:
    std::list<MergeAction::Ptr> _actions;

public:
    using Ptr = std::shared_ptr<MergeOperation>;

    // Creates the merge operation from the given comparison result. 
    // The operation will (on application) change the base map such that it matches the source map.
    static MergeOperation::Ptr CreateFromComparisonResult(const ComparisonResult& comparisonResult);

    void addAction(const MergeAction::Ptr& action);
};

}

}
