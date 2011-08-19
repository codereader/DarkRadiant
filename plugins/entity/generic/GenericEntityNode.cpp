#include "GenericEntityNode.h"

#include "math/Frustum.h"

namespace entity {

GenericEntityNode::GenericEntityNode(const IEntityClassPtr& eclass) :
	EntityNode(eclass),
	m_contained(*this),
	_localPivot(Matrix4::getIdentity())
{}

GenericEntityNode::GenericEntityNode(const GenericEntityNode& other) :
	EntityNode(other),
	Snappable(other),
	m_contained(other.m_contained, *this),
	_localPivot(other._localPivot)
{}

GenericEntityNodePtr GenericEntityNode::Create(const IEntityClassPtr& eclass)
{
	GenericEntityNodePtr instance(new GenericEntityNode(eclass));
	instance->construct();

	return instance;
}

void GenericEntityNode::construct()
{
	EntityNode::construct();

	m_contained.construct();
}

// Snappable implementation
void GenericEntityNode::snapto(float snap) {
	m_contained.snapto(snap);
}

// Bounded implementation
const AABB& GenericEntityNode::localAABB() const {
	return m_contained.localAABB();
}

void GenericEntityNode::testSelect(Selector& selector, SelectionTest& test)
{
	EntityNode::testSelect(selector, test);

	m_contained.testSelect(selector, test, localToWorld());
}

scene::INodePtr GenericEntityNode::clone() const
{
	GenericEntityNodePtr node(new GenericEntityNode(*this));
	node->construct();

	return node;
}

void GenericEntityNode::renderSolid(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderSolid(collector, volume);

	m_contained.renderSolid(collector, volume, localToWorld());
}

void GenericEntityNode::renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const
{
	EntityNode::renderWireframe(collector, volume);

	m_contained.renderWireframe(collector, volume, localToWorld());
}

const Vector3& GenericEntityNode::getDirection() const
{
	// Return the direction as specified by the angle/rotation keys
	return m_contained.getDirection();
}

void GenericEntityNode::_onTransformationChanged()
{
	if (getType() == TRANSFORM_PRIMITIVE)
	{
		m_contained.revertTransform();

		m_contained.translate(getTranslation());
		m_contained.rotate(getRotation());

		m_contained.updateTransform();
	}
}

void GenericEntityNode::_applyTransformation()
{
	if (getType() == TRANSFORM_PRIMITIVE)
	{
		m_contained.revertTransform();

		m_contained.translate(getTranslation());
		m_contained.rotate(getRotation());

		m_contained.freezeTransform();
	}
}

const Matrix4& GenericEntityNode::getLocalPivot() const
{
	return _localPivot;
}

} // namespace entity
