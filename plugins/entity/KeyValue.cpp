#include "KeyValue.h"

namespace entity {

KeyValue::KeyValue(const std::string& string, const std::string& empty) : 
	m_refcount(0), 
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

void KeyValue::IncRef() {
	++m_refcount;
}

void KeyValue::DecRef() {
	if(--m_refcount == 0) {
		delete this;
	}
}

void KeyValue::instanceAttach(MapFile* map) {
	m_undo.instanceAttach(map);
}

void KeyValue::instanceDetach(MapFile* map) {
	m_undo.instanceDetach(map);
}

void KeyValue::attach(const KeyObserver& observer) {
	(*m_observers.insert(observer))(c_str());
}

void KeyValue::detach(const KeyObserver& observer) {
	observer(m_empty.c_str());
	m_observers.erase(observer);
}

const char* KeyValue::c_str() const {
	if (m_string.empty()) {
		return m_empty.c_str();
	}
	return m_string.c_str();
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
		(*i++)(c_str());
	}
}

void KeyValue::importState(const std::string& string) {
	m_string = string;
	notify();
}

} // namespace entity
