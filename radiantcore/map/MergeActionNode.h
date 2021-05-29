#pragma once

#include "imergeaction.h"
#include "iselectiontest.h"
#include "math/AABB.h"
#include "scene/SelectableNode.h"
#include "scene/merge/MergeAction.h"

namespace map
{

class MergeActionNode final :
    public scene::IMergeActionNode,
    public scene::SelectableNode,
    public SelectionTestable
{
private:
    scene::merge::MergeAction::Ptr _action;
    scene::INodePtr _affectedNode;

    bool _syncActionStatus;

public:
    using Ptr = std::shared_ptr<MergeActionNode>;

    MergeActionNode(const scene::merge::MergeAction::Ptr& action) :
        _action(action),
        _syncActionStatus(true)
    {
        _affectedNode = _action->getAffectedNode();
    }

    // Prepare this node right before a merge, such that it
    // doesn't change the action's status when removed from the scene
    void prepareForMerge()
    {
        _syncActionStatus = false;
    }

    void onInsertIntoScene(scene::IMapRootNode& rootNode) override
    {
        SelectableNode::onInsertIntoScene(rootNode);

        if (_syncActionStatus)
        {
            _action->activate();
        }

        addPreviewNodeForAddAction();
        hideAffectedNodes();
    }

    void onRemoveFromScene(scene::IMapRootNode& rootNode) override
    {
        unhideAffectedNodes();
        removePreviewNodeForAddAction();

        if (_syncActionStatus)
        {
            _action->deactivate();
        }

        SelectableNode::onRemoveFromScene(rootNode);
    }

    scene::merge::ActionType getActionType() const override
    {
        return _action->getType();
    }

    scene::INode::Type getNodeType() const override
    {
        return scene::INode::Type::MergeAction;
    }

    bool supportsStateFlag(unsigned int state) const override
    {
        if ((state & (eHidden|eFiltered|eExcluded|eLayered)) != 0)
        {
            return false; // don't allow this node to be hidden
        }

        return Node::supportsStateFlag(state);
    }

    const AABB& localAABB() const override
    {
        return _affectedNode->localAABB();
    }

    void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const override
    {
        _affectedNode->viewChanged();
        _affectedNode->renderSolid(collector, volume);
        _affectedNode->foreachNode([&](const scene::INodePtr& child)
        {
            child->viewChanged();
            child->renderSolid(collector, volume);
            return true;
        });
    }

    void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const override
    {
        _affectedNode->viewChanged();
        _affectedNode->renderWireframe(collector, volume);
        _affectedNode->foreachNode([&](const scene::INodePtr& child)
        {
            child->viewChanged();
            child->renderWireframe(collector, volume);
            return true;
        });
    }

    std::size_t getHighlightFlags() override
    {
        return isSelected() ? Highlight::Selected : Highlight::NoHighlight;
    }

    void testSelect(Selector& selector, SelectionTest& test) override
    {
        testSelectNode(_affectedNode, selector, test);

        _affectedNode->foreachNode([&](const scene::INodePtr& child)
        {
            testSelectNode(child, selector, test);
            return true;
        });
    }

private:
    void testSelectNode(const scene::INodePtr& node, Selector& selector, SelectionTest& test)
    {
        auto selectionTestable = std::dynamic_pointer_cast<SelectionTestable>(node);

        // Regardless of what node we test, it will always be the MergeActionNode that will be selected
        selector.pushSelectable(*this);

        if (selectionTestable)
        {
            selectionTestable->testSelect(selector, test);
        }

        selector.popSelectable();
    }

    void hideAffectedNodes()
    {
        // Hide the affected node itself, we're doing the rendering ourselves, recursively
        _affectedNode->enable(Node::eExcluded);

        _affectedNode->foreachNode([&](const scene::INodePtr& child)
        {
            child->enable(Node::eExcluded);
            return true;
        });
    }

    void unhideAffectedNodes()
    {
        // Release the excluded state of the contained nodes
        _affectedNode->disable(Node::eExcluded);

        _affectedNode->foreachNode([&](const scene::INodePtr& child)
        {
            child->disable(Node::eExcluded);
            return true;
        });
    }

    void addPreviewNodeForAddAction()
    {
        // We add the node to the target scene, for preview purposes
        auto addNodeAction = std::dynamic_pointer_cast<scene::merge::AddCloneToParentAction>(_action);

        if (addNodeAction)
        {
            // Get the clone and add it to the target scene, it needs to be renderable here
            scene::addNodeToContainer(_affectedNode, addNodeAction->getParent());
        }
    }

    void removePreviewNodeForAddAction()
    {
        auto addNodeAction = std::dynamic_pointer_cast<scene::merge::AddCloneToParentAction>(_action);

        if (addNodeAction)
        {
            scene::removeNodeFromParent(_affectedNode);
        }
    }
};

}
