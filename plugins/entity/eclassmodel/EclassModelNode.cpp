#include "EclassModelNode.h"

#include <boost/bind.hpp>

namespace entity {

EclassModelNode::EclassModelNode(const IEntityClassPtr& eclass) :
	EntityNode(eclass),
	m_contained(*this, Callback(boost::bind(&Node::transformChanged, this))),
	_localAABB(Vector3(0,0,0), Vector3(1,1,1)) // minimal AABB, is determined by child bounds anyway
{}

EclassModelNode::EclassModelNode(const EclassModelNode& other) :
	EntityNode(other),
	Snappable(other),
	m_contained(other.m_contained,
				*this,
				Callback(boost::bind(&Node::transformChanged, this))),
	_localAABB(Vector3(0,0,0), Vector3(1,1,1)) // minimal AABB, is determined by child bounds anyway
{}

EclassModelNodePtr EclassModelNode::Create(const IEntityClassPtr& eclass)
{
	EclassModelNodePtr instance(new EclassModelNode(eclass));
	instance->construct();

	return instance;
}

void EclassModelNode::construct()
{
	EntityNode::construct();

	m_contained.construct();
}

// Snappable implementation
void EclassModelNode::snapto(float snap) {
	m_contained.snapto(snap);
}

const AABB& EclassModelNode::localAABB() const
{
	return _localAABB;
}

void EclassModelNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderSolid(collector, volume);

	m_contained.renderSolid(collector, volume, localToWorld(), isSelected());
}

void EclassModelNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderWireframe(collector, volume);

	m_contained.renderWireframe(collector, volume, localToWorld(), isSelected());
}

void EclassModelNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	EntityNode::setRenderSystem(renderSystem);

	m_contained.setRenderSystem(renderSystem);
}

scene::INodePtr EclassModelNode::clone() const
{
	EclassModelNodePtr node(new EclassModelNode(*this));
	node->construct();

	return node;
}

void EclassModelNode::_onTransformationChanged()
{
	if (getType() == TRANSFORM_PRIMITIVE)
	{
		m_contained.revertTransform();

		m_contained.translate(getTranslation());
		m_contained.rotate(getRotation());

		m_contained.updateTransform();
	}
}

void EclassModelNode::_applyTransformation()
{
	if (getType() == TRANSFORM_PRIMITIVE)
	{
		m_contained.revertTransform();

		m_contained.translate(getTranslation());
		m_contained.rotate(getRotation());

		m_contained.freezeTransform();
	}
}

} // namespace entity
