#include "MergeOperation.h"

#include "../scenelib.h"

namespace scene
{

namespace merge
{

class RemoveEntityAction :
    public MergeAction
{
private:
    scene::INodePtr _node;

public:
    RemoveEntityAction(const scene::INodePtr& node) :
        MergeAction(ActionType::RemoveEntity),
        _node(node)
    {}

    void applyChanges() override
    {
        removeNodeFromParent(_node);
    }
};

void MergeOperation::addAction(const MergeAction::Ptr& action)
{
    _actions.push_back(action);
}

MergeOperation::Ptr MergeOperation::CreateFromComparisonResult(const ComparisonResult& comparisonResult)
{
    auto operation = std::make_shared<MergeOperation>();

    for (const auto& entityDifference : comparisonResult.differingEntities)
    {
        if (entityDifference.type == ComparisonResult::EntityDifference::Type::EntityMissingInSource)
        {
            operation->addAction(std::make_shared<RemoveEntityAction>(entityDifference.node));
        }
    }

    return operation;
}

}

}
