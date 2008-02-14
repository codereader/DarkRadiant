/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "targetable.h"
#include <boost/algorithm/string/predicate.hpp>

namespace entity {

TargetManager::TargetManager() :
	_emptyTarget(new Target)
{
	// Just to be sure, clear the target
	_emptyTarget->clear();
}

// Static accessor method
TargetManager& TargetManager::Instance() {
	static TargetManager _instance;
	return _instance;
}

TargetPtr TargetManager::getTarget(const std::string name) {
	if (name.empty()) {
		return _emptyTarget;
	}

	TargetList::iterator found = _targets.find(name);

	if (found != _targets.end()) {
		return found->second;
	}

	// Doesn't exist yet, create this one
	TargetPtr target(new Target);
	target->clear();

	// Insert into the local map and return
	_targets.insert(TargetList::value_type(name, target));

	return target;
}

void TargetManager::associateTarget(const std::string& name, scene::Instance* instance) {
	if (name.empty()) {
		return; // don't associate empty names
	}

	TargetList::iterator found = _targets.find(name);

	if (found != _targets.end()) {
		if (found->second->isEmpty()) {
			// Already registered, but empty => associate it
			found->second->setInstance(instance);
		}
		else {
			// Non-empty target!
			globalErrorStream() << "TargetManager: Target " << name.c_str() << " already associated!\n";
		}

		return;
	}

	// Doesn't exist yet, create a new target
	TargetPtr target(new Target);

	// Associate the target
	target->setInstance(instance);

	// Insert into the local map and return
	_targets.insert(TargetList::value_type(name, target));
}

void TargetManager::clearTarget(const std::string& name) {
	// Locate and clear the named target
	TargetList::iterator found = _targets.find(name);

	if (found != _targets.end()) {
		// Found, clear it
		found->second->clear();
	}
}

// --------------------------------------------------------------------

const TargetPtr& TargetKey::getTarget() const {
	return _target;
}

// Observes the given keyvalue
void TargetKey::attachToKeyValue(EntityKeyValue& value) {
	// Observe this entity keyvalue
	value.attach(TargetChangedCaller(*this));
}

// Stops observing the given keyvalue
void TargetKey::detachFromKeyValue(EntityKeyValue& value) {
	value.detach(TargetChangedCaller(*this));
}

// This gets called as soon as the "target" key in the spawnargs changes
void TargetKey::targetChanged(const std::string& target) {
	// Acquire the Target object (will be created if nonexistent)
	_target = TargetManager::Instance().getTarget(target);
}

} // namespace entity

// This maps names to Targetable pointers
typedef std::map<std::string, Targetable*> TargetableMap;
TargetableMap g_targetables;

Targetable* getTargetable(const std::string& targetname) {
	if (targetname.empty()) {
		return NULL;
	}

	TargetableMap::iterator i = g_targetables.find(targetname);
	if (i == g_targetables.end()) {
		return NULL;
	}
	return i->second;
}

//ShaderPtr RenderableTargetLines::m_state;

// TargetKeys implementation
bool TargetKeys::isTargetKey(const std::string& key) {
	// A key is a target key if it starts with "target" (any case)
	return (boost::algorithm::istarts_with(key, "target"));
}

void TargetKeys::setTargetsChanged(const Callback& targetsChanged) {
	_targetsChanged = targetsChanged;
}

void TargetKeys::targetsChanged() {
	_targetsChanged();
}

// Entity::Observer implementation, gets called on key insert
void TargetKeys::onKeyInsert(const std::string& key, EntityKeyValue& value) {
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
void TargetKeys::onKeyErase(const std::string& key, EntityKeyValue& value) {
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

/*const TargetKeys::TargetingEntities& TargetKeys::get() const {
	return _targetingEntities;
}*/
