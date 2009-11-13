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

#if !defined(INCLUDED_BRUSHMODULE_H)
#define INCLUDED_BRUSHMODULE_H

#include "iregistry.h"
#include "imodule.h"

#include "brush/TexDef.h"
#include "ibrush.h"

class BrushModuleClass : 
	public RegistryKeyObserver,
	public BrushCreator
{
	
	bool _textureLockEnabled;
	
public:
    // destructor
	virtual ~BrushModuleClass() {}

	// This constructs the brush preferences, initialises static variables, etc.
	void construct();
	
	// The opposite of the above construct()
	void destroy();
	
	// Adds the preference settings to the prefdialog
	void constructPreferences();
	
	// --------------- BrushCreator methods ---------------------------------------------
	
	// Creates a new brush node on the heap and returns it 
	scene::INodePtr createBrush();
	
	// ----------------------------------------------------------------------------------
	
	// Re-constructs the BrushClipPlane to activate the colour change
	void clipperColourChanged();
	
	// returns true if the texture lock is enabled
	bool textureLockEnabled() const;
	void setTextureLock(bool enabled);
	
	// Switches the texture lock on/off
	void toggleTextureLock();
	
	// The callback for registry key changes
	void keyChanged(const std::string& key, const std::string& val);
	
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
	virtual void shutdownModule();
	
}; // class BrushModuleClass

// The accessor function declaration
BrushModuleClass* GlobalBrush();

#endif
