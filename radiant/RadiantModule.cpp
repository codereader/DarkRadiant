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
#include "ifilesystem.h"
#include "ieclass.h"
#include "ipreferencesystem.h"
#include "ieventmanager.h"

#include "entity.h"
#include "map.h"
#include "select.h"
#include "map/AutoSaver.h"
#include "map/PointFile.h"
#include "camera/GlobalCamera.h"
#include "xyview/GlobalXYWnd.h"
#include "ui/texturebrowser/TextureBrowser.h"

#include "modulesystem/StaticModule.h"

namespace radiant {

RadiantModule::RadiantModule() :
	_mainWindow(NULL)
{}
	
GtkWindow* RadiantModule::getMainWindow() {
	return _mainWindow;
}

void RadiantModule::setMainWindow(GtkWindow* mainWindow) {
	_mainWindow = mainWindow;
}
	
GdkPixbuf* RadiantModule::getLocalPixbuf(const std::string& fileName) {
	// Try to use a cached pixbuf first
	PixBufMap::iterator i = _localPixBufs.find(fileName);
	
	if (i != _localPixBufs.end()) {
		return i->second;
	}

	// Not cached yet, load afresh

	// Construct the full filename using the Bitmaps path
	std::string fullFileName(GlobalRegistry().get(RKEY_BITMAPS_PATH) + fileName);

	GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(fullFileName.c_str(), NULL);

	_localPixBufs.insert(PixBufMap::value_type(fileName, pixbuf));

	return pixbuf;
}

GdkPixbuf* RadiantModule::getLocalPixbufWithMask(const std::string& fileName) {

	// Try to find a cached pixbuf before loading from disk
	PixBufMap::iterator i = _localPixBufsWithMask.find(fileName);
	
	if (i != _localPixBufsWithMask.end()) {
		return i->second;
	}

	// Not cached yet, load afresh

	std::string fullFileName(GlobalRegistry().get(RKEY_BITMAPS_PATH) + fileName);
	
	GdkPixbuf* rgb = gdk_pixbuf_new_from_file(fullFileName.c_str(), 0);
	if (rgb != NULL) {
		// File load successful, add alpha channel
		GdkPixbuf* rgba = gdk_pixbuf_add_alpha(rgb, TRUE, 255, 0, 255);
		gdk_pixbuf_unref(rgb);

		_localPixBufsWithMask.insert(PixBufMap::value_type(fileName, rgba));

		return rgba;
	}
	else {
		// File load failed
		return NULL;
	}
}

ICounter& RadiantModule::getCounter(CounterType counter) {
	// Pass the call to the helper class
	return _counters.get(counter);
}
	
void RadiantModule::setStatusText(const std::string& statusText) {
	Sys_Status(statusText);
}
	
void RadiantModule::updateAllWindows() {
	UpdateAllWindows();
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
		_dependencies.insert(MODULE_FILETYPES);
		_dependencies.insert(MODULE_SCENEGRAPH);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_ECLASSMANAGER);
		_dependencies.insert(MODULE_SELECTIONSYSTEM);
		_dependencies.insert(MODULE_SHADERCACHE);
	}
	
	return _dependencies;
}

void RadiantModule::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "RadiantModule::initialiseModule called.\n";
	
	// Reset the node id count
  	scene::Node::resetIds();
  	
    GlobalFiletypes().addType(
    	"sound", "wav", FileTypePattern("PCM sound files", "*.wav"));

    Selection_construct();
    map::PointFile::Instance().registerCommands();
    Map_Construct();
    MainFrame_Construct();
    GlobalCamera().construct();
    GlobalXYWnd().construct();
    GlobalTextureBrowser().construct();
    Entity_Construct();
    map::AutoSaver().init();
}

void RadiantModule::shutdownModule() {
	globalOutputStream() << "RadiantModule::shutdownModule called.\n";
	
	GlobalFileSystem().shutdown();

	map::PointFile::Instance().destroy();
    Entity_Destroy();
    Selection_destroy();
    
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

