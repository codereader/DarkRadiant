#include "TargetableInstance.h"

#include "TargetManager.h"

namespace entity {

TargetableInstance::TargetableInstance(const scene::Path& path, 
									   scene::Instance* parent, 
									   Doom3Entity& entity) :
	SelectableInstance(path, parent),
	_entity(entity),
	_renderableLines(_targetKeys)
{
	_entity.attach(*this);
	_entity.attach(_targetKeys);
}

TargetableInstance::~TargetableInstance() {
	_entity.detach(_targetKeys);
	_entity.detach(*this);
}

void TargetableInstance::setTargetsChanged(const Callback& targetsChanged) {
	_targetKeys.setTargetsChanged(targetsChanged);
}

void TargetableInstance::targetsChanged() {
	_targetKeys.targetsChanged();
}

// Gets called as soon as the "name" keyvalue changes
void TargetableInstance::targetnameChanged(const std::string& name) {
	// Check if we were registered before
	if (!_targetName.empty()) {
		// Old name is not empty
		// Tell the Manager to disassociate us from the target
		TargetManager::Instance().clearTarget(_targetName);
	}
	
	// Store the new name, in any case
	_targetName = name;

	if (_targetName.empty()) {
		// New name is empty, do not associate
		return;
	}

	// Tell the TargetManager to associate the name with this Instance here
	TargetManager::Instance().associateTarget(_targetName, this);
}

// Entity::Observer implementation, gets called on key insert
void TargetableInstance::onKeyInsert(const std::string& key, EntityKeyValue& value) {
	if (key == "name") {
		// Subscribe to this keyvalue to get notified about "name" changes
		value.attach(TargetnameChangedCaller(*this));
	}
}

// Entity::Observer implementation, gets called on key erase
void TargetableInstance::onKeyErase(const std::string& key, EntityKeyValue& value) {
	if (key == "name") {
		// Unsubscribe from this keyvalue
		value.detach(TargetnameChangedCaller(*this));
	}
}

const Vector3& TargetableInstance::getWorldPosition() const {
	const AABB& bounds = Instance::worldAABB();
	if (bounds.isValid()) {
		return bounds.getOrigin();
	}
	return localToWorld().t().getVector3();
}

void TargetableInstance::render(Renderer& renderer, const VolumeTest& volume) const {
	renderer.SetState(_entity.getEntityClass()->getWireShader(), Renderer::eWireframeOnly);
	renderer.SetState(_entity.getEntityClass()->getWireShader(), Renderer::eFullMaterials);
	_renderableLines.render(renderer, volume, getWorldPosition());
}

} // namespace entity
