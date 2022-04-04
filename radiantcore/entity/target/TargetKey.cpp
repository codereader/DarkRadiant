#include "TargetKey.h"

#include "TargetManager.h"
#include "TargetKeyCollection.h"

namespace entity {

TargetKey::TargetKey(TargetKeyCollection& owner) :
    _owner(owner)
{}

void TargetKey::onTargetManagerChanged()
{
    ITargetManager* manager = _owner.getTargetManager();

    if (manager == nullptr)
    {
        _positionChangedSignal.disconnect();
        _target.reset();
        return;
    }

    _target = std::static_pointer_cast<Target>(manager->getTarget(_curValue));
    assert(_target);

    _target->signal_TargetChanged().connect(sigc::mem_fun(this, &TargetKey::onTargetPositionChanged));
}

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
    _curValue = newValue;

    ITargetManager* targetManager = _owner.getTargetManager();

    if (targetManager != nullptr)
    {
        // If we have a target manager, acquire the Target right away
        // Acquire the Target object (will be created if nonexistent)
        _positionChangedSignal.disconnect();

        _target = std::static_pointer_cast<Target>(targetManager->getTarget(_curValue));
        assert(_target);

        _target->signal_TargetChanged().connect(sigc::mem_fun(this, &TargetKey::onTargetPositionChanged));
    }
}

void TargetKey::onTargetPositionChanged()
{
    _owner.onTargetPositionChanged();
}

} // namespace entity
