#ifndef ENTITY_KEYVALUE_H_
#define ENTITY_KEYVALUE_H_

#include "ientity.h"
#include "undolib.h"
#include "string/string.h"
#include <vector>

namespace entity {

/// \brief A key/value pair of strings.
///
/// - Notifies observers when value changes - value changes to "" on destruction.
/// - Provides undo support through the global undo system.
class KeyValue : 
	public EntityKeyValue
{
	typedef std::vector<KeyObserver> KeyObservers;
	KeyObservers _observers;
	
	std::string _value;
	std::string _emptyValue;
	ObservedUndoableObject<std::string> _undo;
	static EntityCreator::KeyValueChangedFunc _keyValueChangedNotify;
public:
	KeyValue(const std::string& value, const std::string& empty);
	
	~KeyValue();

	static void setKeyValueChangedFunc(EntityCreator::KeyValueChangedFunc func);

	void instanceAttach(MapFile* map);
	void instanceDetach(MapFile* map);

	void attach(const KeyObserver& observer);
	typedef MemberCaller1<EntityKeyValue, const KeyObserver&, &EntityKeyValue::attach> AttachCaller;
	
	void detach(const KeyObserver& observer);
	typedef MemberCaller1<EntityKeyValue, const KeyObserver&, &EntityKeyValue::detach> DetachCaller;
	
	// Accessor method, retrieve the actual value
	std::string get() const;
	
	void assign(const std::string& other);
	typedef MemberCaller1<EntityKeyValue, const std::string&, &EntityKeyValue::assign> AssignCaller;

	void notify();

	void importState(const std::string& string);
	typedef MemberCaller1<KeyValue, const std::string&, &KeyValue::importState> UndoImportCaller;
};

} // namespace entity

#endif /*ENTITY_KEYVALUE_H_*/
