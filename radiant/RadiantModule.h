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

#include "iradiant.h"

namespace radiant {

/**
 * IRadiant implementation class.
 */
class RadiantModule :
	public IRadiant
{
	typedef std::set<RadiantEventListenerWeakPtr> EventListenerList;
	EventListenerList _eventListeners;

public:
	ui::IModelPreviewPtr createModelPreview();
	
	void addEventListener(RadiantEventListenerPtr listener);
	void removeEventListener(RadiantEventListenerPtr listener);
	
	// Broadcasts a "shutdown" event to all the listeners, this also clears all listeners!
	void broadcastShutdownEvent();
	
	// Broadcasts a "startup" event to all the listeners
	void broadcastStartupEvent();
	
	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();
};
typedef boost::shared_ptr<RadiantModule> RadiantModulePtr;

/**
 * Return the global Radiant module (for use internally, not by other 
 * modules).
 */
RadiantModulePtr getGlobalRadiant();

} // namespace radiant

#endif
