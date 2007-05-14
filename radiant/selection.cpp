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

#include "selection.h"

#include "iselection.h"
#include "iscenegraph.h"
#include "igl.h"
#include "igrid.h"

#include "selection/RadiantSelectionSystem.h"

// greebo: Callback for the selectionSystem event: onBoundsChanged
void SelectionSystem_OnBoundsChanged() {
  GlobalSelectionSystem().pivotChanged();
}

SignalHandlerId SelectionSystem_boundsChanged;

// Module stuff

#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"

class SelectionDependencies :
  public GlobalSceneGraphModuleRef,
  public GlobalShaderCacheModuleRef,
  public GlobalOpenGLModuleRef,
  public GlobalGridModuleRef
{
};

/* greebo: This is the class that handles the selectionSystem instantiation
 * The constructor allocates a RadiantSelectionSystem instance on the heap
 * which is freed again by the destructor 
 */
class SelectionAPI
{
	RadiantSelectionSystem* _selectionSystem;
public:
	typedef SelectionSystem Type;
	STRING_CONSTANT(Name, "*");

	// Constructor
	SelectionAPI() {
		RadiantSelectionSystem::constructStatic();
		
		// allocate a new selection system instance on the heap 
		_selectionSystem = new RadiantSelectionSystem;
	
		SelectionSystem_boundsChanged = 
			GlobalSceneGraph().addBoundsChangedCallback(FreeCaller<SelectionSystem_OnBoundsChanged>());
	
		GlobalShaderCache().attachRenderable(*_selectionSystem);
	}
	
	~SelectionAPI() {
		GlobalShaderCache().detachRenderable(*_selectionSystem);
		GlobalSceneGraph().removeBoundsChangedCallback(SelectionSystem_boundsChanged);
		
		// release the selection system from memory
		delete _selectionSystem;
	
		RadiantSelectionSystem::destroyStatic();
	}
  
	SelectionSystem* getTable() {
		return _selectionSystem;
	}
};

typedef SingletonModule<SelectionAPI, SelectionDependencies> SelectionModule;
typedef Static<SelectionModule> StaticSelectionModule;
StaticRegisterModule staticRegisterSelection(StaticSelectionModule::instance());
