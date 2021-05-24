#include "MergeOperation.h"

#include "ientity.h"
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
    const scene::INodePtr& targetNode)
{
    switch (difference.type)
    {
    case ComparisonResult::KeyValueDifference::Type::KeyValueAdded:
        addAction(std::make_shared<AddEntityKeyValueAction>(targetNode, difference.key, difference.value));
        break;

    case ComparisonResult::KeyValueDifference::Type::KeyValueRemoved:
        addAction(std::make_shared<RemoveEntityKeyValueAction>(targetNode, difference.key));
        break;

    case ComparisonResult::KeyValueDifference::Type::KeyValueChanged:
        addAction(std::make_shared<ChangeEntityKeyValueAction>(targetNode, difference.key, difference.value));
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
        addAction(std::make_shared<AddEntityAction>(difference.sourceNode, _targetRoot));
        break;

    case ComparisonResult::EntityDifference::Type::EntityPresentButDifferent:
    {
        for (const auto& keyValueDiff : difference.differingKeyValues)
        {
            createActionsForKeyValueDiff(keyValueDiff, difference.baseNode);
        }
        break;
    }
    };
}

MergeOperation::Ptr MergeOperation::CreateFromComparisonResult(const ComparisonResult& comparisonResult)
{
    auto operation = std::make_shared<MergeOperation>(comparisonResult.getBaseRootNode());

    for (const auto& difference : comparisonResult.differingEntities)
    {
        operation->createActionsForEntity(difference);
    }

    return operation;
}

}

}
