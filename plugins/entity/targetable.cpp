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

typedef std::map<std::string, targetables_t> targetnames_t;

targetnames_t g_targetnames;

targetables_t* getTargetables(const std::string& targetname)
{
  if (targetname.empty())
    return 0;
  return &g_targetnames[targetname];
}

ShaderPtr RenderableTargetingEntity::m_state;

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

	TargetingEntities::iterator i = _targetingEntities.insert(
		TargetingEntities::value_type(key, TargetingEntity())
	).first;
	
	value.attach(TargetingEntity::TargetChangedCaller(i->second));
	targetsChanged();
}

// Entity::Observer implementation, gets called on key erase
void TargetKeys::onKeyErase(const std::string& key, EntityKeyValue& value) {
	// ignore non-target keys
	if (!isTargetKey(key)) {
		return; 
	}

	TargetingEntities::iterator i = _targetingEntities.find(key);
	
	// This must be found
	assert(i != _targetingEntities.end());

	value.detach(TargetingEntity::TargetChangedCaller(i->second));

	// Remove the found element
	_targetingEntities.erase(i);

	targetsChanged();
}

const TargetingEntities& TargetKeys::get() const {
	return _targetingEntities;
}
