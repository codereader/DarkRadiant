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

#if !defined(INCLUDED_PLUGIN_H)
#define INCLUDED_PLUGIN_H

#include "map/CounterManager.h"
#include "iradiant.h"

/**
 * IRadiant implementation class.
 */
class RadiantCoreAPI :
	public IRadiant
{
	map::CounterManager _counters;
	
	typedef std::set<RadiantEventListenerPtr> EventListenerList;
	EventListenerList _eventListeners;

public:
	RadiantCoreAPI();
	
	virtual GtkWindow* getMainWindow();
	
	virtual GdkPixbuf* getLocalPixbuf(const std::string& fileName);
	
	virtual GdkPixbuf* getLocalPixbufWithMask(const std::string& fileName);
	
	virtual ICounter& getCounter(CounterType counter);
	
	virtual void setStatusText(const std::string& statusText);
	
	virtual void updateAllWindows();
	
	virtual void addEventListener(RadiantEventListenerPtr listener);
	
	virtual void removeEventListener(RadiantEventListenerPtr listener);
	
	// Broadcasts a "shutdown" event to all the listeners, this also clears all listeners!
	void broadcastShutdownEvent();
	
	// Broadcasts a "startup" event to all the listeners
	void broadcastStartupEvent();
	
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	
	virtual const StringSet& getDependencies() const;
	
	virtual void initialiseModule(const ApplicationContext& ctx);
	
	virtual void shutdownModule();
};

namespace radiant {
	
	/**
	 * Return the global Radiant module (for use internally, not by other 
	 * modules).
	 */
	boost::shared_ptr<RadiantCoreAPI> getGlobalRadiant();

}

#endif
