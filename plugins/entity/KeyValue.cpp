#include "KeyValue.h"

namespace entity {

KeyValue::KeyValue(const std::string& string, const std::string& empty) : 
	m_string(string), 
	m_empty(empty),
	m_undo(m_string, UndoImportCaller(*this))
{
	notify();
}

KeyValue::~KeyValue() {
	ASSERT_MESSAGE(m_observers.empty(), "KeyValue::~KeyValue: observers still attached");
}

void KeyValue::setKeyValueChangedFunc(EntityCreator::KeyValueChangedFunc func) {
	m_entityKeyValueChanged = func;
}

void KeyValue::instanceAttach(MapFile* map) {
	m_undo.instanceAttach(map);
}

void KeyValue::instanceDetach(MapFile* map) {
	m_undo.instanceDetach(map);
}

void KeyValue::attach(const KeyObserver& observer) {
	(*m_observers.insert(observer))(get());
}

void KeyValue::detach(const KeyObserver& observer) {
	observer(m_empty.c_str());
	m_observers.erase(observer);
}

std::string KeyValue::get() const {
	// Return the <empty> string if the actual value is ""
	return (m_string.empty()) ? m_empty : m_string;
}

void KeyValue::assign(const std::string& other) {
	if (m_string != other) {
		m_undo.save();
		m_string = other;
		notify();
	}
}

void KeyValue::notify() {
	m_entityKeyValueChanged();
	KeyObservers::reverse_iterator i = m_observers.rbegin();
	while(i != m_observers.rend()) {
		(*i++)(get());
	}
}

void KeyValue::importState(const std::string& string) {
	m_string = string;
	notify();
}

} // namespace entity
