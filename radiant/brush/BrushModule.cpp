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
#include "igame.h"
#include "ilayer.h"
#include "ieventmanager.h"
#include "brush/BrushNode.h"
#include "brush/BrushClipPlane.h"
#include "brush/BrushVisit.h"
#include "brushmanip.h"

#include "ipreferencesystem.h"
#include "stringio.h"
#include "modulesystem/StaticModule.h"

#include "mainframe.h"

// ---------------------------------------------------------------------------------------

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
	BrushNode::constructStatic();
	Brush::constructStatic();
	
	Brush::m_maxWorldCoord = GlobalRegistry().getFloat("game/defaults/maxWorldCoord");
}

void BrushModuleClass::destroy() {
	Brush::m_maxWorldCoord = 0;
	
	Brush::destroyStatic();
	BrushNode::destroyStatic();
	BrushClipPlane::destroyStatic();
}

void BrushModuleClass::clipperColourChanged() {
	BrushClipPlane::destroyStatic();
	BrushClipPlane::constructStatic();
}

void BrushModuleClass::keyChanged(const std::string& key, const std::string& val) {
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
	// Determine the first visible layer
	int layer = GlobalLayerSystem().getFirstVisibleLayer();

	if (layer != -1) {
		scene::INodePtr node(new BrushNode);
		node->setSelf(node);
		
		// Move it to the first visible layer
		node->moveToLayer(layer);
		return node;
	}
	
	return scene::INodePtr();
}

void BrushModuleClass::Brush_forEachFace(scene::INodePtr brush, const BrushFaceDataCallback& callback) {
	::Brush_forEachFace(*Node_getBrush(brush), FaceCallback(BrushFaceDataFromFaceCaller(callback)));
}

// Adds a face plan to the given brush
bool BrushModuleClass::Brush_addFace(scene::INodePtr brush, const _QERFaceData& faceData) {
	Node_getBrush(brush)->undoSave();
	return Node_getBrush(brush)->addPlane(faceData.m_p0, faceData.m_p1, faceData.m_p2, faceData.m_shader, TextureProjection(faceData.m_texdef, BrushPrimitTexDef(), Vector3(0, 0, 0), Vector3(0, 0, 0))) != 0;
}

// RegisterableModule implementation
const std::string& BrushModuleClass::getName() const {
	static std::string _name(MODULE_BRUSHCREATOR);
	return _name;
}

const StringSet& BrushModuleClass::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_GAMEMANAGER);
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
		_dependencies.insert(MODULE_SHADERCACHE);
		_dependencies.insert(MODULE_UNDOSYSTEM);
	}

	return _dependencies;
}

void BrushModuleClass::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "BrushModuleClass::initialiseModule called.\n";
	
	construct();
	
	_textureLockEnabled = (GlobalRegistry().get(RKEY_ENABLE_TEXTURE_LOCK) == "1");
	
	GlobalRegistry().addKeyObserver(this, RKEY_ENABLE_TEXTURE_LOCK);
	
	// add the preference settings
	constructPreferences();
}

void BrushModuleClass::shutdownModule() {
	globalOutputStream() << "BrushModuleClass::shutdownModule called.\n";
	destroy();
}

// -------------------------------------------------------------------------------------

// Define a static BrushModule 
module::StaticModule<BrushModuleClass> staticBrushModule;

ShaderPtr BrushClipPlane::m_state;

// greebo: The accessor function for the brush module containing the static instance
// TODO: Change this to return a reference instead of a raw pointer
BrushModuleClass* GlobalBrush() {
	return staticBrushModule.getModule().get();
}
