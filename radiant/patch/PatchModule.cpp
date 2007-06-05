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

#include "iradiant.h"
#include "ipatch.h"
#include "ifilter.h"
#include "ipreferencesystem.h"
#include "patchmanip.h"

#include "PatchNode.h"
#include "PatchSceneWalk.h"
#include "PatchCreators.h"
#include "PatchInstance.h"

/* greebo: The two Doom3 Patch Modules are defined here. As Doom 3 maps distinct between two different
 * types of Patches (patchDef2 and patchDef3), there are two APIs available for use in other modules.
 * 
 * See plugins/mapdoom3/mapdoom3.cpp for an example.
 */

namespace {
	// A global to keep track of the number of patch modules registered
	std::size_t g_patchModuleCount = 0;
	const std::string RKEY_PATCH_SUBDIVIDE_THRESHOLD = "user/ui/patch/subdivideThreshold";
}

// Initialises the Patch classes and registers the preferences
void Patch_Construct(EPatchType type) {
	// Check if we already registered a patch module
	if (++g_patchModuleCount != 1) {
		return;
	}

	// Construct and Register the patch-related preferences
	PreferencesPagePtr page = GlobalPreferenceSystem().getPage("Settings/Patch");
	page->appendEntry("Patch Subdivide Threshold", RKEY_PATCH_SUBDIVIDE_THRESHOLD);

	// Initialise the static member variables of the Patch and PatchInstance classes
	Patch::constructStatic(type);
	PatchInstance::constructStatic();
}

// De-Initialises the Patch classes (e.g. release the statics)
void Patch_Destroy() {
	if (--g_patchModuleCount != 0) {
		return;
	}

	// Release the static member variables of the classes Patch and PatchInstance 
	Patch::destroyStatic();
	PatchInstance::destroyStatic();
}

// ------------------------------------------------------------------------------------------------

// These are the "host" functions containing the static instances of the patch creators.
PatchCreator& GetDoom3PatchCreator() {
	static Doom3PatchCreator _doom3PatchCreator; 
	return _doom3PatchCreator;
}

PatchCreator& GetDoom3PatchDef2Creator() {
	static Doom3PatchDef2Creator _doom3PatchDef2Creator;
	return _doom3PatchDef2Creator;
}

// ------------------------------------------------------------------------------------------------

#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"

class PatchDependencies :
	public GlobalRadiantModuleRef,
	public GlobalSceneGraphModuleRef,
	public GlobalShaderCacheModuleRef,
	public GlobalSelectionModuleRef,
	public GlobalOpenGLModuleRef,
	public GlobalUndoModuleRef,
	public GlobalFilterModuleRef
{
};

class PatchDoom3API
{
	PatchCreator* m_patchdoom3;
public:
	typedef PatchCreator Type;
	STRING_CONSTANT(Name, "doom3");

	// Constructor
	PatchDoom3API() {
		Patch_Construct(ePatchTypeDoom3);
		m_patchdoom3 = &GetDoom3PatchCreator();
	}
	
	// Destructor
	~PatchDoom3API() {
		Patch_Destroy();
	}
	
	PatchCreator* getTable() {
		return m_patchdoom3;
	}
}; // class PatchDoom3API

typedef SingletonModule<PatchDoom3API, PatchDependencies> PatchDoom3Module;
typedef Static<PatchDoom3Module> StaticPatchDoom3Module;
StaticRegisterModule staticRegisterPatchDoom3(StaticPatchDoom3Module::instance());

class PatchDef2Doom3API
{
	PatchCreator* m_patchdef2doom3;
public:
	typedef PatchCreator Type;
	STRING_CONSTANT(Name, "def2doom3");

	// Constructor
	PatchDef2Doom3API() {
		Patch_Construct(ePatchTypeDoom3);
		// Retrieve a reference to the static instance from the "host" function above.
		m_patchdef2doom3 = &GetDoom3PatchDef2Creator();
		g_patchCreator = m_patchdef2doom3;
	}
	
	// destructor
	~PatchDef2Doom3API() {
		Patch_Destroy();
	}

	PatchCreator* getTable() {
		return m_patchdef2doom3;
	}
}; // class PatchDef2Doom3API

// Register this module with the above API
typedef SingletonModule<PatchDef2Doom3API, PatchDependencies> PatchDef2Doom3Module;
typedef Static<PatchDef2Doom3Module> StaticPatchDef2Doom3Module;
StaticRegisterModule staticRegisterPatchDef2Doom3(StaticPatchDef2Doom3Module::instance());
