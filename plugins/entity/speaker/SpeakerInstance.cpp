#include "SpeakerInstance.h"

namespace entity {

SpeakerInstance::SpeakerInstance(
		const scene::Path& path, 
		scene::Instance* parent, 
		Speaker& contained) :
	TargetableInstance(path, parent, contained.getEntity()),
	TransformModifier(Speaker::TransformChangedCaller(contained), ApplyTransformCaller(*this)),
	m_contained(contained)
{
	m_contained.instanceAttach(Instance::path());
	StaticRenderableConnectionLines::instance().attach(*this);
}

SpeakerInstance::~SpeakerInstance() {
	StaticRenderableConnectionLines::instance().detach(*this);

	m_contained.instanceDetach(Instance::path());
}

// Bounded implementation
const AABB& SpeakerInstance::localAABB() const {
	return m_contained.localAABB();
}

// Cullable implementation
VolumeIntersectionValue SpeakerInstance::intersectVolume(
    const VolumeTest& test, const Matrix4& localToWorld) const
{
	return m_contained.intersectVolume(test, localToWorld);
}

void SpeakerInstance::renderSolid(Renderer& renderer, const VolumeTest& volume) const {
	m_contained.renderSolid(renderer, volume, Instance::localToWorld());
}
void SpeakerInstance::renderWireframe(Renderer& renderer, const VolumeTest& volume) const {
	m_contained.renderWireframe(renderer, volume, Instance::localToWorld());
}

void SpeakerInstance::testSelect(Selector& selector, SelectionTest& test) {
	m_contained.testSelect(selector, test, Instance::localToWorld());
}

void SpeakerInstance::evaluateTransform() {
	if(getType() == TRANSFORM_PRIMITIVE) {
		m_contained.translate(getTranslation());
		m_contained.rotate(getRotation());
	}
}

void SpeakerInstance::applyTransform() {
	m_contained.revertTransform();
	evaluateTransform();
	m_contained.freezeTransform();
}

} // namespace entity
