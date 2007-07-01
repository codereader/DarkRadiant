#ifndef ENTITY_KEYVALUE_H_
#define ENTITY_KEYVALUE_H_

#include "ientity.h"
#include "undolib.h"
#include "string/string.h"
#include "container/container.h"

namespace entity {

/// \brief A key/value pair of strings.
///
/// - Notifies observers when value changes - value changes to "" on destruction.
/// - Provides undo support through the global undo system.
class KeyValue : 
	public EntityKeyValue
{
	typedef UnsortedSet<KeyObserver> KeyObservers;

	KeyObservers m_observers;
	std::string m_string;
	std::string m_empty;
	ObservedUndoableObject<std::string> m_undo;
	static EntityCreator::KeyValueChangedFunc m_entityKeyValueChanged;
public:
	KeyValue(const std::string& string, const std::string& empty);
	
	~KeyValue();

	static void setKeyValueChangedFunc(EntityCreator::KeyValueChangedFunc func);

	void instanceAttach(MapFile* map);
	void instanceDetach(MapFile* map);

	void attach(const KeyObserver& observer);
	void detach(const KeyObserver& observer);
	
	// Accessor method, retrieve the actual value
	std::string get() const;
	
	void assign(const std::string& other);

	void notify();

	void importState(const std::string& string);
	typedef MemberCaller1<KeyValue, const std::string&, &KeyValue::importState> UndoImportCaller;
};

} // namespace entity

#endif /*ENTITY_KEYVALUE_H_*/
