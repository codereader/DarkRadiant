#include "TargetKey.h"

#include "TargetManager.h"

namespace entity {

const TargetPtr& TargetKey::getTarget() const
{
	return _target;
}

void TargetKey::attachToKeyValue(EntityKeyValue& value)
{
	// Observe this entity keyvalue
	value.attach(*this);
}

void TargetKey::detachFromKeyValue(EntityKeyValue& value)
{
	// Stop observing this KeyValue
	value.detach(*this);
}

void TargetKey::onKeyValueChanged(const std::string& newValue)
{
	// Acquire the Target object (will be created if nonexistent)
	_target = TargetManager::Instance().getTarget(newValue);
}

} // namespace entity
