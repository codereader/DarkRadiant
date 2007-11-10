#include "KeyValue.h"

namespace entity {

KeyValue::KeyValue(const std::string& value, const std::string& empty) : 
	_value(value), 
	_emptyValue(empty),
	_undo(_value, UndoImportCaller(*this))
{
	notify();
}

KeyValue::~KeyValue() {
	//ASSERT_MESSAGE(_observers.empty(), "KeyValue::~KeyValue: observers still attached");
}

void KeyValue::setKeyValueChangedFunc(EntityCreator::KeyValueChangedFunc func) {
	_keyValueChangedNotify = func;
}

void KeyValue::instanceAttach(MapFile* map) {
	_undo.instanceAttach(map);
}

void KeyValue::instanceDetach(MapFile* map) {
	_undo.instanceDetach(map);
}

void KeyValue::attach(const KeyObserver& observer) {
	// Store the observer
	_observers.push_back(observer);
	
	// Notify the newly inserted observer with the existing value
	_observers.back()(get());
}

void KeyValue::detach(const KeyObserver& observer) {
	observer(_emptyValue);
	
	KeyObservers::iterator found = std::find(_observers.begin(), _observers.end(), observer);
	if (found != _observers.end()) {
		_observers.erase(found);
	}
}

std::string KeyValue::get() const {
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

void KeyValue::notify() {
	_keyValueChangedNotify();
	KeyObservers::reverse_iterator i = _observers.rbegin();
	while(i != _observers.rend()) {
		(*i++)(get());
	}
}

void KeyValue::importState(const std::string& string) {
	_value = string;
	notify();
}

} // namespace entity
