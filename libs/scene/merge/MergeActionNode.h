#pragma once

#include "imapmerge.h"
#include "iselectiontest.h"
#include "math/AABB.h"

#include "../SelectableNode.h"
#include "MergeAction.h"

namespace scene
{

/**
 * Common node type for visualising a merge action in the scene, targeting a single node.
 * Can be selected and tested for selection, but cannot be hidden.
 * The node affected by the operation is "taken over" and is hidden while this node is active.
 * Rendering of the node is explicitly performed by this merge node.
 */
class MergeActionNodeBase :
    public IMergeActionNode,
    public SelectableNode,
    public SelectionTestable
{
protected:
    INodePtr _affectedNode;
    bool _syncActionStatus;

    MergeActionNodeBase();

public:
    using Ptr = std::shared_ptr<MergeActionNodeBase>;

    // Prepare this node right before a merge, such that it
    // doesn't change the action's status when removed from the scene
    void prepareForMerge();

    INodePtr getAffectedNode() override;

    // Clears the references to actions and nodes, such that it doesn't hold 
    // any strong refs to Nodes, causing trouble when clearing the undo stack later
    virtual void clear();

    virtual void onInsertIntoScene(IMapRootNode& rootNode) override;
    virtual void onRemoveFromScene(IMapRootNode& rootNode) override;

    INode::Type getNodeType() const override;

    bool supportsStateFlag(unsigned int state) const override;

    const AABB& localAABB() const override;
    const Matrix4& localToWorld() const override;

    void onPreRender(const VolumeTest& volume) override;
    void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume) override;
    std::size_t getHighlightFlags() override;

    void testSelect(Selector& selector, SelectionTest& test) override;

private:
    void testSelectNode(const INodePtr& node, Selector& selector, SelectionTest& test);

    void hideAffectedNodes();
    void unhideAffectedNodes();
};

/**
 * Special merge node representing one or more key value changes of a single entity.
 * If not pointing to a conflict, this will report as "ChangeKeyValue" action type, 
 * even though the contained set of merge actions can comprise various types.
 */
class KeyValueMergeActionNode final :
    public MergeActionNodeBase
{
private:
    std::vector<merge::IMergeAction::Ptr> _actions;

public:
    KeyValueMergeActionNode(const std::vector<merge::IMergeAction::Ptr>& actions);

    void clear() override;

    merge::ActionType getActionType() const override;
    void foreachMergeAction(const std::function<void(const merge::IMergeAction::Ptr&)>& functor) override;
    std::size_t getMergeActionCount() override;
    bool hasActiveActions() override;
};

/**
 * For all merge action types other than key value changes this "regular"
 * merge action node is used to represent it in the scene.
 * Encapsulates a single merge action only.
 * 
 * For AddChildNode/AddEntityNode action types this node will take care
 * of inserting and removing the affected node into the target scene,
 * such that it can be previewed by the user.
 */
class RegularMergeActionNode final :
    public MergeActionNodeBase
{
private:
    merge::IMergeAction::Ptr _action;

public:
    RegularMergeActionNode(const merge::IMergeAction::Ptr& action);

    void onInsertIntoScene(IMapRootNode& rootNode) override;
    void onRemoveFromScene(IMapRootNode& rootNode) override;

    void clear() override;

    merge::ActionType getActionType() const override;
    void foreachMergeAction(const std::function<void(const merge::IMergeAction::Ptr&)>& functor) override;
    std::size_t getMergeActionCount() override;
    bool hasActiveActions() override;

private:
    void addPreviewNodeForAddAction();
    void removePreviewNodeForAddAction();

    std::shared_ptr<merge::AddCloneToParentAction> getAddNodeAction();
};

}
