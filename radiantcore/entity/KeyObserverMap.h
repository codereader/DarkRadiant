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
#include <map>
#include <string>

#include "SpawnArgs.h"

namespace entity
{

/**
 * @brief Map of key observers associated with a particular entity.
 *
 * This is used internally by EntityNode to keep track of the KeyObserver
 * classes which are observing particular spawnargs.
 */
class KeyObserverMap :
	public Entity::Observer,
    public sigc::trackable
{
	// A map using case-insensitive comparison
    typedef std::multimap<std::string, KeyObserver::Ptr, string::ILess> KeyObservers;
    KeyObservers _keyObservers;

	// The observed entity
	SpawnArgs& _entity;

    void attachObserver(const std::string& key, KeyObserver& observer)
    {
		// Attach immediately if the entity already has such a (non-inherited) spawnarg
		if (EntityKeyValuePtr keyValue = _entity.getEntityKeyValue(key); keyValue) {
			keyValue->attach(observer);
		}

		// Call the observer right now with the current keyvalue as argument
		observer.onKeyValueChanged(_entity.getKeyValue(key));
    }

    void detachObserver(const std::string& key, KeyObserver& observer, bool sendEmptyValue)
    {
        if (EntityKeyValuePtr kvp = _entity.getEntityKeyValue(key); kvp)
            kvp->detach(observer, sendEmptyValue);
    }

public:
	KeyObserverMap(SpawnArgs& entity) :
		_entity(entity)
	{
		// Start observing the entity
		_entity.attachObserver(this);
	}

	~KeyObserverMap()
	{
        // Detach each individual KeyObserver from its EntityKeyValue, to avoid
        // dangling pointers if KeyObservers are destroyed.
        for (auto& [key, observer]: _keyObservers)
            detachObserver(key, *observer, false /* don't send final value change */);

        // All observers are detached, clear them out
        _keyObservers.clear();

        // Remove ourselves as an Entity::Observer (onKeyInsert and onKeyErase)
		_entity.detachObserver(this);
	}

    /**
     * @brief Add an observer function for the specified key.
     *
     * The observer function will be wrapped in a KeyObserver interface object
     * owned and deleted by the KeyObserverMap. This allows calling code to
     * attach a callback function without worrying about maintaining a
     * KeyObserver object.
     *
     * There is currently no way to manually delete an observer function added
     * in this way. All observer functions will be removed on destruction.
     *
     * @param key
     * Key to observe.
     *
     * @param func
     * Callback function to be invoked when the key value changes. It will also
     * be invoked immediately with the key's current value, or an empty string
     * if the key does not currently exist.
     */
    void observeKey(const std::string& key, KeyObserverFunc func)
    {
        auto iter = _keyObservers.insert(
            {key, std::make_shared<KeyObserverDelegate>(func)}
        );
        assert(iter != _keyObservers.end());
        attachObserver(key, *iter->second);
    }

    /**
     * @brief Add an observer object for the specified key.
     *
     * Multiple observers can be attached to the same key. It is the calling
     * code's responsibility to ensure that the KeyObserver exists for the
     * lifetime of the attachment; undefined behaviour will result if a
     * KeyObserver is destroyed while still attached to the entity.
     *
     * @param key
     * Key to observe.
     *
     * @param observer
     * Observer which will be invoked when the key value changes. The observer
     * will also be invoked immediately with the key's current value, which may
     * be an empty string if the key does not exist.
     */
	void insert(const std::string& key, KeyObserver& observer)
	{
        // Wrap observer in a shared_ptr with a NOP deleter, since we don't own it
		_keyObservers.insert({key, KeyObserver::Ptr(&observer, [](KeyObserver*) {})});

        attachObserver(key, observer);
	}

	void erase(const std::string& key, KeyObserver& observer)
	{
        const auto upperBound = _keyObservers.upper_bound(key);
		for (auto i = _keyObservers.find(key);
			 i != upperBound && i != _keyObservers.end();
			 /* in-loop increment */)
		{
			if (i->second.get() == &observer) {
                // Detach the observer from the actual keyvalue, if it exists
                detachObserver(key, observer, true /* send final empty value */);

                // Order is important: first increment, then erase the non-incremented value
				_keyObservers.erase(i++);
			}
			else {
				++i;
			}
		}
	}

	void refreshObservers()
	{
		for (KeyObservers::const_iterator i = _keyObservers.begin();
			i != _keyObservers.end(); ++i)
		{
			// Call the observer once again with the entity value
			i->second->onKeyValueChanged(_entity.getKeyValue(i->first));
		}
	}

	// Entity::Observer implementation, gets called on key insert
	void onKeyInsert(const std::string& key, EntityKeyValue& value)
	{
		for (KeyObservers::const_iterator i = _keyObservers.find(key);
			 i != _keyObservers.upper_bound(key) && i != _keyObservers.end();
			 ++i)
		{
			value.attach(*i->second);
		}
	}

	// Entity::Observer implementation, gets called on Key erase
	void onKeyErase(const std::string& key, EntityKeyValue& value)
	{
		for (KeyObservers::const_iterator i = _keyObservers.find(key);
			 i != _keyObservers.upper_bound(key) && i != _keyObservers.end();
			 ++i)
		{
			value.detach(*i->second);
		}
	}
};

} // namespace entity

#endif
