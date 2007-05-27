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
#include <gtk/gtkimage.h>
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
#include "isound.h"
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
#include "inamespace.h"

#include "gtkutil/messagebox.h"
#include "gtkutil/filechooser.h"

#include "csg.h"
#include "error.h"
#include "map.h"
#include "environment.h"
#include "qe3.h"
#include "gtkmisc.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "mainframe.h"
#include "multimon.h"
#include "camera/GlobalCamera.h"
#include "entity.h"
#include "select.h"
#include "preferences.h"
#include "nullmodel.h"
#include "xyview/GlobalXYWnd.h"
#include "map/AutoSaver.h"
#include "map/PointFile.h"

#include "modulesystem/modulesmap.h"
#include "modulesystem/singletonmodule.h"

#include "generic/callback.h"
#include "settings/GameManager.h"

#include <boost/shared_ptr.hpp>

// TODO: Move this elsewhere
ui::ColourSchemeManager& ColourSchemes() {
	static ui::ColourSchemeManager _manager;
	return _manager;
}

class RadiantCoreAPI :
	public IRadiant
{
public:
	typedef IRadiant Type;
	STRING_CONSTANT(Name, "*");
	
	RadiantCoreAPI() {
		globalOutputStream() << "RadiantCore initialised.\n";
	}
	
	virtual GtkWindow* getMainWindow() {
		return MainFrame_getWindow();
	}
	
	virtual GdkPixbuf* getLocalPixbuf(const std::string& fileName) {
		// Construct the full filename using the Bitmaps path
		std::string fullFileName(GlobalRegistry().get(RKEY_BITMAPS_PATH) + fileName);
		return gdk_pixbuf_new_from_file(fullFileName.c_str(), NULL);
	}
	
	virtual GdkPixbuf* getLocalPixbufWithMask(const std::string& fileName) {
		std::string fullFileName(GlobalRegistry().get(RKEY_BITMAPS_PATH) + fileName);
		
		GdkPixbuf* rgb = gdk_pixbuf_new_from_file(fullFileName.c_str(), 0);
		if (rgb != NULL) {
			// File load successful, add alpha channel
			GdkPixbuf* rgba = gdk_pixbuf_add_alpha(rgb, TRUE, 255, 0, 255);
			gdk_pixbuf_unref(rgb);
			return rgba;
		}
		else {
			// File load failed
			return NULL;
		}
	}
	
	virtual void setStatusText(const std::string& statusText) {
		Sys_Status(statusText);
	}
	
	virtual const char* getGameDescriptionKeyValue(const char* key) {
		return game::Manager::Instance().currentGame()->getKeyValue(key);
	}
	
	virtual const char* getRequiredGameDescriptionKeyValue(const char* key) {
		return game::Manager::Instance().currentGame()->getRequiredKeyValue(key);
	}
	
	virtual Vector3 getColour(const std::string& colourName) {
		return ColourSchemes().getColourVector3(colourName);
	}
  
	virtual void updateAllWindows() {
		UpdateAllWindows();
	}
	
	// Functions needed for the clipper, this should be moved back into the core app
	void splitSelectedBrushes(const Vector3 planePoints[3], const std::string& shader, EBrushSplit split) {
		Scene_BrushSplitByPlane(planePoints, shader, split);
	}
	
	void brushSetClipPlane(const Plane3& plane) {
		Scene_BrushSetClipPlane(plane);
	}

	virtual const char* TextureBrowser_getSelectedShader() {
		return GlobalTextureBrowser().getSelectedShader().c_str();
	}

	IRadiant* getTable() {
		return this;
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
  public GlobalGridModuleRef,
  public GlobalSoundManagerModuleRef
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

class Radiant
{
	typedef boost::shared_ptr<PluginModulesRef> PluginModulesRefPtr;
	PluginModulesRefPtr _plugins; 
public:
  Radiant()
  {
  	// Reset the node id count
  	scene::Node::resetIds();
  	
    GlobalFiletypes().addType(
    	"sound", "wav", FileTypePattern("PCM sound files", "*.wav"));

    Selection_construct();
    MultiMon_Construct();
    map::PointFile::registerCommands();
    Map_Construct();
    MainFrame_Construct();
    GlobalCamera().construct();
    GlobalXYWnd().construct();
    GlobalTextureBrowser().construct();
    Entity_Construct();
    map::AutoSaver().init();
    
    // Instantiate the plugin modules.
	_plugins = PluginModulesRefPtr(new PluginModulesRef("*"));
    
    // Call the initalise methods to trigger the realisation avalanche
    // FileSystem > ShadersModule >> Renderstate etc.
    GlobalFileSystem().initialise();
    
    // Load the shortcuts from the registry
 	GlobalEventManager().loadAccelerators();
  }
  
	void shutDownPlugins() {
		// Broadcast the "shutdown" command to the plugins
		ModulesMap<IPlugin> pluginMap = _plugins->get();
		
		for (ModulesMap<IPlugin>::iterator i = pluginMap.begin(); 
			 i != pluginMap.end(); i++)
		{
			globalOutputStream() << "Shutting down plugin: " << i->first.c_str() << "\n";
			
			// Get the plugin pointer
			IPlugin* plugin = pluginMap.find(i->first);
			// Issue the command
			plugin->shutdown();
		}
	}
  
  ~Radiant()
  {
    GlobalFileSystem().shutdown();

    Entity_Destroy();
    GlobalXYWnd().destroy();
    GlobalCamera().destroy();
    MainFrame_Destroy();
    Map_Destroy();
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
		// greebo: Allocate a new Radiant object on the heap 
		// (this instantiates the plugins as well)
		g_Radiant = new Radiant;
	}
	
	// Return the status
	return g_RadiantInitialised;
}

void Radiant_shutDownPlugins() {
	if (g_RadiantInitialised) {
		g_Radiant->shutDownPlugins();
	}
}

void Radiant_Destroy()
{
	if (g_RadiantInitialised) {
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
