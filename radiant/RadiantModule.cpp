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
#include <gtk/gtkimage.h>

#include "ifiletypes.h"
#include "iregistry.h"
#include "icommandsystem.h"
#include "ifilesystem.h"
#include "iuimanager.h"
#include "ieclass.h"
#include "ipreferencesystem.h"
#include "ieventmanager.h"
#include "iclipper.h"

#include "entity.h"
#include "map/AutoSaver.h"
#include "map/PointFile.h"
#include "camera/GlobalCamera.h"
#include "xyview/GlobalXYWnd.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "ui/mediabrowser/MediaBrowser.h"
#include "ui/common/ModelPreview.h"
#include "gtkutil/FileChooser.h"

#include "modulesystem/StaticModule.h"

#include "mainframe_old.h"

namespace radiant
{

ui::IModelPreviewPtr RadiantModule::createModelPreview()
{
	return ui::IModelPreviewPtr(new ui::ModelPreview);
}

void RadiantModule::addEventListener(RadiantEventListenerPtr listener) {
	_eventListeners.insert(RadiantEventListenerWeakPtr(listener));
}
	
void RadiantModule::removeEventListener(RadiantEventListenerPtr listener) {
	EventListenerList::iterator found = _eventListeners.find(
		RadiantEventListenerWeakPtr(listener)
	);
	if (found != _eventListeners.end()) {
		_eventListeners.erase(found);
	}
}
	
// Broadcasts a "shutdown" event to all the listeners, this also clears all listeners!
void RadiantModule::broadcastShutdownEvent() {
	for (EventListenerList::iterator i = _eventListeners.begin();
	     i != _eventListeners.end(); /* in-loop increment */)
	{
		// Get the weak pointer and immediately increase the iterator
		// This should allow removal of listeners without running into invalid
		// iterators in the for() loop above.
		RadiantEventListenerWeakPtr weakPtr = *i++;

		// Try to lock the pointer
		RadiantEventListenerPtr listener = weakPtr.lock();

		if (listener != NULL)
		{
			listener->onRadiantShutdown();
		}
	}

	// This was the final radiant event, no need for keeping any pointers anymore
	_eventListeners.clear();
}

// Broadcasts a "startup" event to all the listeners
void RadiantModule::broadcastStartupEvent() {
	for (EventListenerList::iterator i = _eventListeners.begin();
	     i != _eventListeners.end(); /* in-loop increment */)
	{
		// Get the weak pointer and immediately increase the iterator
		// This should allow removal of listeners without running into invalid
		// iterators in the for() loop above.
		RadiantEventListenerWeakPtr weakPtr = *i++;

		// Try to lock the pointer
		RadiantEventListenerPtr listener = weakPtr.lock();

		if (listener != NULL)
		{
			listener->onRadiantStartup();
		}
	}
}

// RegisterableModule implementation
const std::string& RadiantModule::getName() const {
	static std::string _name(MODULE_RADIANT);
	return _name;
}

const StringSet& RadiantModule::getDependencies() const {
	static StringSet _dependencies;
	
	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_FILETYPES);
		_dependencies.insert(MODULE_SCENEGRAPH);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_ECLASSMANAGER);
		_dependencies.insert(MODULE_SELECTIONSYSTEM);
		_dependencies.insert(MODULE_RENDERSYSTEM);
		_dependencies.insert(MODULE_CLIPPER);
	}
	
	return _dependencies;
}

void RadiantModule::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "RadiantModule::initialiseModule called.\n";
	
	// Reset the node id count
  	scene::Node::resetIds();
  	
    GlobalFiletypes().addType(
    	"sound", "wav", FileTypePattern("PCM sound files", "*.wav"));

    map::PointFile::Instance().registerCommands();
    MainFrame_Construct();
    GlobalCamera().construct();
    GlobalXYWnd().construct();
	ui::MediaBrowser::registerPreferences();
    GlobalTextureBrowser().construct();
    Entity_Construct();
    map::AutoSaver().init();
}

void RadiantModule::shutdownModule() {
	globalOutputStream() << "RadiantModule::shutdownModule called.\n";
	
	GlobalFileSystem().shutdown();

	map::PointFile::Instance().destroy();
    Entity_Destroy();
    
    // Remove all the event listeners, otherwise the shared_ptrs 
    // lock the instances. This is just for safety, usually all
	// EventListeners get cleared upon OnRadiantShutdown anyway.
    _eventListeners.clear();
}

// Define the static Radiant module
module::StaticModule<RadiantModule> radiantCoreModule;

// Return the static Radiant module to other code within the main binary
RadiantModulePtr getGlobalRadiant() {
	return radiantCoreModule.getModule();
}

} // namespace radiant

