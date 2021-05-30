#pragma once

#include <memory>
#include "inode.h"

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

/**
 * Represents a merge action, i.e. one single step of a merge operation.
 * Only active actions will be processed when the merge run starts.
 */ 
class IMergeAction
{
public:
    virtual ~IMergeAction() {}

    using Ptr = std::shared_ptr<IMergeAction>;

    // The type performed by this action
    virtual ActionType getType() const = 0;

    // Activate this action, it will be executed during the merge
    virtual void activate() = 0;

    // Deactivate this action, it will NOT be executed during the merge
    virtual void deactivate() = 0;

    // Returns the active state of this action
    virtual bool isActive() const = 0;

    // Applies all changes defined by this action (if it is active,
    // deactivated action will not take any effect).
    // It's the caller's responsibility to set up any Undo operations.
    // Implementations are allowed to throw std::runtime_errors on failure.
    virtual void applyChanges() = 0;

    // Returns the node this action is affecting when applied
    // This is used to identify the scene node and display it appropriately
    virtual scene::INodePtr getAffectedNode() = 0;
};

}

/**
 * Special scene node type representing a change conducted by a merge action, 
 * i.e. addition, removal or changing a node in the scene.
 */
class IMergeActionNode :
    public virtual scene::INode
{
public:
    virtual ~IMergeActionNode() {}

    // Return the action type represented by this node
    virtual merge::ActionType getActionType() const = 0;

    // Return the node this action is affecting
    virtual scene::INodePtr getAffectedNode() = 0;

    // Iterate over all actions of this node
    virtual void foreachMergeAction(const std::function<void(const merge::IMergeAction::Ptr&)>& functor) = 0;
};

}
