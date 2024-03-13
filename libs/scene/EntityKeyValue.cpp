#include "EntityKeyValue.h"

#include <functional>
#include "iundo.h"

EntityKeyValue::EntityKeyValue(const std::string& value, const std::string& empty,
                   const std::function<void(const std::string&)>& valueChanged) :
    _value(value),
    _emptyValue(empty),
    _undo(_value, std::bind(&EntityKeyValue::importState, this, std::placeholders::_1),
        std::bind(&EntityKeyValue::onUndoRedoOperationFinished, this), "KeyValue"),
    _valueChanged(valueChanged)
{}

EntityKeyValue::~EntityKeyValue()
{
	assert(_observers.empty());
}

void EntityKeyValue::connectUndoSystem(IUndoSystem& undoSystem)
{
    _undo.connectUndoSystem(undoSystem);
}

void EntityKeyValue::disconnectUndoSystem(IUndoSystem& undoSystem)
{
    _undo.disconnectUndoSystem(undoSystem);
}

void EntityKeyValue::attach(KeyObserver& observer)
{
	// Store the observer
	_observers.push_back(&observer);

	// Notify the newly inserted observer with the existing value
	observer.onKeyValueChanged(get());
}

void EntityKeyValue::detach(KeyObserver& observer, bool sendEmptyValue)
{
    // Send final empty value if requested
    if (sendEmptyValue)
        observer.onKeyValueChanged(_emptyValue);

    // Remove the observer if present
	auto found = std::find(_observers.begin(), _observers.end(), &observer);
	if (found != _observers.end())
		_observers.erase(found);
}

const std::string& EntityKeyValue::get() const
{
	// Return the <empty> string if the actual value is ""
	return (_value.empty()) ? _emptyValue : _value;
}

void EntityKeyValue::assign(const std::string& other)
{
	if (_value != other)
    {
		_undo.save();
		_value = other;
		notify();
	}
}

void EntityKeyValue::notify()
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

void EntityKeyValue::importState(const std::string& string)
{
	// We notify our observers after the entire undo rollback is done
	_value = string;
}

void EntityKeyValue::onUndoRedoOperationFinished()
{
	notify();
}

void EntityKeyValue::onNameChange(const std::string& oldName, const std::string& newName)
{
	assert(oldName == _value); // The old name should match

	// Just assign the new name to this keyvalue
	assign(newName);
}
