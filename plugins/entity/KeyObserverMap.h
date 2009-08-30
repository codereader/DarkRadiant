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

#if !defined(INCLUDED_KEYOBSERVERS_H)
#define INCLUDED_KEYOBSERVERS_H

#include "ientity.h"
#include "generic/callback.h"
#include <map>
#include <string>
#include <boost/algorithm/string/predicate.hpp>

namespace entity
{

/**
 * Comaparator to allow for case-insensitive search in std::multimap
 */
struct CaseInsensitiveKeyCompare : 
	public std::binary_function<std::string, std::string, bool>
{
	bool operator()(const std::string &s1, const std::string &s2) const
	{
		return boost::algorithm::ilexicographical_compare(s1, s2);
	}
};

class KeyObserverMap : 
	public Entity::Observer
{
	// A map using case-insensitive comparison
	typedef std::multimap<std::string, KeyObserver, CaseInsensitiveKeyCompare> KeyObservers;
	KeyObservers _keyObservers;

	// The observed entity
	Doom3Entity& _entity;

public:
	KeyObserverMap(Doom3Entity& entity) :
		_entity(entity)
	{
		// Start observing the entity
		_entity.attachObserver(this);
	}

	~KeyObserverMap()
	{
		_entity.detachObserver(this);
	}

	/** 
	 * greebo: This registers a key for observation. As soon as the key gets inserted in the
	 * entity's spawnargs, the given observer is attached to the entity's keyvalue.
	 */
	void insert(const std::string& key, const KeyObserver& observer)
	{
		_keyObservers.insert(KeyObservers::value_type(key, observer));

		// Call the observer right now with the current keyvalue as argument
		observer(_entity.getKeyValue(key));
	}

	void erase(const std::string& key, const KeyObserver& observer)
	{
		for (KeyObservers::iterator i = _keyObservers.find(key); 
			 i != _keyObservers.upper_bound(key) && i != _keyObservers.end(); 
			 /* in-loop increment */)
		{
			if (i->second == observer)
			{
				_keyObservers.erase(i++);
			}
			else
			{
				++i;
			}
		}
	}
	
	// Entity::Observer implementation, gets called on key insert
	void onKeyInsert(const std::string& key, EntityKeyValue& value)
	{
		for (KeyObservers::const_iterator i = _keyObservers.find(key); 
			 i != _keyObservers.upper_bound(key) && i != _keyObservers.end(); 
			 ++i)
		{
			value.attach(i->second);
		}
	}
	
	// Entity::Observer implementation, gets called on Key erase	
	void onKeyErase(const std::string& key, EntityKeyValue& value)
	{
		for (KeyObservers::const_iterator i = _keyObservers.find(key); 
			 i != _keyObservers.upper_bound(key) && i != _keyObservers.end(); 
			 ++i)
		{
			value.detach(i->second);
		}
	}
};

} // namespace entity

#endif
