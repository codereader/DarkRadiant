#include "MergeOperation.h"

#include "../Clone.h"
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
    {
        assert(_node);
    }

    void applyChanges() override
    {
        removeNodeFromParent(_node);
    }
};

class AddEntityAction :
    public MergeAction
{
private:
    scene::INodePtr _node;
    scene::IMapRootNodePtr _targetRoot;

public:
    // Will add the given node to the targetRoot when applyChanges() is called
    AddEntityAction(const scene::INodePtr& node, const scene::IMapRootNodePtr& targetRoot) :
        MergeAction(ActionType::AddEntity),
        _node(node),
        _targetRoot(targetRoot)
    {
        assert(_node);
        assert(Node_getCloneable(node));
    }

    void applyChanges() override
    {
        // No post-clone callback since we don't care about selection groups right now
        auto cloned = cloneNodeIncludingDescendants(_node, PostCloneCallback());

        if (!cloned)
        {
            throw std::runtime_error("Node " + _node->name() + " is not cloneable");
        }

        addNodeToContainer(cloned, _targetRoot);
    }
};

void MergeOperation::addAction(const MergeAction::Ptr& action)
{
    _actions.push_back(action);
}

MergeOperation::Ptr MergeOperation::CreateFromComparisonResult(const ComparisonResult& comparisonResult)
{
    auto operation = std::make_shared<MergeOperation>();

    for (const auto& difference : comparisonResult.differingEntities)
    {
        switch (difference.type)
        {
        // Entities missing in the changed map will be removed
        case ComparisonResult::EntityDifference::Type::EntityMissingInSource:
            operation->addAction(std::make_shared<RemoveEntityAction>(difference.node));
            break;

        // Entities missing in the base map will be added
        case ComparisonResult::EntityDifference::Type::EntityMissingInBase:
            operation->addAction(std::make_shared<AddEntityAction>(difference.node, comparisonResult.getBaseRootNode()));
            break;
        };
    }

    return operation;
}

}

}
