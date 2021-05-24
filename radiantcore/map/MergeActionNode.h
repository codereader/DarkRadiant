#pragma once

#include "math/AABB.h"
#include "scene/SelectableNode.h"
#include "scene/merge/MergeAction.h"

namespace map
{

class MergeActionNode :
    public scene::SelectableNode
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

        // Hide the affected node itself
        _affectedNode->enable(Node::eHidden);
    }

    void onInsertIntoScene(scene::IMapRootNode& root) override
    {
        SelectableNode::onInsertIntoScene(root);

        if (_affectedNode->getRootNode() != getRootNode())
        {
            scene::addNodeToContainer(_affectedNode, getRootNode());
        }
    }

    void onRemoveFromScene(scene::IMapRootNode& root) override
    {
        SelectableNode::onRemoveFromScene(root);

        if (!_affectedNode->inScene())
        {
            scene::removeNodeFromParent(_affectedNode);
        }
    }

    void setRenderSystem(const RenderSystemPtr& renderSystem) override
    {
        SelectableNode::setRenderSystem(renderSystem);

        _affectedNode->setRenderSystem(renderSystem);
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
        _affectedNode->renderSolid(collector, volume);
    }

    void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const override
    {
        _affectedNode->renderWireframe(collector, volume);
    }

    std::size_t getHighlightFlags() override
    {
        return Highlight::MergeAction;
    }
};

}
