#pragma once

#include <memory>
#include "ientity.h"
#include "imapmerge.h"
#include "../Clone.h"
#include "../scenelib.h"

namespace scene
{ 

namespace merge
{

// Represents a single step of a merge process, like adding a brush,
// removing an entity, setting a keyvalue, etc.
class MergeAction : 
    public virtual IMergeAction
{
private:
    ActionType _type;

    bool _isActive;

protected:
    MergeAction(ActionType type) :
        _type(type),
        _isActive(true)
    {}

public:
    using Ptr = std::shared_ptr<MergeAction>;

    ActionType getType() const override
    {
        return _type;
    }

    virtual void activate() override
    {
        _isActive = true;
    }

    virtual void deactivate() override
    {
        _isActive = false;
    }

    virtual bool isActive() const override
    {
        return _isActive;
    }
};

// Various implementations of the above MergeAction base type following

class RemoveNodeFromParentAction :
    public MergeAction
{
private:
    INodePtr _nodeToRemove;

protected:
    RemoveNodeFromParentAction(const INodePtr& nodeToRemove, ActionType type) :
        MergeAction(type),
        _nodeToRemove(nodeToRemove)
    {
        assert(_nodeToRemove);
    }

public:
    const INodePtr& getNodeToRemove() const
    {
        return _nodeToRemove;
    }

    void applyChanges() override
    {
        if (!isActive()) return;

        removeNodeFromParent(_nodeToRemove);
    }

    INodePtr getAffectedNode() override
    {
        return getNodeToRemove();
    }
};

class RemoveChildAction :
    public RemoveNodeFromParentAction
{
public:
    RemoveChildAction(const INodePtr& node) :
        RemoveNodeFromParentAction(node, ActionType::RemoveChildNode)
    {}
};

class RemoveEntityAction :
    public RemoveNodeFromParentAction
{
public:
    RemoveEntityAction(const INodePtr& node) :
        RemoveNodeFromParentAction(node, ActionType::RemoveEntity)
    {}
};

class AddCloneToParentAction :
    public MergeAction
{
private:
    INodePtr _node;
    INodePtr _parent;
    INodePtr _cloneToBeInserted;

    // For func_* based entities, the model key needs to be the same as the name
    bool _modelIsEqualToName;

protected:
    // Will add the given node to the parent when applyChanges() is called
    AddCloneToParentAction(const INodePtr& node, const INodePtr& parent, ActionType type) :
        MergeAction(type),
        _node(node),
        _parent(parent),
        _modelIsEqualToName(false)
    {
        assert(_node);
        assert(Node_getCloneable(node));

        auto* entity = Node_getEntity(node);
        _modelIsEqualToName = entity && entity->getKeyValue("name") == entity->getKeyValue("model");

        // No post-clone callback since we don't care about selection groups right now
        _cloneToBeInserted = cloneNodeIncludingDescendants(_node, PostCloneCallback());

        if (!_cloneToBeInserted)
        {
            throw std::runtime_error("Node " + _node->name() + " is not cloneable");
        }

        // Reset all layers of the clone to the active one
        auto activeLayer = parent->getRootNode()->getLayerManager().getActiveLayer();

        _cloneToBeInserted->moveToLayer(activeLayer);
        _cloneToBeInserted->foreachNode([=](const INodePtr& child) 
        { 
            child->moveToLayer(activeLayer); return true; 
        });
    }

public:
    void addSourceNodeToScene()
    {
        // Get the clone and add it to the target scene, it needs to be renderable here
        addNodeToContainer(_cloneToBeInserted, _parent);

        // Check if we need to synchronise the model and name key values
        if (_modelIsEqualToName)
        {
            auto* entity = Node_getEntity(_cloneToBeInserted);

            if (entity)
            {
                auto curName = entity->getKeyValue("name");

                if (curName != entity->getKeyValue("model"))
                {
                    entity->setKeyValue("model", curName);
                }
            }
        }
    }

    void removeSourceNodeFromScene()
    {
        removeNodeFromParent(_cloneToBeInserted);
    }

    void applyChanges() override
    {
        if (!isActive()) return;

        addSourceNodeToScene();
    }

    const INodePtr& getParent() const
    {
        return _parent;
    }

    const INodePtr& getSourceNodeToAdd() const
    {
        return _node;
    }

    INodePtr getAffectedNode() override
    {
        return _cloneToBeInserted;
    }
};

class AddEntityAction :
    public AddCloneToParentAction
{
public:
    AddEntityAction(const INodePtr& node, const IMapRootNodePtr& targetRoot) :
        AddCloneToParentAction(node, targetRoot, ActionType::AddEntity)
    {}
};

class AddChildAction :
    public AddCloneToParentAction
{
public:
    AddChildAction(const INodePtr& node, const INodePtr& parent) :
        AddCloneToParentAction(node, parent, ActionType::AddChildNode)
    {}
};

class SetEntityKeyValueAction :
    public MergeAction,
    public virtual IEntityKeyValueMergeAction
{
private:
    INodePtr _node;
    std::string _key;
    std::string _value;

public:
    // Will call setKeyValue(key, value) on the targetnode when applyChanges() is called
    SetEntityKeyValueAction(const INodePtr& node, const std::string& key, const std::string& value, ActionType mergeActionType) :
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
        if (!isActive()) return;

        // No post-clone callback since we don't care about selection groups right now
        auto entity = Node_getEntity(_node);

        if (!entity)
        {
            throw std::runtime_error("Node " + _node->name() + " is not an entity");
        }

        entity->setKeyValue(_key, _value);
    }

    const INodePtr& getEntityNode() const
    {
        return _node;
    }

    const std::string& getKey() const override
    {
        return _key;
    }

    const std::string& getValue() const override
    {
        return _value;
    }

    INodePtr getAffectedNode() override
    {
        return getEntityNode();
    }
};

class AddEntityKeyValueAction :
    public SetEntityKeyValueAction
{
public:
    AddEntityKeyValueAction(const INodePtr& node, const std::string& key, const std::string& value) :
        SetEntityKeyValueAction(node, key, value, ActionType::AddKeyValue)
    {}
};

class RemoveEntityKeyValueAction :
    public SetEntityKeyValueAction
{
public:
    RemoveEntityKeyValueAction(const INodePtr& node, const std::string& key) :
        SetEntityKeyValueAction(node, key, std::string(), ActionType::RemoveKeyValue)
    {}
};

class ChangeEntityKeyValueAction :
    public SetEntityKeyValueAction
{
public:
    ChangeEntityKeyValueAction(const INodePtr& node, const std::string& key, const std::string& value) :
        SetEntityKeyValueAction(node, key, value, ActionType::ChangeKeyValue)
    {}
};

/**
 * A ConflictResolutionAction encapsulates a source change that has a
 * encountered a conflicting target change.
 * 
 * The source change will only be applied if this action has been
 * told to do so by calling setResolvedByUsingSource(true),
 * otherwise nothing happens and the target change stays in effect.
 */
class ConflictResolutionAction :
    public MergeAction,
    public virtual IConflictResolutionAction
{
protected:
    ConflictType _conflictType;
    INodePtr _conflictingSourceEntity;
    INodePtr _conflictingTargetEntity;

    // The action the source diff is trying to apply
    IMergeAction::Ptr _sourceAction;
    // The action that happened in the target
    IMergeAction::Ptr _targetAction;

    ResolutionType _resolution;

protected:
    ConflictResolutionAction(ConflictType conflictType, const INodePtr& conflictingSourceEntity,
                             const INodePtr& conflictingTargetEntity, const IMergeAction::Ptr& sourceAction) :
        ConflictResolutionAction(conflictType, conflictingSourceEntity, conflictingTargetEntity, sourceAction, IMergeAction::Ptr())
    {}

    ConflictResolutionAction(ConflictType conflictType, const INodePtr& conflictingSourceEntity,
                            const INodePtr& conflictingTargetEntity, const IMergeAction::Ptr& sourceAction, const IMergeAction::Ptr& targetAction) :
        MergeAction(ActionType::ConflictResolution),
        _conflictType(conflictType),
        _conflictingSourceEntity(conflictingSourceEntity),
        _conflictingTargetEntity(conflictingTargetEntity),
        _sourceAction(sourceAction),
        _targetAction(targetAction),
        _resolution(ResolutionType::Unresolved)
    {}

public:
    using Ptr = std::shared_ptr<ConflictResolutionAction>;

    ConflictType getConflictType() const override
    {
        return _conflictType;
    }

    // The action the source diff is trying to apply
    const IMergeAction::Ptr& getSourceAction() const override
    {
        return _sourceAction;
    }

    // The action that happened in the target (can be empty)
    const IMergeAction::Ptr& getTargetAction() const override
    {
        return _targetAction;
    }

    const INodePtr& getConflictingTargetEntity() const override
    {
        return _conflictingTargetEntity;
    }

    const INodePtr& getConflictingSourceEntity() const override
    {
        return _conflictingSourceEntity;
    }

    INodePtr getAffectedNode() override
    {
        // We don't want to return empty references, so use the source entity if the target entity is no longer here
        return _conflictingTargetEntity ? _conflictingTargetEntity : _conflictingSourceEntity;
    }

    ResolutionType getResolution() const override
    {
        return _resolution;
    }

    void setResolution(ResolutionType resolution) override
    {
        _resolution = resolution;
    }

    virtual bool isActive() const override
    {
        // The conflict resolution action is active when either not resolved yet
        // or the wrapped and chosen action is active.
        return _resolution == ResolutionType::Unresolved ||
            _resolution == ResolutionType::ApplySourceChange && _sourceAction->isActive();
    }

    void applyChanges() override
    {
        if (!isActive()) return;

        if (_resolution == ResolutionType::ApplySourceChange)
        {
            _sourceAction->applyChanges();
        }
    }
};

// An entity node is a conflicting subject in both maps
class EntityConflictResolutionAction :
    public ConflictResolutionAction
{
public:
    EntityConflictResolutionAction(ConflictType conflictType, const INodePtr& conflictingSourceEntity,
                                   const INodePtr& conflictingTargetEntity, const MergeAction::Ptr& sourceAction) :
        EntityConflictResolutionAction(conflictType, conflictingSourceEntity, conflictingTargetEntity, sourceAction, MergeAction::Ptr())
    {}

    EntityConflictResolutionAction(ConflictType conflictType, 
                                   const INodePtr& conflictingSourceEntity,
                                   const INodePtr& conflictingTargetEntity,
                                   const MergeAction::Ptr& sourceAction, 
                                   const MergeAction::Ptr& targetAction) :
        ConflictResolutionAction(conflictType, conflictingSourceEntity, conflictingTargetEntity, sourceAction, targetAction)
    {}
};

// An entity key value is a conflicting subject in both maps
class EntityKeyValueConflictResolutionAction :
    public ConflictResolutionAction
{
public:
    EntityKeyValueConflictResolutionAction(ConflictType conflictType,
                                           const INodePtr& conflictingSourceEntity,
                                           const INodePtr& conflictingTargetEntity,
                                           const MergeAction::Ptr& sourceAction, 
                                           const MergeAction::Ptr& targetAction) :
        ConflictResolutionAction(conflictType, conflictingSourceEntity, conflictingTargetEntity, sourceAction, targetAction)
    {}
};

}

}
