#pragma once

#include <memory>
#include "ientity.h"
#include "../Clone.h"
#include "../scenelib.h"

namespace scene
{ 

namespace merge
{

enum class ActionType
{
    AddEntity,
    RemoveEntity,
    AddKeyValue,
    RemoveKeyValue,
    ChangeKeyValue,
    AddChildNode,
    RemoveChildNode,
};

// Represents a single step of a merge process, like adding a brush,
// removing an entity, setting a keyvalue, etc.
class MergeAction
{
private:
    ActionType _type;

protected:
    MergeAction(ActionType type) :
        _type(type)
    {}

public:
    using Ptr = std::shared_ptr<MergeAction>;

    ActionType getType() const
    {
        return _type;
    }

    // Applies all changes defined by this action.
    // It's the caller's responsibility to set up any Undo operations.
    // Implementations are allowed to throw std::runtime_errors on failure.
    virtual void applyChanges() = 0;
};

// Various implementations of the above MergeAction base type following

class RemoveNodeFromParentAction :
    public MergeAction
{
private:
    scene::INodePtr _nodeToRemove;

protected:
    RemoveNodeFromParentAction(const scene::INodePtr& nodeToRemove, ActionType type) :
        MergeAction(type),
        _nodeToRemove(nodeToRemove)
    {
        assert(_nodeToRemove);
    }

public:
    const scene::INodePtr& getNodeToRemove() const
    {
        return _nodeToRemove;
    }

    void applyChanges() override
    {
        removeNodeFromParent(_nodeToRemove);
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

    const scene::INodePtr& getParent() const
    {
        return _parent;
    }

    const scene::INodePtr& getSourceNodeToAdd() const
    {
        return _node;
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
        assert(Node_isEntity(_node));
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

    const scene::INodePtr& getEntityNode() const
    {
        return _node;
    }

    const std::string& getKey() const
    {
        return _key;
    }

    const std::string& getValue() const
    {
        return _value;
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

}

}
