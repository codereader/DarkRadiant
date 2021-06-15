#pragma once

#include <list>
#include "imap.h"
#include "imapmerge.h"
#include "MergeAction.h"
#include "ComparisonResult.h"

namespace scene
{

namespace merge
{

class MergeOperationBase :
    public IMergeOperation
{
private:
    std::list<MergeAction::Ptr> _actions;

public:
    // Executes all active actions defined in this operation
    virtual void applyActions() override;
    virtual void foreachAction(const std::function<void(const IMergeAction::Ptr&)>& visitor) override;

protected:
    virtual void addAction(const MergeAction::Ptr& action);
    void clearActions();
    
    void addActionForKeyValueDiff(const ComparisonResult::KeyValueDifference& difference,
        const scene::INodePtr& targetEntity);
    MergeAction::Ptr createActionForKeyValueDiff(const ComparisonResult::KeyValueDifference& difference,
        const scene::INodePtr& targetEntity);

    void addActionsForPrimitiveDiff(const ComparisonResult::PrimitiveDifference& difference,
        const scene::INodePtr& targetEntity);

    void createActionsForEntity(const ComparisonResult::EntityDifference& difference, const IMapRootNodePtr& targetRoot);
};

}

}