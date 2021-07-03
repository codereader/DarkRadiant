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
    std::list<IMergeAction::Ptr> _actions;

    sigc::signal<void, const IMergeAction::Ptr&> _sigActionAdded;

public:
    // Executes all active actions defined in this operation
    virtual void applyActions() override;
    virtual bool hasActions() override;
    virtual void foreachAction(const std::function<void(const IMergeAction::Ptr&)>& visitor) override;
    virtual void addAction(const IMergeAction::Ptr& action) override;
    virtual sigc::signal<void, const IMergeAction::Ptr&>& sig_ActionAdded() override;

protected:
    void clearActions();
    
    void addActionForKeyValueDiff(const ComparisonResult::KeyValueDifference& difference,
        const scene::INodePtr& targetEntity);
    MergeAction::Ptr createActionForKeyValueDiff(const ComparisonResult::KeyValueDifference& difference,
        const scene::INodePtr& targetEntity);

    void addActionsForPrimitiveDiff(const ComparisonResult::PrimitiveDifference& difference,
        const scene::INodePtr& targetEntity);
};

}

}