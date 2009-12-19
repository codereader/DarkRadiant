/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

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

/* greebo: This is where the interface for other plugins is defined.
 * Functions that should be accessible via GlobalRadiant() are defined here 
 * as function pointers. The class RadiantCoreAPI in plugin.cpp makes sure
 * that these variables are pointing to the correct functions. 
 */

#ifndef IRADIANT_H_
#define IRADIANT_H_

#include "imodule.h"
#include "imodelpreview.h"
#include <boost/weak_ptr.hpp>

// ========================================
// GTK+ helper functions

// NOTE: parent can be 0 in all functions but it's best to set them

// this API does not depend on gtk+ or glib
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkWindow GtkWindow;

enum CounterType {
	counterBrushes,
	counterPatches,
	counterEntities
};

/** greebo: An EventListener gets notified by the Radiant module
 *          on global events like shutdown, startup and such.
 * 
 *          EventListener classes must register themselves using
 *          the GlobalRadiant().addEventListener() method in order
 *          to get notified about the events.
 * 
 * Note: Default implementations are empty, deriving classes are
 *       supposed to pick the events they want to listen to.
 */
class RadiantEventListener {
public:
    /** Destructor
	 */
	virtual ~RadiantEventListener() {}

	/** This gets called AFTER the MainFrame window has been constructed.
	 */
	virtual void onRadiantStartup() {}
	
	/** Gets called when BEFORE the MainFrame window is destroyed.
	 *  Note: After this call, the EventListeners are deregistered from the
	 *        Radiant module, all the internally held shared_ptrs are cleared.
	 */
	virtual void onRadiantShutdown() {}
};
typedef boost::shared_ptr<RadiantEventListener> RadiantEventListenerPtr;
typedef boost::weak_ptr<RadiantEventListener> RadiantEventListenerWeakPtr;

class ICounter;

const std::string MODULE_RADIANT("Radiant");

/** greebo: This abstract class defines the interface to the core application.
 * 			Use this to access methods from the main codebase in radiant/
 */
class IRadiant :
	public RegisterableModule
{
public:
	// Returns the Counter object of the given type
	virtual ICounter& getCounter(CounterType counter) = 0;
	
	virtual void updateAllWindows() = 0;

	// Creates a new model preview (GL view with draggable viewpoint, zoom and filter functionality)
	virtual ui::IModelPreviewPtr createModelPreview() = 0;

	// Registers/de-registers an event listener class
	virtual void addEventListener(RadiantEventListenerPtr listener) = 0;
	virtual void removeEventListener(RadiantEventListenerPtr listener) = 0;
};

inline IRadiant& GlobalRadiant() {
	// Cache the reference locally
	static IRadiant& _radiant(
		*boost::static_pointer_cast<IRadiant>(
			module::GlobalModuleRegistry().getModule(MODULE_RADIANT)
		)
	);
	return _radiant;
}

#endif
