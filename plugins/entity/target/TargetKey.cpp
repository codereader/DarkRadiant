#include "TargetKey.h"

#include "TargetManager.h"

namespace entity {

const TargetPtr& TargetKey::getTarget() const {
	return _target;
}

void TargetKey::attachToKeyValue(EntityKeyValue& value) {
	// Observe this entity keyvalue
	value.attach(TargetChangedCaller(*this));
}

void TargetKey::detachFromKeyValue(EntityKeyValue& value) {
	// Stop observing this KeyValue
	value.detach(TargetChangedCaller(*this));
}

void TargetKey::targetChanged(const std::string& target) {
	// Acquire the Target object (will be created if nonexistent)
	_target = TargetManager::Instance().getTarget(target);
}

} // namespace entity
