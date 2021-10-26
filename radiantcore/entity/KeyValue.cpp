#include "KeyValue.h"

#include <functional>
#include "iundo.h"

namespace entity 
{

KeyValue::KeyValue(const std::string& value, const std::string& empty, 
                   const std::function<void(const std::string&)>& valueChanged) :
    _value(value),
    _emptyValue(empty),
    _undo(_value, std::bind(&KeyValue::importState, this, std::placeholders::_1), 
        std::bind(&KeyValue::onUndoRedoOperationFinished, this), "KeyValue"),
    _valueChanged(valueChanged)
{}

KeyValue::~KeyValue()
{
	assert(_observers.empty());
}

void KeyValue::connectUndoSystem(IUndoSystem& undoSystem)
{
    _undo.connectUndoSystem(undoSystem);
}

void KeyValue::disconnectUndoSystem(IUndoSystem& undoSystem)
{
    _undo.disconnectUndoSystem(undoSystem);
}

void KeyValue::attach(KeyObserver& observer)
{
	// Store the observer
	_observers.push_back(&observer);

	// Notify the newly inserted observer with the existing value
	observer.onKeyValueChanged(get());
}

void KeyValue::detach(KeyObserver& observer)
{
	observer.onKeyValueChanged(_emptyValue);

	auto found = std::find(_observers.begin(), _observers.end(), &observer);

	if (found != _observers.end())
    {
		_observers.erase(found);
	}
}

const std::string& KeyValue::get() const
{
	// Return the <empty> string if the actual value is ""
	return (_value.empty()) ? _emptyValue : _value;
}

void KeyValue::assign(const std::string& other)
{
	if (_value != other)
    {
		_undo.save();
		_value = other;
		notify();
	}
}

void KeyValue::notify()
{
	// Store the name locally, to avoid string-copy operations in the loop below
	const std::string& value = get();

    // Notify the owning SpawnArgs instance
    _valueChanged(value);

	for (auto i = _observers.rbegin(); i != _observers.rend(); ++i)
    {
		(*i)->onKeyValueChanged(value);
	}
}

void KeyValue::importState(const std::string& string) 
{
	// We notify our observers after the entire undo rollback is done
	_value = string;
}

void KeyValue::onUndoRedoOperationFinished()
{
	notify();
}

void KeyValue::onNameChange(const std::string& oldName, const std::string& newName)
{
	assert(oldName == _value); // The old name should match

	// Just assign the new name to this keyvalue
	assign(newName);
}

} // namespace entity
