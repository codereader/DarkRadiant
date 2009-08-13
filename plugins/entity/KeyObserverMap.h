#ifndef _KEY_OBSERVER_MAP_H_
#define _KEY_OBSERVER_MAP_H_

#include "ientity.h"
#include "generic/callback.h"
#include <map>
#include <string>
#include <boost/algorithm/string/case_conv.hpp>

namespace entity
{

class KeyObserverMap : 
	public Entity::Observer
{
	typedef std::multimap<std::string, KeyObserver> KeyObservers;
	KeyObservers _keyObservers;

	// The spawnargs we're attached to
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
		// Stop observing the entity
		_entity.detachObserver(this);
	}

	/**
	 * Register a named key for observation. As soon as the key
	 * is found on the entity, the given observer will be attached to the KeyValue.
	 */
	void insert(const std::string& key, const KeyObserver& observer)
	{
		std::string lowercaseKey = boost::to_lower_copy(key);

		// Remember this (lower-case) key, if the key is added to the entity later on
		_keyObservers.insert(KeyObservers::value_type(lowercaseKey, observer));

		// Check if the entity already has that key
		EntityKeyValuePtr keyValue = _entity.findKeyValue(key);

		if (keyValue != NULL)
		{
			keyValue->attach(observer);
		}
	}

	// Unregister a named key from observation
	void erase(const std::string& key, const KeyObserver& observer)
	{
		std::string lowercaseKey = boost::to_lower_copy(key);

		for (KeyObservers::iterator i = _keyObservers.begin(); i != _keyObservers.end(); )
		{
			if (i->first == lowercaseKey && i->second == observer)
			{
				_keyObservers.erase(i++);

				EntityKeyValuePtr keyValue = _entity.findKeyValue(key);

				if (keyValue != NULL)
				{
					keyValue->detach(observer);
				}
			}
			else
			{
				i++;
			}
		}
	}
	
	// Entity::Observer implementation, gets called on key insert
	void onKeyInsert(const std::string& key, EntityKeyValue& value)
	{
		// Let's see if one of the registered keys matches the one 
		// which just got inserted into the entity
		std::string lowercaseKey = boost::to_lower_copy(key);

		for (KeyObservers::const_iterator i = _keyObservers.find(lowercaseKey); 
			 i != _keyObservers.end() && i->first == lowercaseKey; 
			 ++i)
		{
			value.attach(i->second);
		}
	}
	
	// Entity::Observer implementation, gets called on Key erase	
	void onKeyErase(const std::string& key, EntityKeyValue& value)
	{
		// Check if the deleted key is matching on of "ours"
		std::string lowercaseKey = boost::to_lower_copy(key);

		for (KeyObservers::const_iterator i = _keyObservers.find(lowercaseKey); 
			 i != _keyObservers.end() && i->first == lowercaseKey; 
			 ++i)
		{
			value.detach(i->second);
		}
	}
};

} // namespace entity

#endif /* _KEY_OBSERVER_MAP_H_ */
