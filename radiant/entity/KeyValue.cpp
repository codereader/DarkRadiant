#include "KeyValue.h"

#include <functional>
#include "iundo.h"

namespace entity 
{

KeyValue::KeyValue(const std::string& value, const std::string& empty) :
	_value(value),
	_emptyValue(empty),
	_undo(_value, std::bind(&KeyValue::importState, this, std::placeholders::_1), "KeyValue")
{
	notify();
}

KeyValue::~KeyValue() {
	assert(_observers.empty());
}

void KeyValue::connectUndoSystem(IMapFileChangeTracker& changeTracker)
{
    _undo.connectUndoSystem(changeTracker);
}

void KeyValue::disconnectUndoSystem(IMapFileChangeTracker& changeTracker)
{
    _undo.disconnectUndoSystem(changeTracker);
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

void KeyValue::importState(const std::string& string) 
{
	// Add ourselves to the Undo event observers, to get notified after all this has been finished
	_undoHandler = GlobalUndoSystem().signal_postUndo().connect(
		sigc::mem_fun(this, &KeyValue::onUndoRedoOperationFinished));
	_redoHandler = GlobalUndoSystem().signal_postRedo().connect(
		sigc::mem_fun(this, &KeyValue::onUndoRedoOperationFinished));

	_value = string;
	notify();
}

void KeyValue::onUndoRedoOperationFinished()
{
	_undoHandler.disconnect();
	_redoHandler.disconnect();

	notify();
}

void KeyValue::onNameChange(const std::string& oldName, const std::string& newName)
{
	assert(oldName == _value); // The old name should match

	// Just assign the new name to this keyvalue
	assign(newName);
}

} // namespace entity
