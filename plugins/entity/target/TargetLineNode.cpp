#include "TargetLineNode.h"

#include "../EntityNode.h"

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
    return _aabb;
}

void TargetLineNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
    renderWireframe(collector, volume);
}

void TargetLineNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
    // If the owner is hidden, the lines are hidden too
    if (!_targetLines.hasTargets() || !_owner.visible()) return;

    collector.SetState(_owner.getWireShader(), RenderableCollector::eWireframeOnly);
	collector.SetState(_owner.getWireShader(), RenderableCollector::eFullMaterials);

	_targetLines.render(collector, volume, getWorldPosition());
}

bool TargetLineNode::isHighlighted() const
{
    return false;
}

const Vector3& TargetLineNode::getWorldPosition() const
{
	const AABB& bounds = _owner.worldAABB();

	if (bounds.isValid())
    {
		return bounds.getOrigin();
	}

	return _owner.localToWorld().t().getVector3();
}

}
