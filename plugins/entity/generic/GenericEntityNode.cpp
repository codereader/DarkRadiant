#include "GenericEntityNode.h"

#include "math/frustum.h"

namespace entity {

GenericEntityNode::GenericEntityNode(const IEntityClassConstPtr& eclass) :
	EntityNode(eclass),
	m_contained(*this, 
		Node::TransformChangedCaller(*this))
{}

GenericEntityNode::GenericEntityNode(const GenericEntityNode& other) :
	EntityNode(other),
	scene::Cloneable(other),
	Snappable(other),
	TransformNode(other),
	SelectionTestable(other),
	Cullable(other),
	Bounded(other),
	m_contained(other.m_contained, 
		*this, 
		Node::TransformChangedCaller(*this))
{}

// Snappable implementation
void GenericEntityNode::snapto(float snap) {
	m_contained.snapto(snap);
}

// TransformNode implementation
const Matrix4& GenericEntityNode::localToParent() const {
	return m_contained.getTransformNode().localToParent();
}

// EntityNode implementation
Entity& GenericEntityNode::getEntity() {
	return _entity;
}

void GenericEntityNode::refreshModel() {
	// nothing to do
}

// Bounded implementation
const AABB& GenericEntityNode::localAABB() const {
	return m_contained.localAABB();
}

// Cullable implementation
VolumeIntersectionValue GenericEntityNode::intersectVolume(
    const VolumeTest& test, const Matrix4& localToWorld) const
{
	return m_contained.intersectVolume(test, localToWorld);
}

void GenericEntityNode::testSelect(Selector& selector, SelectionTest& test) {
	m_contained.testSelect(selector, test, localToWorld());
}

scene::INodePtr GenericEntityNode::clone() const {
	scene::INodePtr clone(new GenericEntityNode(*this));
	clone->setSelf(clone);
	return clone;
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

} // namespace entity
