#ifndef ENTITY_KEYVALUE_H_
#define ENTITY_KEYVALUE_H_

#include "ientity.h"
#include "undolib.h"

namespace entity {

/// \brief A key/value pair of strings.
///
/// - Notifies observers when value changes - value changes to "" on destruction.
/// - Provides undo support through the global undo system.
class KeyValue : 
	public EntityKeyValue
{
	typedef UnsortedSet<KeyObserver> KeyObservers;

	std::size_t m_refcount;
	KeyObservers m_observers;
	std::string m_string;
	const char* m_empty;
	ObservedUndoableObject<std::string> m_undo;
	static EntityCreator::KeyValueChangedFunc m_entityKeyValueChanged;
public:

	KeyValue(const char* string, const char* empty)
			: m_refcount(0), m_string(string), m_empty(empty), m_undo(m_string, UndoImportCaller(*this)) {
		notify();
	}
	~KeyValue() {
		ASSERT_MESSAGE(m_observers.empty(), "KeyValue::~KeyValue: observers still attached");
	}

	static void setKeyValueChangedFunc(EntityCreator::KeyValueChangedFunc func) {
		m_entityKeyValueChanged = func;
	}

	void IncRef() {
		++m_refcount;
	}
	void DecRef() {
		if(--m_refcount == 0) {
			delete this;
		}
	}

	void instanceAttach(MapFile* map) {
		m_undo.instanceAttach(map);
	}
	void instanceDetach(MapFile* map) {
		m_undo.instanceDetach(map);
	}

	void attach(const KeyObserver& observer) {
		(*m_observers.insert(observer))(c_str());
	}
	void detach(const KeyObserver& observer) {
		observer(m_empty);
		m_observers.erase(observer);
	}
	const char* c_str() const {
		if(string_empty(m_string.c_str())) {
			return m_empty;
		}
		return m_string.c_str();
	}
	void assign(const char* other) {
		if(!string_equal(m_string.c_str(), other)) {
			m_undo.save();
			m_string = other;
			notify();
		}
	}

	void notify() {
		m_entityKeyValueChanged();
		KeyObservers::reverse_iterator i = m_observers.rbegin();
		while(i != m_observers.rend()) {
			(*i++)(c_str());
		}
	}

	void importState(const std::string& string) {
		m_string = string;

		notify();
	}
	typedef MemberCaller1<KeyValue, const std::string&, &KeyValue::importState> UndoImportCaller;
};

} // namespace entity

#endif /*ENTITY_KEYVALUE_H_*/
