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

#include <iostream>
#include "plugin.h"

#include "debugging/debugging.h"

#include "brush/TexDef.h"
#include "iradiant.h"
#include "iplugin.h"
#include "ifilesystem.h"
#include "ishaders.h"
#include "iclipper.h"
#include "igrid.h"
#include "ieventmanager.h"
#include "ientity.h"
#include "ieclass.h"
#include "irender.h"
#include "iscenegraph.h"
#include "iselection.h"
#include "ifilter.h"
#include "iscriplib.h"
#include "igl.h"
#include "iundo.h"
#include "ireference.h"
#include "ifiletypes.h"
#include "preferencesystem.h"
#include "ibrush.h"
#include "iuimanager.h"
#include "ipatch.h"
#include "iimage.h"
#include "itoolbar.h"
#include "imap.h"
#include "namespace.h"

#include "gtkutil/messagebox.h"
#include "gtkutil/filechooser.h"
#include "maplib.h"

#include "csg.h"
#include "error.h"
#include "map.h"
#include "qe3.h"
#include "entitylist.h"
#include "points.h"
#include "gtkmisc.h"
#include "texwindow.h"
#include "mainframe.h"
#include "multimon.h"
#include "groupdialog.h"
#include "camera/GlobalCamera.h"
#include "entity.h"
#include "select.h"
#include "preferences.h"
#include "nullmodel.h"
#include "xyview/GlobalXYWnd.h"
#include "map/AutoSaver.h"

#include "modulesystem/modulesmap.h"
#include "modulesystem/singletonmodule.h"

#include "generic/callback.h"
#include "settings/GameManager.h"

const char* GameDescription_getKeyValue(const char* key)
{
  return game::Manager::Instance().currentGame()->getKeyValue(key);
}

const char* GameDescription_getRequiredKeyValue(const char* key)
{
  return game::Manager::Instance().currentGame()->getRequiredKeyValue(key);
}

ui::ColourSchemeManager& ColourSchemes() {
	static ui::ColourSchemeManager _manager;
	return _manager;
}

Vector3 getColour(const std::string& colourName) {
	return ColourSchemes().getColourVector3(colourName);
}

const char* getMapName()
{
  return map::getFileName().c_str();
}

scene::Node& getMapWorldEntity()
{
  return Map_FindOrInsertWorldspawn(g_map);
}

const char* TextureBrowser_getSelectedShader()
{
  return TextureBrowser_GetSelectedShader(GlobalTextureBrowser());
}

class RadiantCoreAPI
{
  IRadiant m_radiantcore;
public:
  typedef IRadiant Type;
  STRING_CONSTANT(Name, "*");

  RadiantCoreAPI()
  {
  	m_radiantcore.getMainWindow = MainFrame_getWindow;
  	m_radiantcore.setStatusText = Sys_Status;
  	
    m_radiantcore.getEnginePath = &EnginePath_get;
    
    m_radiantcore.getGameName = &gamename_get;
    m_radiantcore.getGameMode = &gamemode_get;

    m_radiantcore.getMapName = &getMapName;
    m_radiantcore.getMapWorldEntity = getMapWorldEntity;

    m_radiantcore.getGameDescriptionKeyValue = &GameDescription_getKeyValue;
    m_radiantcore.getRequiredGameDescriptionKeyValue = &GameDescription_getRequiredKeyValue;
    
    m_radiantcore.getColour = &getColour;
    m_radiantcore.updateAllWindows = &UpdateAllWindows;
    m_radiantcore.splitSelectedBrushes = &Scene_BrushSplitByPlane;
    m_radiantcore.brushSetClipPlane = &Scene_BrushSetClipPlane; 
    
    m_radiantcore.TextureBrowser_getSelectedShader = TextureBrowser_getSelectedShader;

  }
  IRadiant* getTable()
  {
    return &m_radiantcore;
  }
};

typedef SingletonModule<RadiantCoreAPI> RadiantCoreModule;
typedef Static<RadiantCoreModule> StaticRadiantCoreModule;
StaticRegisterModule staticRegisterRadiantCore(StaticRadiantCoreModule::instance());


class RadiantDependencies :
  public GlobalRadiantModuleRef,
  public GlobalEventManagerModuleRef,
  public GlobalUIManagerModuleRef,
  public GlobalFileSystemModuleRef,
  public GlobalEntityModuleRef,
  public GlobalShadersModuleRef,
  public GlobalBrushModuleRef,
  public GlobalSceneGraphModuleRef,
  public GlobalShaderCacheModuleRef,
  public GlobalFiletypesModuleRef,
  public GlobalSelectionModuleRef,
  public GlobalReferenceModuleRef,
  public GlobalOpenGLModuleRef,
  public GlobalEntityClassManagerModuleRef,
  public GlobalUndoModuleRef,
  public GlobalScripLibModuleRef,
  public GlobalNamespaceModuleRef,
  public GlobalClipperModuleRef,
  public GlobalGridModuleRef
{
  ImageModulesRef m_image_modules;
  MapModulesRef m_map_modules;

public:
  RadiantDependencies() :
    GlobalEntityModuleRef(GlobalRadiant().getRequiredGameDescriptionKeyValue("entities")),
    GlobalShadersModuleRef(GlobalRadiant().getRequiredGameDescriptionKeyValue("shaders")),
    GlobalBrushModuleRef(GlobalRadiant().getRequiredGameDescriptionKeyValue("brushtypes")),
    GlobalEntityClassManagerModuleRef(GlobalRadiant().getRequiredGameDescriptionKeyValue("entityclass")),
    m_image_modules(GlobalRadiant().getRequiredGameDescriptionKeyValue("texturetypes")),
    m_map_modules(GlobalRadiant().getRequiredGameDescriptionKeyValue("maptypes"))
  {
  }

  ImageModules& getImageModules()
  {
    return m_image_modules.get();
  }
  MapModules& getMapModules()
  {
    return m_map_modules.get();
  }
};

class Radiant : public TypeSystemRef
{
public:
  Radiant()
  {
    Preferences_Init();

	GlobalFiletypes().addType(
    	"sound", "wav", FileTypePattern("PCM sound files", "*.wav"));

    Selection_construct();
    MultiMon_Construct();
    Pointfile_Construct();
    Map_Construct();
    EntityList_Construct();
    MainFrame_Construct();
    GroupDialog_Construct();
    GlobalCamera().construct();
    GlobalXYWnd().construct();
    TextureBrowser_Construct();
    Entity_Construct();
    NullModel_construct();
    MapRoot_construct();
    map::AutoSaver().init();

    //EnginePath_verify();
    //EnginePath_Realise();
    
    // Call the initalise methods to trigger the realisation avalanche
    // FileSystem > ShadersModule >> Renderstate etc.
    GlobalFileSystem().initialise();
    
    // Load the shortcuts from the registry
 	GlobalEventManager().loadAccelerators();
  }
  ~Radiant()
  {
    EnginePath_Unrealise();

    MapRoot_destroy();
    NullModel_destroy();
    Entity_Destroy();
    TextureBrowser_Destroy();
    GlobalXYWnd().destroy();
    GlobalCamera().destroy();
    GroupDialog_Destroy();
    MainFrame_Destroy();
    EntityList_Destroy();
    Map_Destroy();
    Pointfile_Destroy();
    MultiMon_Destroy();
    Selection_destroy();
  }
};

namespace
{
  bool g_RadiantInitialised = false;
  RadiantDependencies* g_RadiantDependencies;
  Radiant* g_Radiant;
}

/**
 * Construct the Radiant module.
 */
bool Radiant_Construct(ModuleServer& server)
{
	GlobalModuleServer::instance().set(server);
	StaticModuleRegistryList().instance().registerModules();

	g_RadiantDependencies = new RadiantDependencies();

	// If the module server has been set up correctly, create the Radiant
	// module
	g_RadiantInitialised = !server.getError();
	if(g_RadiantInitialised) {
		g_Radiant = new Radiant;
	}

	// Instantiate the plugin modules.
	PluginModulesRef plugMods("*");

	// Return the status
	return g_RadiantInitialised;
}
void Radiant_Destroy()
{
  if(g_RadiantInitialised)
  {
    delete g_Radiant;
  }

  delete g_RadiantDependencies;
}

ImageModules& Radiant_getImageModules()
{
  return g_RadiantDependencies->getImageModules();
}
MapModules& Radiant_getMapModules()
{
  return g_RadiantDependencies->getMapModules();
}
