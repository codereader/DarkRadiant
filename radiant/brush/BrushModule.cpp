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

#include "BrushModule.h"

#include "iradiant.h"

#include "ifilter.h"
#include "brush/BrushNode.h"
#include "brush/BrushInstance.h"
#include "brush/BrushClipPlane.h"
#include "brush/BrushVisit.h"
#include "brushmanip.h"

#include "preferencesystem.h"
#include "stringio.h"

#include "map.h"
#include "qe3.h"
#include "mainframe.h"
#include "preferences.h"


// ---------------------------------------------------------------------------------------

// Constructor, connect self to the observed registryKey
BrushModuleClass::BrushModuleClass() :
	_textureLockEnabled(GlobalRegistry().get(RKEY_ENABLE_TEXTURE_LOCK) == "1")
{
	GlobalRegistry().addKeyObserver(this, RKEY_ENABLE_TEXTURE_LOCK);
	
	// add the preference settings
	constructPreferences();
}

void BrushModuleClass::constructPreferences() {
	// Add a page to the given group
	PreferencesPagePtr page = GlobalPreferenceSystem().getPage("Settings/Primitives");
	
	// Add the default texture scale preference and connect it to the according registryKey
	// Note: this should be moved somewhere else, I think
	page->appendEntry("Default texture scale", "user/ui/textures/defaultTextureScale");

	// The checkbox to enable/disable the texture lock option
	page->appendCheckBox("", "Enable Texture Lock (for Brushes)", "user/ui/brush/textureLock");
}

void BrushModuleClass::construct() {
	Brush_registerCommands();
	
	BrushClipPlane::constructStatic();
	BrushInstance::constructStatic();
	Brush::constructStatic();
	
	Brush::m_maxWorldCoord = GlobalRegistry().getFloat("game/defaults/maxWorldCoord");
	BrushInstance::m_counter = &GlobalRadiant().getCounter(counterBrushes);
}

void BrushModuleClass::destroy() {
	Brush::m_maxWorldCoord = 0;
	BrushInstance::m_counter = 0;
	
	Brush::destroyStatic();
	BrushInstance::destroyStatic();
	BrushClipPlane::destroyStatic();
}

void BrushModuleClass::clipperColourChanged() {
	BrushClipPlane::destroyStatic();
	BrushClipPlane::constructStatic();
}

void BrushModuleClass::keyChanged() {
	_textureLockEnabled = (GlobalRegistry().get(RKEY_ENABLE_TEXTURE_LOCK) == "1");
}

bool BrushModuleClass::textureLockEnabled() const {
	return _textureLockEnabled;
}

void BrushModuleClass::setTextureLock(bool enabled) {
	// Write the value to the registry, the keyChanged() method is triggered automatically
	GlobalRegistry().set(RKEY_ENABLE_TEXTURE_LOCK, enabled ? "1" : "0");
}

void BrushModuleClass::toggleTextureLock() {
	setTextureLock(!textureLockEnabled());
	
	if (g_pParentWnd != 0) {
		g_pParentWnd->SetGridStatus();
	}
}

// ------------ BrushCreator implementation --------------------------------------------

void BrushFaceData_fromFace(const BrushFaceDataCallback& callback, Face& face) {
	  _QERFaceData faceData;
	  faceData.m_p0 = face.getPlane().planePoints()[0];
	  faceData.m_p1 = face.getPlane().planePoints()[1];
	  faceData.m_p2 = face.getPlane().planePoints()[2];
	  faceData.m_shader = face.GetShader();
	  faceData.m_texdef = face.getTexdef().m_projection.m_texdef;
	  faceData.contents = face.getShader().m_flags.m_contentFlags;
	  faceData.flags = face.getShader().m_flags.m_surfaceFlags;
	  faceData.value = face.getShader().m_flags.m_value;
	  callback(faceData);
}
typedef ConstReferenceCaller1<BrushFaceDataCallback, Face&, BrushFaceData_fromFace> BrushFaceDataFromFaceCaller;
typedef Callback1<Face&> FaceCallback;

scene::INodePtr BrushModuleClass::createBrush() {
	return scene::INodePtr(new BrushNode);
}

void BrushModuleClass::Brush_forEachFace(scene::INodePtr brush, const BrushFaceDataCallback& callback) {
	::Brush_forEachFace(*Node_getBrush(brush), FaceCallback(BrushFaceDataFromFaceCaller(callback)));
}

// Adds a face plan to the given brush
bool BrushModuleClass::Brush_addFace(scene::INodePtr brush, const _QERFaceData& faceData) {
	Node_getBrush(brush)->undoSave();
	return Node_getBrush(brush)->addPlane(faceData.m_p0, faceData.m_p1, faceData.m_p2, faceData.m_shader, TextureProjection(faceData.m_texdef, BrushPrimitTexDef(), Vector3(0, 0, 0), Vector3(0, 0, 0))) != 0;
}

// -------------------------------------------------------------------------------------

// greebo: The accessor function for the brush module containing the static instance
BrushModuleClass* GlobalBrush() {
	static BrushModuleClass _brushModule;
	return &_brushModule;
}

// ---------------------------------------------------------------------------------------

#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"

class BrushDependencies :
	public GlobalRadiantModuleRef,
	public GlobalRegistryModuleRef,
	public GlobalPreferenceSystemModuleRef,
	public GlobalSceneGraphModuleRef,
	public GlobalShaderCacheModuleRef,
	public GlobalSelectionModuleRef,
	public GlobalOpenGLModuleRef,
	public GlobalUndoModuleRef,
	public GlobalFilterModuleRef 
{};

class BrushDoom3API
{
	BrushCreator* m_brushdoom3;
public:
	typedef BrushCreator Type;
	STRING_CONSTANT(Name, "doom3");

	BrushDoom3API() {
		GlobalBrush()->construct();

		m_brushdoom3 = GlobalBrush();
	}
	
	~BrushDoom3API() {
		GlobalBrush()->destroy();
	}
	
	BrushCreator* getTable() {
		return m_brushdoom3;
	}
};

typedef SingletonModule<BrushDoom3API, BrushDependencies> BrushDoom3Module;
typedef Static<BrushDoom3Module> StaticBrushDoom3Module;
StaticRegisterModule staticRegisterBrushDoom3(StaticBrushDoom3Module::instance());
