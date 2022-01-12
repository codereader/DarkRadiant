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
#include <sigc++/connection.h>

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
	// A map using case-insensitive comparison, storing one or more KeyObserver
	// objects for each observed key.
    typedef std::multimap<std::string, KeyObserver::Ptr, string::ILess> KeyObservers;
    KeyObservers _keyObservers;

    // Signals for each key observed with observeKey(). This is a map, not a
    // multimap, since each signal can be connected to an arbitrary number of
    // slots.
    using KeySignal = sigc::signal<void, std::string>;
    using KeySignals = std::map<std::string, KeySignal, string::ILess>;
    KeySignals _keySignals;

    // Keep track of connections for each external observer, so we can
    // disconnect them if erase() is called.
    std::multimap<KeyObserver*, sigc::connection> _connectionsByObserver;

	// The observed entity
	SpawnArgs& _entity;

    void attachObserver(const std::string& key, KeyObserver& observer)
    {
        if (EntityKeyValuePtr keyValue = _entity.getEntityKeyValue(key); keyValue) {
            // Attach immediately if the entity already has such a (non-inherited)
            // spawnarg. This will automatically send the current value.
            keyValue->attach(observer);
        }
        else {
            // No current value, call the observer with the inherited value or
            // the empty string.
            observer.onKeyValueChanged(_entity.getKeyValue(key));
        }
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

        // Clear out and destroy all signals
        _keySignals.clear();

        // Remove ourselves as an Entity::Observer (onKeyInsert and onKeyErase)
		_entity.detachObserver(this);
	}

    /**
     * @brief Add an observer slot for the specified key.
     *
     * The slot will be connected to an internal signal which will be emitted
     * when the associated key changes. This enables the standard libsigc++
     * auto-disconnection if the slot is bound to a member function of a
     * sigc::trackable class using sigc::mem_fun. If a lambda is used, there
     * will be no auto-disconnection; in this case the calling code must ensure
     * that the lambda does not capture variables that may become invalid while
     * the signal is still connected.
     *
     * There is currently no way to manually disconnect an observer function
     * added in this way. All observer functions will be removed on destruction.
     *
     * @param key
     * Key to observe.
     *
     * @param func
     * Slot to be invoked when the key value changes. It will also be invoked
     * immediately with the key's current value, or an empty string if the key
     * does not currently exist.
     */
    sigc::connection observeKey(const std::string& key, KeyObserverFunc func)
    {
        // If there is already a signal for this key, just connect the slot to it
        sigc::connection conn;
        if (auto iter = _keySignals.find(key); iter != _keySignals.end()) {
            conn = iter->second.connect(func);

            // Send initial value to slot
            func(_entity.getKeyValue(key));
        }
        else {
            // No existing signal, so we need to create one
            conn = _keySignals[key].connect(func);

            // Create and attach an internal KeyObserver to respond to keyvalue
            // changes and emit the associated signal. Note that we don't just wrap
            // the slot in a delegate to invoke it directly — we need the
            // intervening sigc::signal to allow for auto-disconnection.
            auto delegate = std::make_shared<KeyObserverDelegate>(
                [=](const std::string& value) { _keySignals[key].emit(value); }
            );

            // Store the observer internally. We must only do this once per key;
            // multiple observers would result in multiple signal emissions.
            _keyObservers.insert({key, delegate});

            // Send initial value and attach to EntityKeyValue immediately if needed
            attachObserver(key, *delegate);
        }
        return conn;
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
