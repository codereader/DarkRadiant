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
#include "RadiantModule.h"

#include <iostream>

#include "ifiletypes.h"
#include "iregistry.h"
#include "icommandsystem.h"
#include "itextstream.h"
#include "ifilesystem.h"
#include "iuimanager.h"
#include "ieclass.h"
#include "ipreferencesystem.h"
#include "ieventmanager.h"
#include "iclipper.h"

#include "scene/Node.h"

#include "entity.h"
#include "map/AutoSaver.h"
#include "map/PointFile.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "ui/mediabrowser/MediaBrowser.h"
#include "ui/overlay/OverlayDialog.h"
#include "gtkutil/FileChooser.h"

#include "modulesystem/StaticModule.h"

#include "mainframe_old.h"

namespace radiant
{

sigc::signal<void> RadiantModule::signal_radiantStarted() const
{
    return _radiantStarted;
}

sigc::signal<void> RadiantModule::signal_radiantShutdown() const
{
    return _radiantShutdown;
}

void RadiantModule::broadcastShutdownEvent()
{
    _radiantShutdown.emit();
    _radiantShutdown.clear();
}

// Broadcasts a "startup" event to all the listeners
void RadiantModule::broadcastStartupEvent()
{
    _radiantStarted.emit();
}

// RegisterableModule implementation
const std::string& RadiantModule::getName() const
{
	static std::string _name(MODULE_RADIANT);
	return _name;
}

const StringSet& RadiantModule::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
    {
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_SELECTIONSYSTEM);
		_dependencies.insert(MODULE_RENDERSYSTEM);
		_dependencies.insert(MODULE_CLIPPER);
	}

	return _dependencies;
}

void RadiantModule::initialiseModule(const ApplicationContext& ctx)
{
	globalOutputStream() << "RadiantModule::initialiseModule called." << std::endl;

	// Reset the node id count
  	scene::Node::resetIds();

    map::PointFile::Instance().registerCommands();
    MainFrame_Construct();
	ui::MediaBrowser::registerPreferences();
	ui::TextureBrowser::construct();
	entity::registerCommands();
    map::AutoSaver().init();
}

void RadiantModule::shutdownModule()
{
	globalOutputStream() << "RadiantModule::shutdownModule called." << std::endl;

	GlobalFileSystem().shutdown();

	map::PointFile::Instance().destroy();
	ui::OverlayDialog::destroy();
	ui::TextureBrowser::destroy();

    _radiantShutdown.clear();
}

// Define the static Radiant module
module::StaticModule<RadiantModule> radiantCoreModule;

// Return the static Radiant module to other code within the main binary
RadiantModulePtr getGlobalRadiant()
{
	return radiantCoreModule.getModule();
}

} // namespace radiant

