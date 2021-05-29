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

public:
    using Ptr = std::shared_ptr<MergeActionNode>;

    MergeActionNode(const scene::merge::MergeAction::Ptr& action) :
        _action(action)
    {
        _affectedNode = _action->getAffectedNode();

        auto addNodeAction = std::dynamic_pointer_cast<scene::merge::AddCloneToParentAction>(_action);

        if (addNodeAction)
        {
            // Get the clone and add it to the target scene, it needs to be renderable here
            scene::addNodeToContainer(_affectedNode, addNodeAction->getParent());
        }

        // Hide the affected node itself, we're doing the rendering ourselves, recursively
        _affectedNode->enable(Node::eHidden);

        _affectedNode->foreachNode([&](const scene::INodePtr& child)
        {
            child->enable(Node::eHidden);
            return true;
        });
    }

    void onRemoveFromScene(scene::IMapRootNode& rootNode) override
    {
        auto addNodeAction = std::dynamic_pointer_cast<scene::merge::AddCloneToParentAction>(_action);

        if (addNodeAction)
        {
            scene::removeNodeFromParent(_affectedNode);
        }

        // Release the hidden state of the contained nodes
        _affectedNode->disable(Node::eHidden);

        _affectedNode->foreachNode([&](const scene::INodePtr& child)
        {
            child->disable(Node::eHidden);
            return true;
        });

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
        return Highlight::NoHighlight;
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
};

}
