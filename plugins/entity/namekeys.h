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

#if !defined(INCLUDED_NAMEKEYS_H)
#define INCLUDED_NAMEKEYS_H

#include "ientity.h"
#include "inamespace.h"

#include <map>
#include "Doom3Entity.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace entity {

class NameKeys : 
	public Entity::Observer, 
	public Namespaced
{
	INamespace* m_namespace;
	
	// The attached entity
	entity::Doom3Entity& m_entity;
	
	// The function pointer used to check the key for "name" keys
	typedef bool (*KeyIsNameFunc)(const std::string& key);
	KeyIsNameFunc m_keyIsName;
	
	// Not copy-constructible, not assignable
	NameKeys(const NameKeys& other);
	NameKeys& operator=(const NameKeys& other);

	// All the observed key values of the entity
	typedef std::map<std::string, EntityKeyValue*> KeyValues;
	KeyValues m_keyValues;

	/** greebo: This checks the key for known keynames and attaches
	 * 			the matching keys to the known Namespace.
	 */
	void attachKeyToNamespace(const std::string& key, EntityKeyValue& value) {
		// Only attach the name
		if (m_namespace != NULL && m_keyIsName(key)) {
			//globalOutputStream() << "insert " << key << "\n";
			m_namespace->attach(KeyValue::AssignCaller(value), KeyValue::AttachCaller(value));
		}
	}
	
	/** greebo: Detaches the key from the namespace, if it matches
	 * 			the "name key" convention ("name", "target", "bind")
	 */
	void detachKeyFromNamespace(const std::string& key, EntityKeyValue& value) {
		// Only detach "name keys"
		if (m_namespace != NULL && m_keyIsName(key)) {
			//globalOutputStream() << "erase " << key << "\n";
			m_namespace->detach(KeyValue::AssignCaller(value), KeyValue::DetachCaller(value));
		}
	}
	
	void attachAllKeys() {
		for (KeyValues::iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
			attachKeyToNamespace(i->first, *i->second);
		}
	}
	
	void detachAllKeys() {
		for (KeyValues::iterator i = m_keyValues.begin(); i != m_keyValues.end(); ++i) {
			detachKeyFromNamespace(i->first, *i->second);
		}
	}
public:
	NameKeys(entity::Doom3Entity& entity) :
		m_namespace(NULL), 
		m_entity(entity), 
		m_keyIsName(keyIsNameDoom3)
	{
		// Attach <self> to the observed entity
		m_entity.attach(*this);
	}

	~NameKeys() {
		// Detach <self> from the observed Entity
		m_entity.detach(*this);
	}

	/** greebo: Sets a new Namespace
	 */
	void setNamespace(INamespace& space) {
		// Detach all keys from the old namespace first
		detachAllKeys();
		
		// Set the new namespace
		m_namespace = &space;
		
		// Now attach the relevant keys again
		attachAllKeys();
	}
	
	/** greebo: Sets a new key-classification function
	 */
	void setKeyIsName(KeyIsNameFunc keyIsName) {
		// First, detach all relevant keys from the namespace
		detachAllKeys();
		
		// Set the new rule
		m_keyIsName = keyIsName;
		
		// Then re-attach all relevant keys matching the new convention
		attachAllKeys();
	}

	/** greebo: This gets called as soon as a new entity key/value gets added
	 * 			to the attached entity.
	 * 
	 * 			The routine saves all keyvalues and attaches the relevant
	 * 			"name keys" to the Namespace.
	 * 
	 *  Note: Entity::Observer implementation
	 */
	void onKeyInsert(const std::string& key, EntityKeyValue& value) {
		// Save the key/value pair locally
		m_keyValues.insert(KeyValues::value_type(key, &value));
		
		// Check if this key is a "name key"
		attachKeyToNamespace(key, value);
	}
	
	/** greebo: Gets called by the observed Entity when a value is erased from
	 * 			the list of spawnargs.
	 * 
	 *  Note: Entity::Observer implementation
	 */
	void onKeyErase(const std::string& key, EntityKeyValue& value) {
		// Detach the relevant keys from the Namespace
		detachKeyFromNamespace(key, value);
		
		// Remove it from the local map
		m_keyValues.erase(key);
	}
	
	/** greebo: This should return TRUE for "target", "targetNNN", "name" and "bind"
	 */
	static bool keyIsNameDoom3(const std::string& key) {
		return key == "target" || 
				(key.substr(0,6) == "target" && 
					boost::algorithm::all(key.substr(6), boost::algorithm::is_digit())) || 
				key == "name" || key == "bind";
	}
	
	static bool keyIsNameDoom3Doom3Group(const std::string& key) {
		return keyIsNameDoom3(key) || key == "model";
	}
};

} // namespace entity

#endif
