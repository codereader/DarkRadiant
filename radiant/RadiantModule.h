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

#pragma once

#include "iradiant.h"

#include <boost/scoped_ptr.hpp>

namespace radiant
{

class RadiantThreadManager;

/// IRadiant implementation class.
class RadiantModule :
	public IRadiant
{
    // Our signals
    sigc::signal<void> _radiantStarted;
    sigc::signal<void> _radiantShutdown;

    // Thread manager instance
    mutable boost::scoped_ptr<RadiantThreadManager> _threadManager;

public:

    /// Broadcast shutdwon signal and clear all listeners
	void broadcastShutdownEvent();

	/// Broadcasts startup signal
	void broadcastStartupEvent();

    // IRadiant implementation
    sigc::signal<void> signal_radiantStarted() const;
    sigc::signal<void> signal_radiantShutdown() const;
    const ThreadManager& getThreadManager() const;

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
