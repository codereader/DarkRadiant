#include "MergeOperation.h"

#include "ientity.h"
#include "../Clone.h"
#include "../scenelib.h"

namespace scene
{

namespace merge
{

class RemoveNodeFromParentAction :
    public MergeAction
{
private:
    scene::INodePtr _child;

protected:
    RemoveNodeFromParentAction(const scene::INodePtr& child, ActionType type) :
        MergeAction(type),
        _child(child)
    {
        assert(_child);
    }

public:
    void applyChanges() override
    {
        removeNodeFromParent(_child);
    }
};

class RemoveChildAction :
    public RemoveNodeFromParentAction
{
public:
    RemoveChildAction(const scene::INodePtr& node) :
        RemoveNodeFromParentAction(node, ActionType::RemoveChildNode)
    {}
};

class RemoveEntityAction :
    public RemoveNodeFromParentAction
{
public:
    RemoveEntityAction(const scene::INodePtr& node) :
        RemoveNodeFromParentAction(node, ActionType::RemoveEntity)
    {}
};

class AddCloneToParentAction :
    public MergeAction
{
private:
    scene::INodePtr _node;
    scene::INodePtr _parent;

protected:
    // Will add the given node to the parent when applyChanges() is called
    AddCloneToParentAction(const scene::INodePtr& node, const scene::INodePtr& parent, ActionType type) :
        MergeAction(type),
        _node(node),
        _parent(parent)
    {
        assert(_node);
        assert(Node_getCloneable(node));
    }

public:
    void applyChanges() override
    {
        // No post-clone callback since we don't care about selection groups right now
        auto cloned = cloneNodeIncludingDescendants(_node, PostCloneCallback());

        if (!cloned)
        {
            throw std::runtime_error("Node " + _node->name() + " is not cloneable");
        }

        addNodeToContainer(cloned, _parent);
    }
};

class AddEntityAction :
    public AddCloneToParentAction
{
public:
    AddEntityAction(const scene::INodePtr& node, const scene::IMapRootNodePtr& targetRoot) :
        AddCloneToParentAction(node, targetRoot, ActionType::AddEntity)
    {}
};

class AddChildAction :
    public AddCloneToParentAction
{
public:
    AddChildAction(const scene::INodePtr& node, const scene::INodePtr& parent) :
        AddCloneToParentAction(node, parent, ActionType::AddChildNode)
    {}
};

class SetEntityKeyValueAction :
    public MergeAction
{
private:
    scene::INodePtr _node;
    std::string _key;
    std::string _value;

public:
    // Will call setKeyValue(key, value) on the targetnode when applyChanges() is called
    SetEntityKeyValueAction(const scene::INodePtr& node, const std::string& key, const std::string& value, ActionType mergeActionType) :
        MergeAction(mergeActionType),
        _node(node),
        _key(key),
        _value(value)
    {
        assert(_node);
        assert(!_key.empty());
    }

    void applyChanges() override
    {
        // No post-clone callback since we don't care about selection groups right now
        auto entity = Node_getEntity(_node);

        if (!entity)
        {
            throw std::runtime_error("Node " + _node->name() + " is not an entity");
        }

        entity->setKeyValue(_key, _value);
    }
};

class AddEntityKeyValueAction :
    public SetEntityKeyValueAction
{
public:
    AddEntityKeyValueAction(const scene::INodePtr& node, const std::string& key, const std::string& value) :
        SetEntityKeyValueAction(node, key, value, ActionType::AddKeyValue)
    {}
};

class RemoveEntityKeyValueAction :
    public SetEntityKeyValueAction
{
public:
    RemoveEntityKeyValueAction(const scene::INodePtr& node, const std::string& key) :
        SetEntityKeyValueAction(node, key, std::string(), ActionType::RemoveKeyValue)
    {}
};

class ChangeEntityKeyValueAction :
    public SetEntityKeyValueAction
{
public:
    ChangeEntityKeyValueAction(const scene::INodePtr& node, const std::string& key, const std::string& value) :
        SetEntityKeyValueAction(node, key, value, ActionType::ChangeKeyValue)
    {}
};

void MergeOperation::addAction(const MergeAction::Ptr& action)
{
    _actions.push_back(action);
}

void MergeOperation::createActionsForKeyValueDiff(const ComparisonResult::KeyValueDifference& difference, 
    const scene::INodePtr& targetEntity)
{
    switch (difference.type)
    {
    case ComparisonResult::KeyValueDifference::Type::KeyValueAdded:
        addAction(std::make_shared<AddEntityKeyValueAction>(targetEntity, difference.key, difference.value));
        break;

    case ComparisonResult::KeyValueDifference::Type::KeyValueRemoved:
        addAction(std::make_shared<RemoveEntityKeyValueAction>(targetEntity, difference.key));
        break;

    case ComparisonResult::KeyValueDifference::Type::KeyValueChanged:
        addAction(std::make_shared<ChangeEntityKeyValueAction>(targetEntity, difference.key, difference.value));
        break;
    }
}

void MergeOperation::createActionsForPrimitiveDiff(const ComparisonResult::PrimitiveDifference& difference,
    const scene::INodePtr& targetEntity)
{
    switch (difference.type)
    {
    case ComparisonResult::PrimitiveDifference::Type::PrimitiveAdded:
        addAction(std::make_shared<AddChildAction>(difference.node, targetEntity));
        break;

    case ComparisonResult::PrimitiveDifference::Type::PrimitiveRemoved:
        addAction(std::make_shared<RemoveChildAction>(difference.node));
        break;
    }
}

void MergeOperation::createActionsForEntity(const ComparisonResult::EntityDifference& difference)
{
    switch (difference.type)
    {
    case ComparisonResult::EntityDifference::Type::EntityMissingInSource:
        addAction(std::make_shared<RemoveEntityAction>(difference.baseNode));
        break;

    case ComparisonResult::EntityDifference::Type::EntityMissingInBase:
        addAction(std::make_shared<AddEntityAction>(difference.sourceNode, _baseRoot));
        break;

    case ComparisonResult::EntityDifference::Type::EntityPresentButDifferent:
    {
        for (const auto& keyValueDiff : difference.differingKeyValues)
        {
            createActionsForKeyValueDiff(keyValueDiff, difference.baseNode);
        }

        for (const auto& primitiveDiff : difference.differingChildren)
        {
            createActionsForPrimitiveDiff(primitiveDiff, difference.baseNode);
        }
        break;
    }
    };
}

MergeOperation::Ptr MergeOperation::CreateFromComparisonResult(const ComparisonResult& result)
{
    auto operation = std::make_shared<MergeOperation>(result.getSourceRootNode(), result.getBaseRootNode());

    for (const auto& difference : result.differingEntities)
    {
        operation->createActionsForEntity(difference);
    }

    return operation;
}

}

}
