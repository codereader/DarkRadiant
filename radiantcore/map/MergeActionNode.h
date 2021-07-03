#pragma once

#include "imapmerge.h"
#include "iselectiontest.h"
#include "math/AABB.h"
#include "scene/SelectableNode.h"
#include "scene/merge/MergeAction.h"

namespace map
{

/**
 * Common node type for visualising a merge action in the scene, targeting a single node.
 * Can be selected and tested for selection, but cannot be hidden.
 * The node affected by the operation is "taken over" and is hidden while this node is active.
 * Rendering of the node is explicitly performed by this merge node.
 */
class MergeActionNodeBase :
    public scene::IMergeActionNode,
    public scene::SelectableNode,
    public SelectionTestable
{
protected:
    scene::INodePtr _affectedNode;
    bool _syncActionStatus;

    MergeActionNodeBase();

public:
    using Ptr = std::shared_ptr<MergeActionNodeBase>;

    // Prepare this node right before a merge, such that it
    // doesn't change the action's status when removed from the scene
    void prepareForMerge();

    scene::INodePtr getAffectedNode() override;

    // Clears the references to actions and nodes, such that it doesn't hold 
    // any strong refs to scene::Nodes, causing trouble when clearing the undo stack later
    virtual void clear();

    virtual void onInsertIntoScene(scene::IMapRootNode& rootNode) override;
    virtual void onRemoveFromScene(scene::IMapRootNode& rootNode) override;

    scene::INode::Type getNodeType() const override;

    bool supportsStateFlag(unsigned int state) const override;

    const AABB& localAABB() const override;

    void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const override;
    void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const override;

    std::size_t getHighlightFlags() override;

    void testSelect(Selector& selector, SelectionTest& test) override;

private:
    void testSelectNode(const scene::INodePtr& node, Selector& selector, SelectionTest& test);

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
    std::vector<scene::merge::IMergeAction::Ptr> _actions;

public:
    KeyValueMergeActionNode(const std::vector<scene::merge::IMergeAction::Ptr>& actions);

    void clear() override;

    scene::merge::ActionType getActionType() const override;
    void foreachMergeAction(const std::function<void(const scene::merge::IMergeAction::Ptr&)>& functor) override;
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
    scene::merge::IMergeAction::Ptr _action;

public:
    RegularMergeActionNode(const scene::merge::IMergeAction::Ptr& action);

    void onInsertIntoScene(scene::IMapRootNode& rootNode) override;
    void onRemoveFromScene(scene::IMapRootNode& rootNode) override;

    void clear() override;

    scene::merge::ActionType getActionType() const override;
    void foreachMergeAction(const std::function<void(const scene::merge::IMergeAction::Ptr&)>& functor) override;
    std::size_t getMergeActionCount() override;
    bool hasActiveActions() override;

private:
    void addPreviewNodeForAddAction();
    void removePreviewNodeForAddAction();

    std::shared_ptr<scene::merge::AddCloneToParentAction> getAddNodeAction();
};

}
