#include "EclassModelInstance.h"

namespace entity {

EclassModelInstance::EclassModelInstance(const scene::Path& path, 
										 scene::Instance* parent, 
										 EclassModel& contained) :
	TargetableInstance(path, parent, contained.getEntity(), *this),
	TransformModifier(EclassModel::TransformChangedCaller(contained), 
					  ApplyTransformCaller(*this)),
	m_contained(contained)
{
	m_contained.instanceAttach(Instance::path());

	StaticRenderableConnectionLines::instance().attach(*this);
}

EclassModelInstance::~EclassModelInstance() {
	StaticRenderableConnectionLines::instance().detach(*this);

	m_contained.instanceDetach(Instance::path());
}

void EclassModelInstance::renderSolid(Renderer& renderer, 
	const VolumeTest& volume) const
{
	m_contained.renderSolid(renderer, volume, Instance::localToWorld(), getSelectable().isSelected());
}

void EclassModelInstance::renderWireframe(Renderer& renderer, 
	const VolumeTest& volume) const
{
	m_contained.renderWireframe(renderer, volume, Instance::localToWorld(), getSelectable().isSelected());
}

void EclassModelInstance::evaluateTransform() {
	if (getType() == TRANSFORM_PRIMITIVE) {
		m_contained.translate(getTranslation());
		m_contained.rotate(getRotation());
	}
}

void EclassModelInstance::applyTransform() {
	m_contained.revertTransform();
	evaluateTransform();
	m_contained.freezeTransform();
}

} // namespace entity
