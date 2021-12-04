#include "TargetLineNode.h"

#include "../EntityNode.h"
#include "ilightnode.h"

namespace entity
{

TargetLineNode::TargetLineNode(EntityNode& owner) :
    scene::Node(),
    _owner(owner),
    _targetLines(_owner.getTargetKeys())
{}

TargetLineNode::TargetLineNode(TargetLineNode& other) :
    scene::Node(other),
    _owner(other._owner),
    _targetLines(_owner.getTargetKeys())
{}

scene::INode::Type TargetLineNode::getNodeType() const
{
    return Type::EntityConnection;
}

const AABB& TargetLineNode::localAABB() const
{
    static AABB _aabb;
    return _aabb;
}

void TargetLineNode::onInsertIntoScene(scene::IMapRootNode& root)
{
    Node::onInsertIntoScene(root);

    _targetLines.queueUpdate();
}

void TargetLineNode::onRemoveFromScene(scene::IMapRootNode& root)
{
    Node::onRemoveFromScene(root);

    _targetLines.clear();
}

void TargetLineNode::onPreRender(const VolumeTest& volume)
{
    // If the owner is hidden, the lines are hidden too
    if (!_targetLines.hasTargets() || !_owner.visible())
    {
        // Hide ourselves
        _targetLines.clear();
        return;
    }

    _targetLines.update(_owner.getWireShader(), getOwnerPosition());
}

void TargetLineNode::renderSolid(IRenderableCollector& collector, const VolumeTest& volume) const
{
}

void TargetLineNode::renderWireframe(IRenderableCollector& collector, const VolumeTest& volume) const
{
}

void TargetLineNode::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
    collector.addHighlightRenderable(_targetLines, Matrix4::getIdentity());
}

void TargetLineNode::onRenderSystemChanged()
{
    _targetLines.clear();
    _targetLines.queueUpdate();
}

void TargetLineNode::onVisibilityChanged(bool visible)
{
    Node::onVisibilityChanged(visible);

    if (!visible)
    {
        // Disconnect our renderable when the node is hidden
        _targetLines.clear();
    }
    else
    {
        // Update the vertex buffers next time we need to render
        _targetLines.queueUpdate();
    }
}

std::size_t TargetLineNode::getHighlightFlags()
{
    // We don't need to return highlighting, since the render system will use
    // the result of the parent entity node
    return Highlight::NoHighlight;
}

Vector3 TargetLineNode::getOwnerPosition() const
{
    // Try to use the origin if this is a light
    auto* light = dynamic_cast<ILightNode*>(&_owner);

    if (!light)
    {
        const AABB& bounds = _owner.worldAABB();

        if (bounds.isValid())
        {
            return bounds.getOrigin();
        }

        return _owner.localToWorld().translation();
    }

    return light->getSelectAABB().getOrigin();
}

}
