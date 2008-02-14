#include "TargetKeyCollection.h"

#include <boost/algorithm/string/predicate.hpp>

namespace entity {

void TargetKeyCollection::forEachTarget(Visitor& visitor) const {
	for (TargetKeyMap::const_iterator i = _targetKeys.begin(); 
		 i != _targetKeys.end(); i++)
	{
		visitor.visit(i->second.getTarget());
	}
}

bool TargetKeyCollection::empty() const {
	return _targetKeys.empty();
}

bool TargetKeyCollection::isTargetKey(const std::string& key) {
	// A key is a target key if it starts with "target" (any case)
	return (boost::algorithm::istarts_with(key, "target"));
}

void TargetKeyCollection::setTargetsChanged(const Callback& targetsChanged) {
	_targetsChanged = targetsChanged;
}

void TargetKeyCollection::targetsChanged() {
	_targetsChanged();
}

// Entity::Observer implementation, gets called on key insert
void TargetKeyCollection::onKeyInsert(const std::string& key, EntityKeyValue& value) {
	// ignore non-target keys
	if (!isTargetKey(key)) {
		return; 
	}

	TargetKeyMap::iterator i = _targetKeys.insert(
		TargetKeyMap::value_type(key, entity::TargetKey())
	).first;
	
	i->second.attachToKeyValue(value);
	//value.attach(TargetingEntity::TargetChangedCaller(i->second));
	targetsChanged();
}

// Entity::Observer implementation, gets called on key erase
void TargetKeyCollection::onKeyErase(const std::string& key, EntityKeyValue& value) {
	// ignore non-target keys
	if (!isTargetKey(key)) {
		return; 
	}

	TargetKeyMap::iterator i = _targetKeys.find(key);
	
	// This must be found
	assert(i != _targetKeys.end());

	i->second.detachFromKeyValue(value);
	//value.detach(TargetingEntity::TargetChangedCaller(i->second));

	// Remove the found element
	_targetKeys.erase(i);

	targetsChanged();
}

} // namespace entity
