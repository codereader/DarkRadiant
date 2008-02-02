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
#include <boost/algorithm/string/case_conv.hpp>

class KeyObserverMap : 
	public Entity::Observer
{
	typedef std::multimap<std::string, KeyObserver> KeyObservers;
	KeyObservers _keyObservers;

public:
	void insert(const std::string& key, const KeyObserver& observer) {
		std::string lowercaseKey = boost::to_lower_copy(key);
		_keyObservers.insert(KeyObservers::value_type(lowercaseKey, observer));
	}

	void erase(const std::string& key, const KeyObserver& observer) {
		for (KeyObservers::iterator i = _keyObservers.begin(); i != _keyObservers.end(); ) {
			if (i->first == key && i->second == observer) {
				_keyObservers.erase(i++);
			}
			else {
				i++;
			}
		}
	}
	
	// Entity::Observer implementation, gets called on key insert
	void onKeyInsert(const std::string& key, EntityKeyValue& value) {
		std::string lowercaseKey = boost::to_lower_copy(key);
		for (KeyObservers::const_iterator i = _keyObservers.find(lowercaseKey); 
			 i != _keyObservers.end() && i->first == lowercaseKey; 
			 ++i)
		{
			value.attach(i->second);
		}
	}
	
	// Entity::Observer implementation, gets called on Key erase	
	void onKeyErase(const std::string& key, EntityKeyValue& value) {
		std::string lowercaseKey = boost::to_lower_copy(key);
		for (KeyObservers::const_iterator i = _keyObservers.find(lowercaseKey); 
			 i != _keyObservers.end() && i->first == lowercaseKey; 
			 ++i)
		{
			value.detach(i->second);
		}
	}
};

#endif
