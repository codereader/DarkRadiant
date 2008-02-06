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
bool TargetKeys::readTargetKey(const char* key, std::size_t& index) {
	if(string_equal_n(key, "target", 6)) {
		index = 0;

		if (string_empty(key + 6) || string_parse_size(key + 6, index)) {
			return true;
		}
	}
	return false;
}

void TargetKeys::setTargetsChanged(const Callback& targetsChanged) {
	m_targetsChanged = targetsChanged;
}

void TargetKeys::targetsChanged() {
	m_targetsChanged();
}

// Entity::Observer implementation, gets called on key insert
void TargetKeys::onKeyInsert(const std::string& key, EntityKeyValue& value) {
	std::size_t index;
	if (readTargetKey(key.c_str(), index)) {
		TargetingEntities::iterator i = m_targetingEntities.insert(TargetingEntities::value_type(index, TargetingEntity())).first;
		value.attach(TargetingEntity::TargetChangedCaller((*i).second));
		targetsChanged();
	}
}

// Entity::Observer implementation, gets called on key erase
void TargetKeys::onKeyErase(const std::string& key, EntityKeyValue& value) {
	std::size_t index;
	if (readTargetKey(key.c_str(), index)) {
		TargetingEntities::iterator i = m_targetingEntities.find(index);
		value.detach(TargetingEntity::TargetChangedCaller((*i).second));
		m_targetingEntities.erase(i);
		targetsChanged();
	}
}

const TargetingEntities& TargetKeys::get() const {
	return m_targetingEntities;
}
