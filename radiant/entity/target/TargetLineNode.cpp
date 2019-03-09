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

	_targetLines.render(_owner.getWireShader(), collector, volume, getOwnerPosition());
}

std::size_t TargetLineNode::getHighlightFlags()
{
    // We don't need to return highlighting, since the render system will use 
    // the result of the parent entity node
    return Highlight::NoHighlight;
}

const Vector3& TargetLineNode::getOwnerPosition() const
{
	const AABB& bounds = _owner.worldAABB();

	if (bounds.isValid())
    {
		return bounds.getOrigin();
	}

	return _owner.localToWorld().t().getVector3();
}

}
