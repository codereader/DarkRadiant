#include "KeyValueObserver.h"

#include "inamespace.h"
#include "ientity.h"

namespace entity {

KeyValueObserver::KeyValueObserver(EntityKeyValue& keyValue, INamespace* ns) :
	_keyValue(keyValue),
	_namespace(ns),
	_observing(false)
{
	assert(_namespace != NULL);

	_keyValue.attach(ValueChangeCallback(*this));
}

KeyValueObserver::~KeyValueObserver() {
	_keyValue.detach(ValueChangeCallback(*this));
}

// A callback compatible with the KeyObserver declaration in ientity.h
// This gets called when the observed KeyValue changes.
void KeyValueObserver::onValueChange(const std::string& newValue) {
	assert(_namespace != NULL);

	if (_observing) {
		// The key value was already observing as a NameObserver, 
		// so let's unregister first. It's likely that the new value
		// will not point to a name anymore
		_namespace->removeNameObserver(_observedValue, _keyValue);
		_observing = false;
	}
	
	// Check if the new value is a name
	if (_namespace->nameExists(newValue)) {
		// Gotcha, the new value is pointing to a name
		_observedValue = newValue;
		_observing = true;

		// Register the KeyValue as nameobserver
		_namespace->addNameObserver(_observedValue, _keyValue);
	}
}

} // namespace entity
