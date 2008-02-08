#include "GenericEntityInstance.h"

namespace entity {

GenericEntityInstance::GenericEntityInstance(
		const scene::Path& path, 
		scene::Instance* parent, 
		GenericEntity& contained) :
	TargetableInstance(path, parent, contained.getEntity()),
	TransformModifier(GenericEntity::TransformChangedCaller(contained), ApplyTransformCaller(*this)),
	m_contained(contained)
{
	m_contained.instanceAttach(Instance::path());
	StaticRenderableConnectionLines::instance().attach(*this);
}

GenericEntityInstance::~GenericEntityInstance() {
	StaticRenderableConnectionLines::instance().detach(*this);

	m_contained.instanceDetach(Instance::path());
}

// Bounded implementation
const AABB& GenericEntityInstance::localAABB() const {
	return m_contained.localAABB();
}

// Cullable implementation
VolumeIntersectionValue GenericEntityInstance::intersectVolume(
    const VolumeTest& test, const Matrix4& localToWorld) const
{
	return m_contained.intersectVolume(test, localToWorld);
}

void GenericEntityInstance::renderSolid(Renderer& renderer, const VolumeTest& volume) const {
	m_contained.renderSolid(renderer, volume, Instance::localToWorld());
}
void GenericEntityInstance::renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
	m_contained.renderWireframe(renderer, volume, Instance::localToWorld());
}

void GenericEntityInstance::testSelect(Selector& selector, SelectionTest& test) {
	m_contained.testSelect(selector, test, Instance::localToWorld());
}

void GenericEntityInstance::evaluateTransform() {
	if(getType() == TRANSFORM_PRIMITIVE) {
		m_contained.translate(getTranslation());
		m_contained.rotate(getRotation());
	}
}

void GenericEntityInstance::applyTransform() {
	m_contained.revertTransform();
	evaluateTransform();
	m_contained.freezeTransform();
}

} // namespace entity
