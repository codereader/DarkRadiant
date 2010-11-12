#include "KeyValue.h"

#include <boost/bind.hpp>

namespace entity {

KeyValue::KeyValue(const std::string& value, const std::string& empty) :
	_value(value),
	_emptyValue(empty),
	_undo(_value, boost::bind(&KeyValue::importState, this, _1))
{
	notify();
}

KeyValue::~KeyValue() {
	assert(_observers.empty());
}

void KeyValue::instanceAttach(MapFile* map) {
	_undo.instanceAttach(map);
}

void KeyValue::instanceDetach(MapFile* map) {
	_undo.instanceDetach(map);
}

void KeyValue::attach(KeyObserver& observer) {
	// Store the observer
	_observers.push_back(&observer);

	// Notify the newly inserted observer with the existing value
	observer.onKeyValueChanged(get());
}

void KeyValue::detach(KeyObserver& observer)
{
	observer.onKeyValueChanged(_emptyValue);

	KeyObservers::iterator found = std::find(_observers.begin(), _observers.end(), &observer);
	if (found != _observers.end()) {
		_observers.erase(found);
	}
}

const std::string& KeyValue::get() const {
	// Return the <empty> string if the actual value is ""
	return (_value.empty()) ? _emptyValue : _value;
}

void KeyValue::assign(const std::string& other) {
	if (_value != other) {
		_undo.save();
		_value = other;
		notify();
	}
}

void KeyValue::notify()
{
	// Store the name locally, to avoid string-copy operations in the loop below
	const std::string& value = get();

	KeyObservers::reverse_iterator i = _observers.rbegin();
	while(i != _observers.rend()) {
		(*i++)->onKeyValueChanged(value);
	}
}

void KeyValue::importState(const std::string& string) {
	// Add ourselves to the Undo event observers, to get notified after all this has been finished
	GlobalUndoSystem().addObserver(this);

	_value = string;
	notify();
}

void KeyValue::postUndo() {
	GlobalUndoSystem().removeObserver(this);
	notify();
}

void KeyValue::postRedo() {
	GlobalUndoSystem().removeObserver(this);
	notify();
}

void KeyValue::onNameChange(const std::string& oldName, const std::string& newName) {
	assert(oldName == _value); // The old name should match

	// Just assign the new name to this keyvalue
	assign(newName);
}

} // namespace entity
