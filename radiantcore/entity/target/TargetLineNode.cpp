#include "TargetLineNode.h"

#include "../EntityNode.h"
#include "ilightnode.h"

namespace entity
{

TargetLineNode::TargetLineNode(EntityNode& owner) :
    scene::Node(),
    _owner(owner),
    _targetLines(_owner, _owner.getTargetKeys())
{
    _owner.getTargetKeys().signal_TargetPositionChanged().connect(
        sigc::mem_fun(this, &TargetLineNode::queueRenderableUpdate)
    );
}

TargetLineNode::TargetLineNode(TargetLineNode& other) :
    scene::Node(other),
    _owner(other._owner),
    _targetLines(_owner, _owner.getTargetKeys())
{
    _owner.getTargetKeys().signal_TargetPositionChanged().connect(
        sigc::mem_fun(this, &TargetLineNode::queueRenderableUpdate)
    );
}

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

    _targetLines.clear();
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

    _targetLines.update(_owner.getColourShader(), getOwnerPosition());
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
        // Once this node is shown again, the onPreRender method will
        // call RenderableTargetLines::update()
        _targetLines.clear();
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

void TargetLineNode::queueRenderableUpdate()
{
    _targetLines.queueUpdate();
}

}
