#pragma once

#include "ientity.h"
#include <sigc++/connection.h>

#include "Target.h"

namespace entity
{

class TargetKeyCollection;

/**
 * greebo: A TargetKey represents a "targetN" key of a given entity.
 * It acts as Observer for this key and maintains a reference to
 * the named Target.
 *
 * Note: An Entity can have multiple "targetN" keys, hence it can hold
 * multiple instances of this TargetKey class. They are stored in and
 * maintainted by the TargetKeyCollection container.
 *
 * Note: Each TargetKey instance can only refer to one Target.
 */
class TargetKey :
	public KeyObserver
{
private:
    TargetKeyCollection& _owner;

    std::string _curValue;

	// The target this key is pointing to (can be empty)
	TargetPtr _target;

    sigc::connection _positionChangedSignal;
public:
    TargetKey(TargetKeyCollection& owner);

	// Accessor method for the contained TargetPtr
    const TargetPtr& getTarget() const;

    // Called when the owning TargetableNode is inserted into or removed from the scene
    void onTargetManagerChanged();

	// Observes the given keyvalue
	void attachToKeyValue(EntityKeyValue& value);

	// Stops observing the given keyvalue
	void detachFromKeyValue(EntityKeyValue& value);

	// This gets called as soon as the "target" key in the spawnargs changes
	void onKeyValueChanged(const std::string& newValue) override;

private:
    void onTargetPositionChanged();
};

} // namespace entity
