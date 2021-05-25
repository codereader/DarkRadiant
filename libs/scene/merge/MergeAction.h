#pragma once

#include <memory>
#include "ientity.h"
#include "imergeaction.h"
#include "../Clone.h"
#include "../scenelib.h"

namespace scene
{ 

namespace merge
{

// Represents a single step of a merge process, like adding a brush,
// removing an entity, setting a keyvalue, etc.
class MergeAction : 
    public IMergeAction
{
private:
    ActionType _type;

protected:
    MergeAction(ActionType type) :
        _type(type)
    {}

public:
    using Ptr = std::shared_ptr<MergeAction>;

    ActionType getType() const override
    {
        return _type;
    }
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

    scene::INodePtr getAffectedNode() override
    {
        return getNodeToRemove();
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
    scene::INodePtr _cloneToBeInserted;

protected:
    // Will add the given node to the parent when applyChanges() is called
    AddCloneToParentAction(const scene::INodePtr& node, const scene::INodePtr& parent, ActionType type) :
        MergeAction(type),
        _node(node),
        _parent(parent)
    {
        assert(_node);
        assert(Node_getCloneable(node));

        // No post-clone callback since we don't care about selection groups right now
        _cloneToBeInserted = cloneNodeIncludingDescendants(_node, PostCloneCallback());

        if (!_cloneToBeInserted)
        {
            throw std::runtime_error("Node " + _node->name() + " is not cloneable");
        }

        // Reset all layers of the clone to the active one
        auto activeLayer = parent->getRootNode()->getLayerManager().getActiveLayer();

        _cloneToBeInserted->moveToLayer(activeLayer);
        _cloneToBeInserted->foreachNode([=](const scene::INodePtr& child) 
        { 
            child->moveToLayer(activeLayer); return true; 
        });
    }

public:
    void applyChanges() override
    {
        addNodeToContainer(_cloneToBeInserted, _parent);
    }

    const scene::INodePtr& getParent() const
    {
        return _parent;
    }

    const scene::INodePtr& getSourceNodeToAdd() const
    {
        return _node;
    }

    scene::INodePtr getAffectedNode() override
    {
        return _cloneToBeInserted;
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

    scene::INodePtr getAffectedNode() override
    {
        return getEntityNode();
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
