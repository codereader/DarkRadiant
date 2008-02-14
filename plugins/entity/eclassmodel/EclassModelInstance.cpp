#include "EclassModelInstance.h"
#include "../target/RenderableTargetInstances.h"

namespace entity {

EclassModelInstance::EclassModelInstance(const scene::Path& path, 
										 scene::Instance* parent, 
										 EclassModel& contained) :
	TargetableInstance(path, parent, contained.getEntity()),
	TransformModifier(EclassModel::TransformChangedCaller(contained), 
					  ApplyTransformCaller(*this)),
	m_contained(contained),
	_updateSkin(true)
{
	m_contained.instanceAttach(Instance::path());

	RenderableTargetInstances::Instance().attach(*this);

	m_contained.addKeyObserver("skin", SkinChangedCaller(*this));
}

EclassModelInstance::~EclassModelInstance() {
	m_contained.removeKeyObserver("skin", SkinChangedCaller(*this));

	RenderableTargetInstances::Instance().detach(*this);

	m_contained.instanceDetach(Instance::path());
}

void EclassModelInstance::renderSolid(Renderer& renderer, 
	const VolumeTest& volume) const
{
	// greebo: Check if the skin needs updating before rendering.
	if (_updateSkin) {
		// Instantiate a walker class equipped with the new value
		SkinChangedWalker walker(m_contained.getEntity().getKeyValue("skin"));
		// Update all children
		GlobalSceneGraph().traverse_subgraph(walker, path());

		_updateSkin = false;
	}

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

void EclassModelInstance::skinChanged(const std::string& value) {
	// Instantiate a walker class equipped with the new value
	SkinChangedWalker walker(value);
	// Update all children
	GlobalSceneGraph().traverse_subgraph(walker, path());
}

} // namespace entity
