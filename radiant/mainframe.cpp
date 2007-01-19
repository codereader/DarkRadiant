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

//
// Main Window for Q3Radiant
//
// Leonardo Zide (leo@lokigames.com)
//

#include "mainframe.h"

#include "debugging/debugging.h"
#include "version.h"

#include "brushexport/BrushExportOBJ.h"
#include "ui/einspector/EntityInspector.h"
#include "ui/lightinspector/LightInspector.h"
#include "ui/mediabrowser/MediaBrowser.h"
#include "ui/common/ToolbarCreator.h"
#include "ui/colourscheme/ColourSchemeManager.h"
#include "ui/colourscheme/ColourSchemeEditor.h"
#include "ui/menu/FiltersMenu.h"
#include "iclipper.h"
#include "ifilesystem.h"
#include "iundo.h"
#include "igrid.h"
#include "ifilter.h"
#include "itoolbar.h"
#include "editable.h"
#include "ientity.h"
#include "ieventmanager.h"
#include "ieclass.h"
#include "iregistry.h"
#include "irender.h"
#include "ishaders.h"
#include "igl.h"
#include "moduleobserver.h"
#include "server.h"
#include "os/dir.h"

#include <ctime>
#include <iostream>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkframe.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkhpaned.h>
#include <gtk/gtkvpaned.h>
#include <gtk/gtktoolbar.h>
#include <gtk/gtkmenubar.h>
#include <gtk/gtkimage.h>
#include <gtk/gtktable.h>

#include <boost/algorithm/string/predicate.hpp>

#include "cmdlib.h"
#include "scenelib.h"
#include "stream/stringstream.h"
#include "signal/isignal.h"
#include "os/path.h"
#include "os/file.h"
#include "moduleobservers.h"

#include "gtkutil/clipboard.h"
#include "gtkutil/container.h"
#include "gtkutil/frame.h"
#include "gtkutil/glfont.h"
#include "gtkutil/glwidget.h"
#include "gtkutil/image.h"
#include "gtkutil/menu.h"
#include "gtkutil/paned.h"
#include "gtkutil/widget.h"
#include "gtkutil/IconTextMenuToggle.h"
#include "gtkutil/TextMenuItem.h"
#include "gtkutil/messagebox.h"
#include "gtkutil/dialog.h"

#include "autosave.h"
#include "brushmanip.h"
#include "brush/BrushModule.h"
#include "csg.h"
#include "gtkutil/accelerator.h"
#include "console.h"
#include "entity.h"
#include "entitylist.h"
#include "findtexturedialog.h"
#include "groupdialog.h"
#include "gtkdlgs.h"
#include "gtkmisc.h"
#include "map.h"
#include "multimon.h"
#include "patchdialog.h"
#include "patchmanip.h"
#include "plugin.h"
#include "plugin/PluginManager.h"
#include "pluginmenu.h"
#include "points.h"
#include "preferences.h"
#include "qe3.h"
#include "qgl.h"
#include "select.h"
#include "server.h"
#include "surfacedialog.h"
#include "textures.h"
#include "texwindow.h"
#include "windowobservers.h"
#include "renderstate.h"
#include "referencecache.h"
#include "camera/GlobalCamera.h"
#include "camera/CameraSettings.h"
#include "xyview/GlobalXYWnd.h"
#include "ui/mru/MRU.h"
#include "ui/commandlist/CommandList.h"

struct layout_globals_t
{
  WindowPosition m_position;


  int nXYHeight;
  int nXYWidth;
  int nCamWidth;
  int nCamHeight;
  int nState;

  layout_globals_t() :
    m_position(-1, -1, 640, 480),

    nXYHeight(300),
    nXYWidth(300),
    nCamWidth(200),
    nCamHeight(200),
    nState(GDK_WINDOW_STATE_MAXIMIZED)
  {
  }
};

layout_globals_t g_layout_globals;

// VFS
class VFSModuleObserver : public ModuleObserver
{
  std::size_t m_unrealised;
public:
  VFSModuleObserver() : m_unrealised(1)
  {
  }
  void realise()
  {
    if(--m_unrealised == 0)
    {
      QE_InitVFS();
      GlobalFileSystem().initialise();
    }
  }
  void unrealise()
  {
    if(++m_unrealised == 1)
    {
      GlobalFileSystem().shutdown();
    }
  }
};

VFSModuleObserver g_VFSModuleObserver;

void VFS_Construct()
{
    Radiant_attachHomePathsObserver(g_VFSModuleObserver);
}
void VFS_Destroy()
{
    Radiant_detachHomePathsObserver(g_VFSModuleObserver);
}

// Home Paths

void HomePaths_Realise()
{
#if defined(POSIX)
  const char* prefix = g_pGameDescription->getKeyValue("prefix");
  if(!string_empty(prefix)) 
  {
    StringOutputStream path(256);
    path << DirectoryCleaned(g_get_home_dir()) << prefix << "/";
    g_qeglobals.m_userEnginePath = path.c_str();
    Q_mkdir(g_qeglobals.m_userEnginePath.c_str());
  }
  else
#endif
  {
    g_qeglobals.m_userEnginePath = EnginePath_get();
  }

  {
    StringOutputStream path(256);
    path << g_qeglobals.m_userEnginePath.c_str() << gamename_get() << '/';
    g_qeglobals.m_userGamePath = path.c_str();
  }
  ASSERT_MESSAGE(!string_empty(g_qeglobals.m_userGamePath.c_str()), "HomePaths_Realise: user-game-path is empty");
  Q_mkdir(g_qeglobals.m_userGamePath.c_str());
}

ModuleObservers g_homePathObservers;

void Radiant_attachHomePathsObserver(ModuleObserver& observer)
{
  g_homePathObservers.attach(observer);
}

void Radiant_detachHomePathsObserver(ModuleObserver& observer)
{
  g_homePathObservers.detach(observer);
}

class HomePathsModuleObserver : public ModuleObserver
{
  std::size_t m_unrealised;
public:
  HomePathsModuleObserver() : m_unrealised(1)
  {
  }
  void realise()
  {
    if(--m_unrealised == 0)
    {
      HomePaths_Realise();
      g_homePathObservers.realise();
    }
  }
  void unrealise()
  {
    if(++m_unrealised == 1)
    {
      g_homePathObservers.unrealise();
    }
  }
};

HomePathsModuleObserver g_HomePathsModuleObserver;

void HomePaths_Construct()
{
    Radiant_attachEnginePathObserver(g_HomePathsModuleObserver);
}
void HomePaths_Destroy()
{
    Radiant_detachEnginePathObserver(g_HomePathsModuleObserver);
}


// Engine Path

CopiedString g_strEnginePath;
ModuleObservers g_enginePathObservers;
std::size_t g_enginepath_unrealised = 1;

void Radiant_attachEnginePathObserver(ModuleObserver& observer)
{
  g_enginePathObservers.attach(observer);
}

void Radiant_detachEnginePathObserver(ModuleObserver& observer)
{
  g_enginePathObservers.detach(observer);
}


void EnginePath_Realise()
{
  if(--g_enginepath_unrealised == 0)
  {
    g_enginePathObservers.realise();
  }
}


const char* EnginePath_get()
{
  ASSERT_MESSAGE(g_enginepath_unrealised == 0, "EnginePath_get: engine path not realised");
  return g_strEnginePath.c_str();
}

void EnginePath_Unrealise()
{
  if(++g_enginepath_unrealised == 1)
  {
    g_enginePathObservers.unrealise();
  }
}

void setEnginePath(const char* path)
{
  StringOutputStream buffer(256);
  buffer << DirectoryCleaned(path);
  if(!path_equal(buffer.c_str(), g_strEnginePath.c_str()))
  {
#if 0
    while(!ConfirmModified("Paths Changed"))
    {
      if(Map_Unnamed(g_map))
      {
        Map_SaveAs();
      }
      else
      {
        Map_Save();
      }
    }
    Map_RegionOff();
#endif

    ScopeDisableScreenUpdates disableScreenUpdates("Processing...", "Changing Engine Path");

    EnginePath_Unrealise();

    g_strEnginePath = buffer.c_str();

    EnginePath_Realise();
  }
}


// App Path

CopiedString g_strAppPath;                 ///< holds the full path of the executable

const char* AppPath_get()
{
  return g_strAppPath.c_str();
}

/// directory for temp files
/// NOTE: on *nix this is were we check for .pid
CopiedString g_strSettingsPath;
const char* SettingsPath_get()
{
  return g_strSettingsPath.c_str();
}


void EnginePathImport(CopiedString& self, const char* value)
{
  setEnginePath(value);
}
typedef ReferenceCaller1<CopiedString, const char*, EnginePathImport> EnginePathImportCaller;

void Paths_constructPreferences(PrefPage* page)
{
  page->appendPathEntry("Engine Path", true,
    StringImportCallback(EnginePathImportCaller(g_strEnginePath)),
    StringExportCallback(StringExportCaller(g_strEnginePath))
  );
}
void Paths_constructPage(PreferenceGroup& group)
{
  PreferencesPage* page(group.createPage("Paths", "Path Settings"));
  Paths_constructPreferences(reinterpret_cast<PrefPage*>(page));
}
void Paths_registerPreferencesPage()
{
  PreferencesDialog_addSettingsPage(FreeCaller1<PreferenceGroup&, Paths_constructPage>());
}


class PathsDialog : public Dialog
{
public:
  GtkWindow* BuildDialog()
  {
    GtkFrame* frame = create_dialog_frame("Path settings", GTK_SHADOW_ETCHED_IN);

    GtkVBox* vbox2 = create_dialog_vbox(0, 4);
    gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(vbox2));

    {
      PrefPage preferencesPage(*this, GTK_WIDGET(vbox2));
      Paths_constructPreferences(&preferencesPage);
    }

    return create_simple_modal_dialog_window("Engine Path Not Found", m_modal, GTK_WIDGET(frame));
  }
};

PathsDialog g_PathsDialog;

void EnginePath_verify()
{
  if(!file_exists(g_strEnginePath.c_str()))
  {
    g_PathsDialog.Create();
    g_PathsDialog.DoModal();
    g_PathsDialog.Destroy();
  }
}

namespace
{
  CopiedString g_gamename;
  CopiedString g_gamemode;
  ModuleObservers g_gameNameObservers;
  ModuleObservers g_gameModeObservers;
}

void Radiant_attachGameNameObserver(ModuleObserver& observer)
{
  g_gameNameObservers.attach(observer);
}

void Radiant_detachGameNameObserver(ModuleObserver& observer)
{
  g_gameNameObservers.detach(observer);
}

const char* basegame_get()
{
  return g_pGameDescription->getRequiredKeyValue("basegame");
}

const char* gamename_get()
{
  const char* gamename = g_gamename.c_str();
  if(string_empty(gamename))
  {
    return basegame_get();
  }
  return gamename;
}

void gamename_set(const char* gamename)
{
  if(!string_equal(gamename, g_gamename.c_str()))
  {
    g_gameNameObservers.unrealise();
    g_gamename = gamename;
    g_gameNameObservers.realise();
  }
}

void Radiant_attachGameModeObserver(ModuleObserver& observer)
{
  g_gameModeObservers.attach(observer);
}

void Radiant_detachGameModeObserver(ModuleObserver& observer)
{
  g_gameModeObservers.detach(observer);
}

const char* gamemode_get()
{
  return g_gamemode.c_str();
}

void gamemode_set(const char* gamemode)
{
  if(!string_equal(gamemode, g_gamemode.c_str()))
  {
    g_gameModeObservers.unrealise();
    g_gamemode = gamemode;
    g_gameModeObservers.realise();
  }
}

/** Module loader functor class. This class is used to traverse a directory and
 * load each module into the GlobalModuleServer.
 */
class ModuleLoader
{
	// The path of the directory we are in
	const std::string _path;
	
	// The filename extension which indicates a module (platform-specific)
	const std::string _ext;
	
public:

	// Constructor sets platform-specific extension to match
	ModuleLoader(const std::string& path) 
	: _path(path),

#if defined(WIN32)
	  _ext(".dll")
#elif defined(POSIX)
	  _ext(".so")
#endif

	{}

	// Functor operator
	void operator() (const std::string& fileName) const {
		// Check for correct extension
		if (boost::algorithm::iends_with(fileName, _ext)) {
			std::string fullName = _path + fileName;
			globalOutputStream() << "Loading module '" << fullName << "'\n";      
			GlobalModuleServer_loadModule(fullName);
		}
	}
};

/** Load modules from a specified directory.
 * 
 * @param path
 * The directory path to load from.
 */
void Radiant_loadModules(const std::string& path)
{
	ModuleLoader loader(path);
	Directory_forEach(path, loader);
}

/** Load all of the modules in the DarkRadiant install directory. Modules
 * are loaded from modules/ and plugins/.
 * 
 * @param directory
 * The root directory to search.
 */
void Radiant_loadModulesFromRoot(const std::string& directory)
{
    Radiant_loadModules(directory + g_modulesDir);
    Radiant_loadModules(directory + g_pluginsDir);
}

//! Make COLOR_BRUSHES override worldspawn eclass colour.
void SetWorldspawnColour(const Vector3& colour)
{
	IEntityClass* worldspawn = GlobalEntityClassManager().findOrInsert("worldspawn", true);
	worldspawn->setColour(colour);
}

//! Make colourscheme definition override light volume eclass colour.
void SetLightVolumeColour(const Vector3& colour)
{
	IEntityClass* light = GlobalEntityClassManager().findOrInsert("light", true);
	light->setColour(colour);
}

class WorldspawnColourEntityClassObserver : public ModuleObserver
{
  std::size_t m_unrealised;
public:
  WorldspawnColourEntityClassObserver() : m_unrealised(1)
  {
  }
  void realise()
  {
    if(--m_unrealised == 0)
    {
      SetWorldspawnColour(ColourSchemes().getColourVector3("default_brush"));
      SetLightVolumeColour(ColourSchemes().getColourVector3("light_volumes"));
    }
  }
  void unrealise()
  {
    if(++m_unrealised == 1)
    {
    }
  }
};

WorldspawnColourEntityClassObserver g_WorldspawnColourEntityClassObserver;


ModuleObservers g_gameToolsPathObservers;

void Radiant_attachGameToolsPathObserver(ModuleObserver& observer)
{
  g_gameToolsPathObservers.attach(observer);
}

void Radiant_detachGameToolsPathObserver(ModuleObserver& observer)
{
  g_gameToolsPathObservers.detach(observer);
}

/*
 * Load XML config files into the XML registry.
 */
void populateRegistry() {
	
	// Load the XML files from the installation directory
	std::string base = AppPath_get();

	try {
		// Load all of the required XML files
		GlobalRegistry().importFromFile(base + "user.xml", "");
		GlobalRegistry().importFromFile(base + "upgradepaths.xml", "user");
		GlobalRegistry().importFromFile(base + "colours.xml", "user/ui");
		GlobalRegistry().importFromFile(base + "input.xml", "user/ui");
		
		// Load the debug.xml file only if the relevant key is set in user.xml
		if (GlobalRegistry().get("user/debug") == "1")
			GlobalRegistry().importFromFile(base + "debug.xml", "");
	}
	catch (std::runtime_error e) {
		gtkutil::fatalErrorDialog("XML registry population failed:\n\n"
								  + std::string(e.what()));
	}
	
	// Traverse the game files stored in the GamesDialog class and load them into the registry
	// The information stored in the game files is needed to successfully instantiate the other modules
	for (std::list<CGameDescription*>::iterator game = g_GamesDialog.mGames.begin(); 
		 game != g_GamesDialog.mGames.end(); game++) 
	{
		// Construct the filename and load it into the registry
		const std::string filename = std::string(AppPath_get()) + "games/" + (*game)->mGameFile.c_str();  	
		GlobalRegistry().importFromFile(filename, "");
	}
  
	// Load user preferences, these overwrite any values that have defined before
	// The called method also checks for any upgrades that have to be performed
	const std::string userSettingsFile = std::string(SettingsPath_get()) + "user.xml";
	if (file_exists(userSettingsFile.c_str())) {
		GlobalRegistry().importUserXML(userSettingsFile);
	}
	
	const std::string userColoursFile = std::string(SettingsPath_get()) + "colours.xml";
	if (file_exists(userColoursFile.c_str())) {
		GlobalRegistry().importFromFile(userColoursFile, "user/ui");
	}
	
	const std::string userInputFile = std::string(SettingsPath_get()) + "input.xml";
	if (file_exists(userInputFile.c_str())) {
		GlobalRegistry().importFromFile(userInputFile, "user/ui");
	}
}

// This is called from main() to start up the Radiant stuff.
void Radiant_Initialise() 
{
	// Initialise the module server
	GlobalModuleServer_Initialise();
  
	// Load the Radiant modules from the modules/ dir.
	Radiant_loadModulesFromRoot(AppPath_get());

	// Initialise and instantiate the registry
	GlobalModuleServer::instance().set(GlobalModuleServer_get());
	GlobalRegistryModuleRef registryRef;
	
	// Try to load all the XML files into the registry
	populateRegistry();
	
	// Load the ColourSchemes from the registry
	ColourSchemes().loadColourSchemes();

	Preferences_Load();

	// Load the other modules
	Radiant_Construct(GlobalModuleServer_get());
	
	g_gameToolsPathObservers.realise();
	g_gameModeObservers.realise();
	g_gameNameObservers.realise();
	
	// Initialise the most recently used files list
	GlobalMRU().loadRecentFiles();
}

void Radiant_Shutdown() {
	GlobalMRU().saveRecentFiles();
	
	// Export the colour schemes and remove them from the registry
	GlobalRegistry().exportToFile("user/ui/colourschemes", std::string(SettingsPath_get()) + "colours.xml");
	GlobalRegistry().deleteXPath("user/ui/colourschemes");
	
	// Save the current event set to the Registry and export it 
	GlobalEventManager().saveEventListToRegistry();
	
	// Export the input definitions into the user's settings folder and remove them as well
	GlobalRegistry().exportToFile("user/ui/input", std::string(SettingsPath_get()) + "input.xml");
	GlobalRegistry().deleteXPath("user/ui/input");
	
	// Delete all nodes marked as "transient", they are NOT exported into the user's xml file
	GlobalRegistry().deleteXPath("user/*[@transient='1']");
	
	// Remove any remaining upgradePaths (from older registry files)
	GlobalRegistry().deleteXPath("user/upgradePaths");
	// Remove legacy <interface> node
	GlobalRegistry().deleteXPath("user/ui/interface");
	
	// Save the remaining /darkradiant/user tree to user.xml so that the current settings are preserved
	GlobalRegistry().exportToFile("user", std::string(SettingsPath_get()) + "user.xml");

  g_gameNameObservers.unrealise();
  g_gameModeObservers.unrealise();
  g_gameToolsPathObservers.unrealise();

  if (!g_preferences_globals.disable_ini)
  {
    globalOutputStream() << "Start writing prefs\n";
    Preferences_Save();
    globalOutputStream() << "Done prefs\n";
  }

  Radiant_Destroy();

  GlobalModuleServer_Shutdown();
}

void Exit()
{
  if(ConfirmModified("Exit Radiant"))
  {
    gtk_main_quit();
  }
}


void Undo()
{
  GlobalUndoSystem().undo();
  SceneChangeNotify();
}

void Redo()
{
  GlobalUndoSystem().redo();
  SceneChangeNotify();
}

void deleteSelection()
{
  UndoableCommand undo("deleteSelected");
  Select_Delete();
}

void Map_ExportSelected(std::ostream& ostream)
{
	Map_ExportSelected(ostream, Map_getFormat(g_map));
}

void Map_ImportSelected(TextInputStream& istream)
{
  Map_ImportSelected(istream, Map_getFormat(g_map));
}

void Selection_Copy()
{
  clipboard_copy(Map_ExportSelected);
}

void Selection_Paste()
{
  clipboard_paste(Map_ImportSelected);
}

void Copy()
{
  if(SelectedFaces_empty())
  {
    Selection_Copy();
  }
  else
  {
    SelectedFaces_copyTexture();
  }
}

void Paste()
{
  if(SelectedFaces_empty())
  {
    UndoableCommand undo("paste");
    
    GlobalSelectionSystem().setSelectedAll(false);
    Selection_Paste();
  }
  else
  {
    SelectedFaces_pasteTexture();
  }
}

void PasteToCamera()
{
  CamWnd& camwnd = *g_pParentWnd->GetCamWnd();
  GlobalSelectionSystem().setSelectedAll(false);
  
  UndoableCommand undo("pasteToCamera");
  
  Selection_Paste();
  
  // Work out the delta
  Vector3 mid;
  Select_GetMid(mid);
  Vector3 delta = vector3_snapped(camwnd.getCameraOrigin(), GlobalGrid().getGridSize()) - mid;
  
  // Move to camera
  GlobalSelectionSystem().translateSelected(delta);
}

void updateTextureBrowser() {
	TextureBrowser_queueDraw(GlobalTextureBrowser());
}

void Restart()
{
  PluginsMenu_clear();

  Radiant_Shutdown();
  Radiant_Initialise();

  PluginsMenu_populate();

}


void thunk_OnSleep()
{
  g_pParentWnd->OnSleep();
}

GtkWidget* g_page_console;

void Console_ToggleShow()
{
  GroupDialog_showPage(g_page_console);
}

GtkWidget* g_page_entity;

void EntityInspector_ToggleShow()
{  
  GroupDialog_showPage(g_page_entity);
}



void SetClipMode(bool enable);
void ModeChangeNotify();

typedef void(*ToolMode)();
ToolMode g_currentToolMode = 0;
bool g_currentToolModeSupportsComponentEditing = false;
ToolMode g_defaultToolMode = 0;



void SelectionSystem_DefaultMode()
{
  GlobalSelectionSystem().SetMode(SelectionSystem::ePrimitive);
  GlobalSelectionSystem().SetComponentMode(SelectionSystem::eDefault);
  ModeChangeNotify();
}


bool EdgeMode() {
	return GlobalSelectionSystem().Mode() == SelectionSystem::eComponent
	       && GlobalSelectionSystem().ComponentMode() == SelectionSystem::eEdge;
}

bool VertexMode() {
	return GlobalSelectionSystem().Mode() == SelectionSystem::eComponent
	       && GlobalSelectionSystem().ComponentMode() == SelectionSystem::eVertex;
}

bool FaceMode() {
	return GlobalSelectionSystem().Mode() == SelectionSystem::eComponent
	       && GlobalSelectionSystem().ComponentMode() == SelectionSystem::eFace;
}

void ComponentModeChanged() {
	GlobalEventManager().setToggled("DragVertices", VertexMode());
	GlobalEventManager().setToggled("DragEdges", EdgeMode());
	GlobalEventManager().setToggled("DragFaces", FaceMode()); 
}

void ComponentMode_SelectionChanged(const Selectable& selectable) {
	if (GlobalSelectionSystem().Mode() == SelectionSystem::eComponent
	        && GlobalSelectionSystem().countSelected() == 0) {
		SelectionSystem_DefaultMode();
		ComponentModeChanged();
	}
}

void ToggleEdgeMode() {
	if (EdgeMode()) {
		SelectionSystem_DefaultMode();
	}
	else if (GlobalSelectionSystem().countSelected() != 0) {
		if (!g_currentToolModeSupportsComponentEditing) {
			g_defaultToolMode();
		}

		GlobalSelectionSystem().SetMode(SelectionSystem::eComponent);
		GlobalSelectionSystem().SetComponentMode(SelectionSystem::eEdge);
	}

	ComponentModeChanged();

	ModeChangeNotify();
}

void ToggleVertexMode() {
	if (VertexMode()) {
		SelectionSystem_DefaultMode();
	}
	else if(GlobalSelectionSystem().countSelected() != 0) {
		if (!g_currentToolModeSupportsComponentEditing) {
			g_defaultToolMode();
		}

		GlobalSelectionSystem().SetMode(SelectionSystem::eComponent);
		GlobalSelectionSystem().SetComponentMode(SelectionSystem::eVertex);
	}

	ComponentModeChanged();

	ModeChangeNotify();
}

void ToggleFaceMode() {
	if (FaceMode()) {
		SelectionSystem_DefaultMode();
	}
	else if (GlobalSelectionSystem().countSelected() != 0) {
		if (!g_currentToolModeSupportsComponentEditing) {
			g_defaultToolMode();
		}

		GlobalSelectionSystem().SetMode(SelectionSystem::eComponent);
		GlobalSelectionSystem().SetComponentMode(SelectionSystem::eFace);
	}

	ComponentModeChanged();

	ModeChangeNotify();
}


class CloneSelected : public scene::Graph::Walker
{
public:
  bool pre(const scene::Path& path, scene::Instance& instance) const
  {
    if(path.size() == 1)
      return true;
    
    if(!path.top().get().isRoot())
    {
      Selectable* selectable = Instance_getSelectable(instance);
      if(selectable != 0
        && selectable->isSelected())
      {
        return false;
      }
    }

    return true;
  }
  void post(const scene::Path& path, scene::Instance& instance) const
  {
    if(path.size() == 1)
      return;

    if(!path.top().get().isRoot())
    {
      Selectable* selectable = Instance_getSelectable(instance);
      if(selectable != 0
        && selectable->isSelected())
      {
        NodeSmartReference clone(Node_Clone(path.top()));
        Map_gatherNamespaced(clone);
        Node_getTraversable(path.parent().get())->insert(clone);
      }
    }
  }
};

void Scene_Clone_Selected(scene::Graph& graph)
{
  graph.traverse(CloneSelected());

  Map_mergeClonedNames();
}

enum ENudgeDirection
{
  eNudgeUp = 1,
  eNudgeDown = 3,
  eNudgeLeft = 0,
  eNudgeRight = 2,
};

struct AxisBase
{
  Vector3 x;
  Vector3 y;
  Vector3 z;
  AxisBase(const Vector3& x_, const Vector3& y_, const Vector3& z_)
    : x(x_), y(y_), z(z_)
  {
  }
};

AxisBase AxisBase_forViewType(EViewType viewtype)
{
  switch(viewtype)
  {
  case XY:
    return AxisBase(g_vector3_axis_x, g_vector3_axis_y, g_vector3_axis_z);
  case XZ:
    return AxisBase(g_vector3_axis_x, g_vector3_axis_z, g_vector3_axis_y);
  case YZ:
    return AxisBase(g_vector3_axis_y, g_vector3_axis_z, g_vector3_axis_x);
  }

  ERROR_MESSAGE("invalid viewtype");
  return AxisBase(Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0));
}

Vector3 AxisBase_axisForDirection(const AxisBase& axes, ENudgeDirection direction)
{
  switch (direction)
  {
  case eNudgeLeft:
    return -axes.x;
  case eNudgeUp:
    return axes.y;
  case eNudgeRight:
    return axes.x;
  case eNudgeDown:
    return -axes.y;
  }

  ERROR_MESSAGE("invalid direction");
  return Vector3(0, 0, 0);
}

void NudgeSelection(ENudgeDirection direction, float fAmount, EViewType viewtype)
{
  AxisBase axes(AxisBase_forViewType(viewtype));
  Vector3 view_direction(-axes.z);
  Vector3 nudge(AxisBase_axisForDirection(axes, direction) * fAmount);
  GlobalSelectionSystem().NudgeManipulator(nudge, view_direction);
}

void Selection_Clone()
{
  if(GlobalSelectionSystem().Mode() == SelectionSystem::ePrimitive)
  {
    UndoableCommand undo("cloneSelected");

    Scene_Clone_Selected(GlobalSceneGraph());

    //NudgeSelection(eNudgeRight, GlobalGrid().getGridSize(), GlobalXYWnd_getCurrentViewType());
    //NudgeSelection(eNudgeDown, GlobalGrid().getGridSize(), GlobalXYWnd_getCurrentViewType());
  }
}

// called when the escape key is used (either on the main window or on an inspector)
void Selection_Deselect()
{
  if(GlobalSelectionSystem().Mode() == SelectionSystem::eComponent)
  {
    if(GlobalSelectionSystem().countSelectedComponents() != 0)
    {
      GlobalSelectionSystem().setSelectedAllComponents(false);
    }
    else
    {
      SelectionSystem_DefaultMode();
      ComponentModeChanged();
    }
  }
  else
  {
    if(GlobalSelectionSystem().countSelectedComponents() != 0)
    {
      GlobalSelectionSystem().setSelectedAllComponents(false);
    }
    else
    {
      GlobalSelectionSystem().setSelectedAll(false);
    }
  }
}


void Selection_NudgeUp()
{
  UndoableCommand undo("nudgeSelectedUp");
  NudgeSelection(eNudgeUp, GlobalGrid().getGridSize(), GlobalXYWnd().getActiveViewType());
}

void Selection_NudgeDown()
{
  UndoableCommand undo("nudgeSelectedDown");
  NudgeSelection(eNudgeDown, GlobalGrid().getGridSize(), GlobalXYWnd().getActiveViewType());
}

void Selection_NudgeLeft()
{
  UndoableCommand undo("nudgeSelectedLeft");
  NudgeSelection(eNudgeLeft, GlobalGrid().getGridSize(), GlobalXYWnd().getActiveViewType());
}

void Selection_NudgeRight()
{
  UndoableCommand undo("nudgeSelectedRight");
  NudgeSelection(eNudgeRight, GlobalGrid().getGridSize(), GlobalXYWnd().getActiveViewType());
}

void ToolChanged() {  
	GlobalEventManager().setToggled("ToggleClipper", GlobalClipper().clipMode());
	GlobalEventManager().setToggled("MouseTranslate", GlobalSelectionSystem().ManipulatorMode() == SelectionSystem::eTranslate);
	GlobalEventManager().setToggled("MouseRotate", GlobalSelectionSystem().ManipulatorMode() == SelectionSystem::eRotate);
	GlobalEventManager().setToggled("MouseScale", GlobalSelectionSystem().ManipulatorMode() == SelectionSystem::eScale);
	GlobalEventManager().setToggled("MouseDrag", GlobalSelectionSystem().ManipulatorMode() == SelectionSystem::eDrag);
}

const char* const c_ResizeMode_status = "QE4 Drag Tool: move and resize objects";

void DragMode()
{
  if(g_currentToolMode == DragMode && g_defaultToolMode != DragMode)
  {
    g_defaultToolMode();
  }
  else
  {
    g_currentToolMode = DragMode;
    g_currentToolModeSupportsComponentEditing = true;

    GlobalClipper().onClipMode(false);

    Sys_Status(c_ResizeMode_status);
    GlobalSelectionSystem().SetManipulatorMode(SelectionSystem::eDrag);
    ToolChanged();
    ModeChangeNotify();
  }
}


const char* const c_TranslateMode_status = "Translate Tool: translate objects and components";

void TranslateMode()
{
  if(g_currentToolMode == TranslateMode && g_defaultToolMode != TranslateMode)
  {
    g_defaultToolMode();
  }
  else
  {
    g_currentToolMode = TranslateMode;
    g_currentToolModeSupportsComponentEditing = true;

    GlobalClipper().onClipMode(false);

    Sys_Status(c_TranslateMode_status);
    GlobalSelectionSystem().SetManipulatorMode(SelectionSystem::eTranslate);
    ToolChanged();
    ModeChangeNotify();
  }
}

const char* const c_RotateMode_status = "Rotate Tool: rotate objects and components";

void RotateMode()
{
  if(g_currentToolMode == RotateMode && g_defaultToolMode != RotateMode)
  {
    g_defaultToolMode();
  }
  else
  {
    g_currentToolMode = RotateMode;
    g_currentToolModeSupportsComponentEditing = true;

    GlobalClipper().onClipMode(false);

    Sys_Status(c_RotateMode_status);
    GlobalSelectionSystem().SetManipulatorMode(SelectionSystem::eRotate);
    ToolChanged();
    ModeChangeNotify();
  }
}

const char* const c_ScaleMode_status = "Scale Tool: scale objects and components";

void ScaleMode()
{
  if(g_currentToolMode == ScaleMode && g_defaultToolMode != ScaleMode)
  {
    g_defaultToolMode();
  }
  else
  {
    g_currentToolMode = ScaleMode;
    g_currentToolModeSupportsComponentEditing = true;

    GlobalClipper().onClipMode(false);

    Sys_Status(c_ScaleMode_status);
    GlobalSelectionSystem().SetManipulatorMode(SelectionSystem::eScale);
    ToolChanged();
    ModeChangeNotify();
  }
}


const char* const c_ClipperMode_status = "Clipper Tool: apply clip planes to objects";


void ClipperMode() {
	if (g_currentToolMode == ClipperMode && g_defaultToolMode != ClipperMode) {
		g_defaultToolMode();
	}
	else {
		g_currentToolMode = ClipperMode;
		g_currentToolModeSupportsComponentEditing = false;

		SelectionSystem_DefaultMode();

		GlobalClipper().onClipMode(true);

		Sys_Status(c_ClipperMode_status);
		GlobalSelectionSystem().SetManipulatorMode(SelectionSystem::eClip);
		ToolChanged();
		ModeChangeNotify();
	}
}

void Texdef_Rotate(float angle)
{
  StringOutputStream command;
  command << "brushRotateTexture -angle " << angle;
  UndoableCommand undo(command.c_str());
  Select_RotateTexture(angle);
}

void Texdef_RotateClockwise()
{
  Texdef_Rotate(static_cast<float>(fabs(g_si_globals.rotate)));
}

void Texdef_RotateAntiClockwise()
{
  Texdef_Rotate(static_cast<float>(-fabs(g_si_globals.rotate)));
}

void Texdef_Scale(float x, float y)
{
  StringOutputStream command;
  command << "brushScaleTexture -x " << x << " -y " << y;
  UndoableCommand undo(command.c_str());
  Select_ScaleTexture(x, y);
}

void Texdef_ScaleUp()
{
  Texdef_Scale(0, g_si_globals.scale[1]);
}

void Texdef_ScaleDown()
{
  Texdef_Scale(0, -g_si_globals.scale[1]);
}

void Texdef_ScaleLeft()
{
  Texdef_Scale(-g_si_globals.scale[0],0);
}

void Texdef_ScaleRight()
{
  Texdef_Scale(g_si_globals.scale[0],0);
}

void Texdef_Shift(float x, float y)
{
  StringOutputStream command;
  command << "brushShiftTexture -x " << x << " -y " << y;
  UndoableCommand undo(command.c_str());
  Select_ShiftTexture(x, y);
}

void Texdef_ShiftLeft()
{
  Texdef_Shift(-g_si_globals.shift[0], 0);
}

void Texdef_ShiftRight()
{
  Texdef_Shift(g_si_globals.shift[0], 0);
}

void Texdef_ShiftUp()
{
  Texdef_Shift(0, g_si_globals.shift[1]);
}

void Texdef_ShiftDown()
{
  Texdef_Shift(0, -g_si_globals.shift[1]);
}



class SnappableSnapToGridSelected : public scene::Graph::Walker
{
  float m_snap;
public:
  SnappableSnapToGridSelected(float snap)
    : m_snap(snap)
  {
  }
  bool pre(const scene::Path& path, scene::Instance& instance) const
  {
    if(path.top().get().visible())
    {
      Snappable* snappable = Node_getSnappable(path.top());
      if(snappable != 0
        && Instance_getSelectable(instance)->isSelected())
      {
        snappable->snapto(m_snap);
      }
    }
    return true;
  }
};

void Scene_SnapToGrid_Selected(scene::Graph& graph, float snap)
{
  graph.traverse(SnappableSnapToGridSelected(snap));
}

/* greebo: This is the visitor class to snap all components of a selected instance to the grid
 * While visiting all the instances, it checks if the instance derives from ComponentSnappable 
 */
class ComponentSnappableSnapToGridSelected : public scene::Graph::Walker {
	float m_snap;
public:
	// Constructor: expects the grid size that should be snapped to
	ComponentSnappableSnapToGridSelected(float snap): m_snap(snap) {}
  
	bool pre(const scene::Path& path, scene::Instance& instance) const {
	    if (path.top().get().visible()) {
	    	// Check if the visited instance is componentSnappable
			ComponentSnappable* componentSnappable = Instance_getComponentSnappable(instance);
			
			// Call the snapComponents() method if the instance is also a _selected_ Selectable 
			if (componentSnappable != 0  && Instance_getSelectable(instance)->isSelected()) {
				componentSnappable->snapComponents(m_snap);
			}
	    }
		return true;
	}
}; // ComponentSnappableSnapToGridSelected

void Scene_SnapToGrid_Component_Selected(scene::Graph& graph, float snap)
{
  graph.traverse(ComponentSnappableSnapToGridSelected(snap));
}

void Selection_SnapToGrid()
{
  StringOutputStream command;
  command << "snapSelected -grid " << GlobalGrid().getGridSize();
  UndoableCommand undo(command.c_str());

  if(GlobalSelectionSystem().Mode() == SelectionSystem::eComponent)
  {
    Scene_SnapToGrid_Component_Selected(GlobalSceneGraph(), GlobalGrid().getGridSize());
  }
  else
  {
    Scene_SnapToGrid_Selected(GlobalSceneGraph(), GlobalGrid().getGridSize());
  }
}


static gint qe_every_second(gpointer data)
{
  GdkModifierType mask;

  gdk_window_get_pointer (0, 0, 0, &mask);

  if ((mask & (GDK_BUTTON1_MASK|GDK_BUTTON2_MASK|GDK_BUTTON3_MASK)) == 0)
  {
    QE_CheckAutoSave();
  }

  return TRUE;
}

guint s_qe_every_second_id = 0;

void EverySecondTimer_enable()
{
  if(s_qe_every_second_id == 0)
  {
    s_qe_every_second_id = gtk_timeout_add(1000, qe_every_second, 0);
  }
}

void EverySecondTimer_disable()
{
  if(s_qe_every_second_id != 0)
  {
    gtk_timeout_remove(s_qe_every_second_id);
    s_qe_every_second_id = 0;
  }
}

gint window_realize_remove_decoration(GtkWidget* widget, gpointer data)
{
  gdk_window_set_decorations(widget->window, (GdkWMDecoration)(GDK_DECOR_ALL|GDK_DECOR_MENU|GDK_DECOR_MINIMIZE|GDK_DECOR_MAXIMIZE));
  return FALSE;
}

class WaitDialog
{
public:
  GtkWindow* m_window;
  GtkLabel* m_label;
};

WaitDialog create_wait_dialog(const char* title, const char* text)
{
  WaitDialog dialog;

  dialog.m_window = create_floating_window(title, MainFrame_getWindow());
  gtk_window_set_resizable(dialog.m_window, FALSE);
  gtk_container_set_border_width(GTK_CONTAINER(dialog.m_window), 0);
  gtk_window_set_position(dialog.m_window, GTK_WIN_POS_CENTER_ON_PARENT);

  g_signal_connect(G_OBJECT(dialog.m_window), "realize", G_CALLBACK(window_realize_remove_decoration), 0);

  {
    dialog.m_label = GTK_LABEL(gtk_label_new(text));
    gtk_misc_set_alignment(GTK_MISC(dialog.m_label), 0.0, 0.5);
    gtk_label_set_justify(dialog.m_label, GTK_JUSTIFY_LEFT);
    gtk_widget_show(GTK_WIDGET(dialog.m_label));
    gtk_widget_set_size_request(GTK_WIDGET(dialog.m_label), 200, -1);

    gtk_container_add(GTK_CONTAINER(dialog.m_window), GTK_WIDGET(dialog.m_label));
  }
  return dialog;
}

namespace
{
  clock_t g_lastRedrawTime = 0;
  const clock_t c_redrawInterval = clock_t(CLOCKS_PER_SEC / 10);

  bool redrawRequired()
  {
    clock_t currentTime = std::clock();
    if(currentTime - g_lastRedrawTime >= c_redrawInterval)
    {
      g_lastRedrawTime = currentTime;
      return true;
    }
    return false;
  }
}

bool MainFrame_isActiveApp()
{
  //globalOutputStream() << "listing\n";
  GList* list = gtk_window_list_toplevels();
  for(GList* i = list; i != 0; i = g_list_next(i))
  {
    //globalOutputStream() << "toplevel.. ";
    if(gtk_window_is_active(GTK_WINDOW(i->data)))
    {
      //globalOutputStream() << "is active\n";
      return true;
    }
    //globalOutputStream() << "not active\n";
  }
  return false;
}

typedef std::list<CopiedString> StringStack;
StringStack g_wait_stack;
WaitDialog g_wait;

bool ScreenUpdates_Enabled()
{
  return g_wait_stack.empty();
}

void ScreenUpdates_process()
{
  if(redrawRequired() && GTK_WIDGET_VISIBLE(g_wait.m_window))
  {
    process_gui();
  }
}


void ScreenUpdates_Disable(const char* message, const char* title)
{
  if(g_wait_stack.empty())
  {
    EverySecondTimer_disable();

    process_gui();

    bool isActiveApp = MainFrame_isActiveApp();

    g_wait = create_wait_dialog(title, message);
    gtk_grab_add(GTK_WIDGET(g_wait.m_window));

    if(isActiveApp)
    {
      gtk_widget_show(GTK_WIDGET(g_wait.m_window));
      ScreenUpdates_process();
    }
  }
  else if(GTK_WIDGET_VISIBLE(g_wait.m_window))
  {
    gtk_label_set_text(g_wait.m_label, message);
    ScreenUpdates_process();
  }
  g_wait_stack.push_back(message);
}

void ScreenUpdates_Enable()
{
  ASSERT_MESSAGE(!ScreenUpdates_Enabled(), "screen updates already enabled");
  g_wait_stack.pop_back();
  if(g_wait_stack.empty())
  {
    EverySecondTimer_enable();
    //gtk_widget_set_sensitive(GTK_WIDGET(MainFrame_getWindow()), TRUE);

    gtk_grab_remove(GTK_WIDGET(g_wait.m_window));
    destroy_floating_window(g_wait.m_window);
    g_wait.m_window = 0;

    //gtk_window_present(MainFrame_getWindow());
  }
  else if(GTK_WIDGET_VISIBLE(g_wait.m_window))
  {
    gtk_label_set_text(g_wait.m_label, g_wait_stack.back().c_str());
    ScreenUpdates_process();
  }
}



void GlobalCamera_UpdateWindow()
{
  if(g_pParentWnd != 0)
  {
    g_pParentWnd->GetCamWnd()->update();
  }
}

void XY_UpdateAllWindows(MainFrame& mainframe) {
	GlobalXYWnd().updateAllViews();
}

void XY_UpdateAllWindows()
{
  if(g_pParentWnd != 0)
  {
    XY_UpdateAllWindows(*g_pParentWnd);
    
    // Set the World Spawn Colour
	SetWorldspawnColour(ColourSchemes().getColourVector3("default_brush"));
	SetLightVolumeColour(ColourSchemes().getColourVector3("light_volumes"));
  }
}

void UpdateAllWindows()
{
  GlobalCamera_UpdateWindow();
  XY_UpdateAllWindows();
}


void ModeChangeNotify()
{
  SceneChangeNotify();
}

void ClipperChangeNotify()
{
  GlobalCamera_UpdateWindow();
  XY_UpdateAllWindows();
}


LatchedInt g_Layout_viewStyle(MainFrame::eFloating, "Window Layout");
LatchedBool g_Layout_enableDetachableMenus(false, "Detachable Menus");
LatchedBool g_Layout_enablePatchToolbar(true, "Patch Toolbar");
LatchedBool g_Layout_enablePluginToolbar(true, "Plugin Toolbar");



GtkMenuItem* create_file_menu()
{
  // File menu
  GtkMenuItem* file_menu_item = new_sub_menu_item_with_mnemonic("_File");
  GtkMenu* menu = GTK_MENU(gtk_menu_item_get_submenu(file_menu_item));
  if (g_Layout_enableDetachableMenus.m_value)
    menu_tearoff (menu);

	createMenuItemWithMnemonic(menu, "_New Map", "NewMap");
	createSeparatorMenuItem(menu);
	createMenuItemWithMnemonic(menu, "_Open...", "OpenMap");
	createMenuItemWithMnemonic(menu, "_Import...", "ImportMap");
	createMenuItemWithMnemonic(menu, "_Save", "SaveMap");
	createMenuItemWithMnemonic(menu, "Save _as...", "SaveMapAs");
	createMenuItemWithMnemonic(menu, "Save s_elected...", "SaveSelected");
	createSeparatorMenuItem(menu);
	createMenuItemWithMnemonic(menu, "Save re_gion...", "SaveRegion");
	createSeparatorMenuItem(menu);
	createMenuItemWithMnemonic(menu, "_Refresh models", "RefreshReferences");
	createSeparatorMenuItem(menu);
	createMenuItemWithMnemonic(menu, "Pro_ject settings...", "ProjectSettings");
	createSeparatorMenuItem(menu);
	createMenuItemWithMnemonic(menu, "_Pointfile...", "TogglePointfile");
	createSeparatorMenuItem(menu);
	GlobalMRU().constructMenu(menu);
	createSeparatorMenuItem(menu);
	createMenuItemWithMnemonic(menu, "E_xit", "Exit");

  return file_menu_item;
}

GtkMenuItem* create_edit_menu()
{
  // Edit menu
  GtkMenuItem* edit_menu_item = new_sub_menu_item_with_mnemonic("_Edit");
  GtkMenu* menu = GTK_MENU(gtk_menu_item_get_submenu(edit_menu_item));
  if (g_Layout_enableDetachableMenus.m_value)
    menu_tearoff (menu);
  createMenuItemWithMnemonic(menu, "_Undo", "Undo");
  createMenuItemWithMnemonic(menu, "_Redo", "Redo");
  createSeparatorMenuItem(menu);
  createMenuItemWithMnemonic(menu, "_Copy", "Copy");
  createMenuItemWithMnemonic(menu, "_Paste", "Paste");
  createMenuItemWithMnemonic(menu, "P_aste To Camera", "PasteToCamera");
  createSeparatorMenuItem(menu);
  createMenuItemWithMnemonic(menu, "_Duplicate", "CloneSelection");
  createMenuItemWithMnemonic(menu, "D_elete", "DeleteSelection");
  createSeparatorMenuItem(menu);
  createMenuItemWithMnemonic(menu, "Pa_rent", "ParentSelection");
  createSeparatorMenuItem(menu);
  createMenuItemWithMnemonic(menu, "C_lear Selection", "UnSelectSelection");
  createMenuItemWithMnemonic(menu, "_Invert Selection", "InvertSelection");
  createMenuItemWithMnemonic(menu, "Select i_nside", "SelectInside");
  createMenuItemWithMnemonic(menu, "Select _touching", "SelectTouching");

  //GtkMenu* convert_menu = create_sub_menu_with_mnemonic(menu, "E_xpand Selection");
  createMenuItemWithMnemonic(menu, "Expand Selection to Whole _Entities", "ExpandSelectionToEntities");

  createSeparatorMenuItem(menu);
  createMenuItemWithMnemonic(menu, "Export Selected Brushes to _OBJ", "BrushExportOBJ");

  createSeparatorMenuItem(menu);
  createMenuItemWithMnemonic(menu, "Pre_ferences...", "Preferences");

  return edit_menu_item;
}

GtkWidget* g_toggle_z_item = 0;
GtkWidget* g_toggle_console_item = 0;
GtkWidget* g_toggle_entity_item = 0;
GtkWidget* g_toggle_entitylist_item = 0;

GtkMenuItem* create_view_menu(MainFrame::EViewStyle style)
{
  // View menu
  GtkMenuItem* view_menu_item = new_sub_menu_item_with_mnemonic("_View");
  GtkMenu* menu = GTK_MENU(gtk_menu_item_get_submenu(view_menu_item));
  if (g_Layout_enableDetachableMenus.m_value)
    menu_tearoff (menu);

	createMenuItemWithMnemonic(menu, "New XY View", "NewOrthoView");

  if(style == MainFrame::eFloating)
  {
    createCheckMenuItemWithMnemonic(menu, "Camera View", "ToggleCamera");
  }
  if(style == MainFrame::eFloating || style == MainFrame::eSplit)
  {
    createMenuItemWithMnemonic(menu, "Console View", "ToggleConsole");
    createMenuItemWithMnemonic(menu, "Texture Browser", "ViewTextures");
    createMenuItemWithMnemonic(menu, "Entity Inspector", "ToggleEntityInspector");
  }
  else
  {
    createMenuItemWithMnemonic(menu, "Entity Inspector", "ViewEntityInfo");
  }
	// Light inspector
	createMenuItemWithMnemonic(menu, "_Light Inspector", "ToggleLightInspector");
								   
  createMenuItemWithMnemonic(menu, "_Surface Inspector", "SurfaceInspector");
  createMenuItemWithMnemonic(menu, "Entity List", "EntityList");

  menu_separator(menu);
  {
    GtkMenu* camera_menu = create_sub_menu_with_mnemonic (menu, "Camera");
    createMenuItemWithMnemonic(camera_menu, "_Center", "CenterView");
    createMenuItemWithMnemonic(camera_menu, "_Up Floor", "UpFloor");
    createMenuItemWithMnemonic(camera_menu, "_Down Floor", "DownFloor");
    menu_separator(camera_menu);
    createMenuItemWithMnemonic(camera_menu, "Far Clip Plane In", "CubicClipZoomIn");
    createMenuItemWithMnemonic(camera_menu, "Far Clip Plane Out", "CubicClipZoomOut");
    menu_separator(camera_menu);
    createMenuItemWithMnemonic(camera_menu, "Next leak spot", "NextLeakSpot");
    createMenuItemWithMnemonic(camera_menu, "Previous leak spot", "PrevLeakSpot");
    menu_separator(camera_menu);
    createMenuItemWithMnemonic(camera_menu, "Look Through Selected", "LookThroughSelected");
    createMenuItemWithMnemonic(camera_menu, "Look Through Camera", "LookThroughCamera");
  }
  {
    GtkMenu* orthographic_menu = create_sub_menu_with_mnemonic(menu, "Orthographic");
    if(style == MainFrame::eRegular || style == MainFrame::eRegularLeft || style == MainFrame::eFloating)
    {
      createMenuItemWithMnemonic(orthographic_menu, "_Next (XY, YZ, XY)", "NextView");
      createMenuItemWithMnemonic(orthographic_menu, "XY (Top)", "ViewTop");
      createMenuItemWithMnemonic(orthographic_menu, "YZ", "ViewSide");
      createMenuItemWithMnemonic(orthographic_menu, "XZ", "ViewFront");
      menu_separator(orthographic_menu);
    }

    createMenuItemWithMnemonic(orthographic_menu, "_XY 100%", "Zoom100");
    createMenuItemWithMnemonic(orthographic_menu, "XY Zoom _In", "ZoomIn");
    createMenuItemWithMnemonic(orthographic_menu, "XY Zoom _Out", "ZoomOut");
  }

  menu_separator(menu);

  {
    GtkMenu* menu_in_menu = create_sub_menu_with_mnemonic (menu, "Show");
    createCheckMenuItemWithMnemonic(menu_in_menu, "Show _Angles", "ShowAngles");
    createCheckMenuItemWithMnemonic(menu_in_menu, "Show _Names", "ShowNames");
    createCheckMenuItemWithMnemonic(menu_in_menu, "Show Blocks", "ShowBlocks");
    createCheckMenuItemWithMnemonic(menu_in_menu, "Show C_oordinates", "ShowCoordinates");
    createCheckMenuItemWithMnemonic(menu_in_menu, "Show Window Outline", "ShowWindowOutline");
    createCheckMenuItemWithMnemonic(menu_in_menu, "Show Axes", "ShowAxes");
    createCheckMenuItemWithMnemonic(menu_in_menu, "Show Workzone", "ShowWorkzone");
    createCheckMenuItemWithMnemonic(menu_in_menu, "Show Size Info", "ToggleShowSizeInfo");
  }

  menu_separator(menu);
  {
    GtkMenu* menu_in_menu = create_sub_menu_with_mnemonic (menu, "Hide/Show");
    createMenuItemWithMnemonic(menu_in_menu, "Hide Selected", "HideSelected");
    createMenuItemWithMnemonic(menu_in_menu, "Show Hidden", "ShowHidden");
  }
  {
    GtkMenu* menu_in_menu = create_sub_menu_with_mnemonic (menu, "Region");
    createMenuItemWithMnemonic(menu_in_menu, "_Off", "RegionOff");
    createMenuItemWithMnemonic(menu_in_menu, "_Set XY", "RegionSetXY");
    createMenuItemWithMnemonic(menu_in_menu, "Set _Brush", "RegionSetBrush");
    createMenuItemWithMnemonic(menu_in_menu, "Set Se_lected Brushes", "RegionSetSelection");
  }
  menu_separator(menu);
  createMenuItemWithMnemonic(menu, "Colours...", "EditColourScheme");
  
  return view_menu_item;
}

GtkMenuItem* create_selection_menu()
{
  // Selection menu
  GtkMenuItem* selection_menu_item = new_sub_menu_item_with_mnemonic("_Modify");
  GtkMenu* menu = GTK_MENU(gtk_menu_item_get_submenu(selection_menu_item));
  if (g_Layout_enableDetachableMenus.m_value)
    menu_tearoff (menu);

  {
    GtkMenu* menu_in_menu = create_sub_menu_with_mnemonic (menu, "Components");
    createCheckMenuItemWithMnemonic(menu_in_menu, "_Edges", "DragEdges");
    createCheckMenuItemWithMnemonic(menu_in_menu, "_Vertices", "DragVertices");
    createCheckMenuItemWithMnemonic(menu_in_menu, "_Faces", "DragFaces");
  }

  menu_separator(menu);

  {
    GtkMenu* menu_in_menu = create_sub_menu_with_mnemonic(menu, "Nudge");
    createMenuItemWithMnemonic(menu_in_menu, "Nudge Left", "SelectNudgeLeft");
    createMenuItemWithMnemonic(menu_in_menu, "Nudge Right", "SelectNudgeRight");
    createMenuItemWithMnemonic(menu_in_menu, "Nudge Up", "SelectNudgeUp");
    createMenuItemWithMnemonic(menu_in_menu, "Nudge Down", "SelectNudgeDown");
  }
  {
    GtkMenu* menu_in_menu = create_sub_menu_with_mnemonic (menu, "Rotate");
    createMenuItemWithMnemonic(menu_in_menu, "Rotate X", "RotateSelectionX");
    createMenuItemWithMnemonic(menu_in_menu, "Rotate Y", "RotateSelectionY");
    createMenuItemWithMnemonic(menu_in_menu, "Rotate Z", "RotateSelectionZ");
  }
  {
    GtkMenu* menu_in_menu = create_sub_menu_with_mnemonic (menu, "Flip");
    createMenuItemWithMnemonic(menu_in_menu, "Flip _X", "MirrorSelectionX");
    createMenuItemWithMnemonic(menu_in_menu, "Flip _Y", "MirrorSelectionY");
    createMenuItemWithMnemonic(menu_in_menu, "Flip _Z", "MirrorSelectionZ");
  }
  menu_separator(menu);
  createMenuItemWithMnemonic(menu, "Arbitrary rotation...", "ArbitraryRotation");
  createMenuItemWithMnemonic(menu, "Arbitrary scale...", "ArbitraryScale");

  return selection_menu_item;
}

GtkMenuItem* create_grid_menu()
{
  // Grid menu
  GtkMenuItem* grid_menu_item = new_sub_menu_item_with_mnemonic("_Grid");
  GtkMenu* menu = GTK_MENU(gtk_menu_item_get_submenu(grid_menu_item));
  if (g_Layout_enableDetachableMenus.m_value)
    menu_tearoff (menu);

	createCheckMenuItemWithMnemonic(menu, "Grid0.125", "SetGrid0.125");
	createCheckMenuItemWithMnemonic(menu, "Grid0.25", "SetGrid0.25");
	createCheckMenuItemWithMnemonic(menu, "Grid0.5", "SetGrid0.5");
	createCheckMenuItemWithMnemonic(menu, "Grid1", "SetGrid1");
	createCheckMenuItemWithMnemonic(menu, "Grid2", "SetGrid2");
	createCheckMenuItemWithMnemonic(menu, "Grid4", "SetGrid4");
	createCheckMenuItemWithMnemonic(menu, "Grid8", "SetGrid8");
	createCheckMenuItemWithMnemonic(menu, "Grid16", "SetGrid16");
	createCheckMenuItemWithMnemonic(menu, "Grid32", "SetGrid32");
	createCheckMenuItemWithMnemonic(menu, "Grid64", "SetGrid64");
	createCheckMenuItemWithMnemonic(menu, "Grid128", "SetGrid128");
	createCheckMenuItemWithMnemonic(menu, "Grid256", "SetGrid256");

  return grid_menu_item;
}

void RefreshShaders()
{
  ScopeDisableScreenUpdates disableScreenUpdates("Processing...", "Loading Shaders");
  GlobalShaderSystem().refresh();
  UpdateAllWindows();
}

void CallBrushExportOBJ() {
	if (GlobalSelectionSystem().countSelected() != 0) {
		export_selected(MainFrame_getWindow());
	}
	else {
		gtk_MessageBox(GTK_WIDGET(MainFrame_getWindow()), "No Brushes Selected!", "Error", eMB_OK, eMB_ICONERROR);
	}
}

GtkMenuItem* create_misc_menu()
{
  // Misc menu
  GtkMenuItem* misc_menu_item = new_sub_menu_item_with_mnemonic("M_isc");
  GtkMenu* menu = GTK_MENU(gtk_menu_item_get_submenu(misc_menu_item));
  if (g_Layout_enableDetachableMenus.m_value)
    menu_tearoff (menu);
  
  
#if 0
  createMenuItemWithMnemonic(menu, "_Benchmark", FreeCaller<GlobalCamera_Benchmark>());
#endif
  
  createMenuItemWithMnemonic(menu, "Find brush...", "FindBrush");
  createMenuItemWithMnemonic(menu, "Map Info...", "MapInfo");

  return misc_menu_item;
}

GtkMenuItem* create_entity_menu()
{
  // Brush menu
  GtkMenuItem* entity_menu_item = new_sub_menu_item_with_mnemonic("E_ntity");
  GtkMenu* menu = GTK_MENU(gtk_menu_item_get_submenu(entity_menu_item));
  if (g_Layout_enableDetachableMenus.m_value)
    menu_tearoff (menu);

  Entity_constructMenu(menu);

  return entity_menu_item;
}

GtkMenuItem* create_brush_menu()
{
  // Brush menu
  GtkMenuItem* brush_menu_item = new_sub_menu_item_with_mnemonic("B_rush");
  GtkMenu* menu = GTK_MENU(gtk_menu_item_get_submenu(brush_menu_item));
  if (g_Layout_enableDetachableMenus.m_value)
    menu_tearoff (menu);

  Brush_constructMenu(menu);

  return brush_menu_item;
}

GtkMenuItem* create_patch_menu()
{
  // Curve menu
  GtkMenuItem* patch_menu_item = new_sub_menu_item_with_mnemonic("_Curve");
  GtkMenu* menu = GTK_MENU(gtk_menu_item_get_submenu(patch_menu_item));
  if (g_Layout_enableDetachableMenus.m_value)
  {
    menu_tearoff(menu);
  }

  Patch_constructMenu(menu);

  return patch_menu_item;
}

GtkMenuItem* create_help_menu()
{
  // Help menu
  GtkMenuItem* help_menu_item = new_sub_menu_item_with_mnemonic("_Help");
  GtkMenu* menu = GTK_MENU(gtk_menu_item_get_submenu(help_menu_item));
  if (g_Layout_enableDetachableMenus.m_value)
    menu_tearoff (menu);

  createMenuItemWithMnemonic(menu, "Shortcuts list", "ShowCommandList");
  createMenuItemWithMnemonic(menu, "_About", "About");

  return help_menu_item;
}

GtkMenuBar* create_main_menu(MainFrame::EViewStyle style)
{
  GtkMenuBar* menu_bar = GTK_MENU_BAR(gtk_menu_bar_new());
  gtk_widget_show(GTK_WIDGET(menu_bar));

  gtk_container_add(GTK_CONTAINER(menu_bar), GTK_WIDGET(create_file_menu()));
  gtk_container_add(GTK_CONTAINER(menu_bar), GTK_WIDGET(create_edit_menu()));
  gtk_container_add(GTK_CONTAINER(menu_bar), GTK_WIDGET(create_view_menu(style)));
  gtk_container_add(GTK_CONTAINER(menu_bar), GTK_WIDGET(create_selection_menu()));
  gtk_container_add(GTK_CONTAINER(menu_bar), GTK_WIDGET(create_grid_menu()));
  
  // Filters menu
  ui::FiltersMenu filtersMenu;
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), filtersMenu);

  gtk_container_add(GTK_CONTAINER(menu_bar), GTK_WIDGET(create_misc_menu()));
  gtk_container_add(GTK_CONTAINER(menu_bar), GTK_WIDGET(create_entity_menu()));
  gtk_container_add(GTK_CONTAINER(menu_bar), GTK_WIDGET(create_brush_menu()));
  gtk_container_add(GTK_CONTAINER(menu_bar), GTK_WIDGET(create_patch_menu()));
  gtk_container_add(GTK_CONTAINER(menu_bar), GTK_WIDGET(create_plugins_menu()));
  gtk_container_add(GTK_CONTAINER(menu_bar), GTK_WIDGET(create_help_menu()));

  return menu_bar;
}

GtkWidget* create_main_statusbar(GtkWidget *pStatusLabel[c_count_status])
{
  GtkTable* table = GTK_TABLE(gtk_table_new(1, c_count_status, FALSE));
  gtk_widget_show(GTK_WIDGET(table));

  {
    GtkLabel* label = GTK_LABEL(gtk_label_new ("Label"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(label), 4, 2);
    gtk_widget_show(GTK_WIDGET(label));
    gtk_table_attach_defaults(table, GTK_WIDGET(label), 0, 1, 0, 1);
    pStatusLabel[c_command_status] = GTK_WIDGET(label);
  }

  for(int i = 1; i < c_count_status; ++i)
  {
    GtkFrame* frame = GTK_FRAME(gtk_frame_new(0));
    gtk_widget_show(GTK_WIDGET(frame));
    gtk_table_attach_defaults(table, GTK_WIDGET(frame), i, i + 1, 0, 1);
    gtk_frame_set_shadow_type(frame, GTK_SHADOW_IN);

    GtkLabel* label = GTK_LABEL(gtk_label_new ("Label"));
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_misc_set_padding(GTK_MISC(label), 4, 2);
    gtk_widget_show(GTK_WIDGET(label));
    gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(label));
    pStatusLabel[i] = GTK_WIDGET(label);
  }

  return GTK_WIDGET(table);
}

#if 0


WidgetFocusPrinter g_mainframeWidgetFocusPrinter("mainframe");

class WindowFocusPrinter
{
  const char* m_name;

  static gboolean frame_event(GtkWidget *widget, GdkEvent* event, WindowFocusPrinter* self)
  {
    globalOutputStream() << self->m_name << " frame_event\n";
    return FALSE;
  }
  static gboolean keys_changed(GtkWidget *widget, WindowFocusPrinter* self)
  {
    globalOutputStream() << self->m_name << " keys_changed\n";
    return FALSE;
  }
  static gboolean notify(GtkWindow* window, gpointer dummy, WindowFocusPrinter* self)
  {
    if(gtk_window_is_active(window))
    {
      globalOutputStream() << self->m_name << " takes toplevel focus\n";
    }
    else
    {
      globalOutputStream() << self->m_name << " loses toplevel focus\n";
    }
    return FALSE;
  }
public:
  WindowFocusPrinter(const char* name) : m_name(name)
  {
  }
  void connect(GtkWindow* toplevel_window)
  {
    g_signal_connect(G_OBJECT(toplevel_window), "notify::has_toplevel_focus", G_CALLBACK(notify), this);
    g_signal_connect(G_OBJECT(toplevel_window), "notify::is_active", G_CALLBACK(notify), this);
    g_signal_connect(G_OBJECT(toplevel_window), "keys_changed", G_CALLBACK(keys_changed), this);
    g_signal_connect(G_OBJECT(toplevel_window), "frame_event", G_CALLBACK(frame_event), this);
  }
};

WindowFocusPrinter g_mainframeFocusPrinter("mainframe");

#endif

class MainWindowActive
{
  static gboolean notify(GtkWindow* window, gpointer dummy, MainWindowActive* self)
  {
    if(g_wait.m_window != 0 && gtk_window_is_active(window) && !GTK_WIDGET_VISIBLE(g_wait.m_window))
    {
      gtk_widget_show(GTK_WIDGET(g_wait.m_window));
    }
    
    return FALSE;
  }
public:
  void connect(GtkWindow* toplevel_window)
  {
    g_signal_connect(G_OBJECT(toplevel_window), "notify::is-active", G_CALLBACK(notify), this);
  }
};

MainWindowActive g_MainWindowActive;

// =============================================================================
// MainFrame class

MainFrame* g_pParentWnd = 0;

GtkWindow* MainFrame_getWindow()
{
  if(g_pParentWnd == 0)
  {
    return 0;
  }
  return g_pParentWnd->m_window;
}

std::vector<GtkWidget*> g_floating_windows;

MainFrame::MainFrame() : m_window(0), m_idleRedrawStatusText(RedrawStatusTextCaller(*this))
{
  m_pCamWnd = 0;

  for (int n = 0;n < c_count_status;n++)
  {
    m_pStatusLabel[n] = 0;
  }

  m_bSleeping = false;

  Create();
}

MainFrame::~MainFrame()
{
  SaveWindowInfo();

  gtk_widget_hide(GTK_WIDGET(m_window));

  Shutdown();

  for(std::vector<GtkWidget*>::iterator i = g_floating_windows.begin(); i != g_floating_windows.end(); ++i)
  {
    gtk_widget_destroy(*i);
  }  

  gtk_widget_destroy(GTK_WIDGET(m_window));
}

void MainFrame::ReleaseContexts()
{

}

void MainFrame::CreateContexts()
{

}

#ifdef _DEBUG
//#define DBG_SLEEP
#endif

void MainFrame::OnSleep()
{
#if 0
  m_bSleeping ^= 1;
  if (m_bSleeping)
  {
    // useful when trying to debug crashes in the sleep code
    globalOutputStream() << "Going into sleep mode..\n";

    globalOutputStream() << "Dispatching sleep msg...";
    DispatchRadiantMsg (RADIANT_SLEEP);
    globalOutputStream() << "Done.\n";

    gtk_window_iconify(m_window);
    GlobalSelectionSystem().setSelectedAll(false);

    GlobalShaderCache().unrealise();
    Shaders_Free();
    GlobalOpenGL_debugAssertNoErrors();
    ScreenUpdates_Disable();

    // release contexts
    globalOutputStream() << "Releasing contexts...";
    ReleaseContexts();
    globalOutputStream() << "Done.\n";
  }
  else
  {
    globalOutputStream() << "Waking up\n";

    gtk_window_deiconify(m_window);

    // create contexts
    globalOutputStream() << "Creating contexts...";
    CreateContexts();
    globalOutputStream() << "Done.\n";

    globalOutputStream() << "Making current on camera...";
    m_pCamWnd->MakeCurrent();
    globalOutputStream() << "Done.\n";

    globalOutputStream() << "Reloading shaders...";
    Shaders_Load();
    GlobalShaderCache().realise();
    globalOutputStream() << "Done.\n";

    ScreenUpdates_Enable();

    globalOutputStream() << "Dispatching wake msg...";
    DispatchRadiantMsg (RADIANT_WAKEUP);
    globalOutputStream() << "Done\n";
  }
#endif
}

// Create and show the splash screen.

static const char *SPLASH_FILENAME = "darksplash.png";

GtkWindow* create_splash()
{
  GtkWindow* window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
  gtk_window_set_decorated(window, FALSE);
  gtk_window_set_resizable(window, FALSE);
  gtk_window_set_modal(window, TRUE);
  gtk_window_set_default_size(window, -1, -1);
  gtk_window_set_position(window, GTK_WIN_POS_CENTER);
  gtk_container_set_border_width(GTK_CONTAINER(window), 0);

  GtkImage* image = new_local_image(SPLASH_FILENAME);
  gtk_widget_show(GTK_WIDGET(image));
  gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(image));

  gtk_widget_set_size_request(GTK_WIDGET(window), -1, -1);
  gtk_widget_show(GTK_WIDGET(window));

  return window;
}

static GtkWindow *splash_screen = 0;

void show_splash()
{
  splash_screen = create_splash();

  process_gui();
}

void hide_splash()
{
  gtk_widget_destroy(GTK_WIDGET(splash_screen));
}

static gint mainframe_delete (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  if(ConfirmModified("Exit Radiant"))
  {
    gtk_main_quit();
  }

  return TRUE;
}

/* Construct the main Radiant window
 */

void MainFrame::Create()
{
  GtkWindow* window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
  
  // Tell the XYManager which window the xyviews should be transient for
  GlobalXYWnd().setGlobalParentWindow(window);

  GlobalWindowObservers_connectTopLevel(window);

  gtk_window_set_transient_for(splash_screen, window);

#if !defined(WIN32)
  {
    GdkPixbuf* pixbuf = pixbuf_new_from_file_with_mask("icon.bmp");
    if(pixbuf != 0)
    {
      gtk_window_set_icon(window, pixbuf);
      gdk_pixbuf_unref(pixbuf);
    }
  }
#endif

  gtk_widget_add_events(GTK_WIDGET(window), GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_FOCUS_CHANGE_MASK);
  g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(mainframe_delete), this);

  m_position_tracker.connect(window);

#if 0
  g_mainframeWidgetFocusPrinter.connect(window);
  g_mainframeFocusPrinter.connect(window);
#endif

    g_MainWindowActive.connect(window);
    
    GetPlugInMgr().Init(GTK_WIDGET(window));
    
    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show(vbox);
    
    GlobalEventManager().connect(GTK_OBJECT(window));
    GlobalEventManager().connectAccelGroup(GTK_WINDOW(window));
    
    m_nCurrentStyle = (EViewStyle) g_Layout_viewStyle.m_value;
    
    // Create and add main menu    
    GtkMenuBar *main_menu = create_main_menu(CurrentStyle());
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(main_menu), FALSE, FALSE, 0);
    
    // Instantiate the ToolbarCreator and retrieve the view toolbar widget 
	ui::ToolbarCreator toolbarCreator;
	
	GtkToolbar* viewToolbar = toolbarCreator.getToolbar("view");
	if (viewToolbar != NULL) {
		// Pack it into the main window
		gtk_widget_show(GTK_WIDGET(viewToolbar));
		gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(viewToolbar), FALSE, FALSE, 0);
	}
	
    GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
    
    // Get the edit toolbar widget 
	GtkToolbar* editToolbar = toolbarCreator.getToolbar("edit");
	if (editToolbar != NULL) {
		// Pack it into the main window
		gtk_widget_show(GTK_WIDGET(editToolbar));
		gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(editToolbar), FALSE, FALSE, 0);
	}
    
    // Create and pack main statusbar 
    GtkWidget *main_statusbar = create_main_statusbar(m_pStatusLabel);
    gtk_box_pack_end(GTK_BOX(vbox), main_statusbar, FALSE, TRUE, 2);

	/* Construct the Group Dialog. This is the tabbed window that contains
     * a number of pages - usually Entities, Textures and possibly Console.
     */

    GroupDialog_constructWindow(window);

    // Add entity inspector widget
    g_page_entity = GroupDialog_addPage("Entities", 
    									"cmenu_add_entity.png", 
    									ui::EntityInspector::getInstance().getWidget(), 
    									RawStringExportCaller("Entities"));

	// Add the Media Browser page
	GroupDialog_addPage("Media", 
						"folder16.png", 
						ui::MediaBrowser::getInstance().getWidget(), 
						RawStringExportCaller("Media"));

    // Add the console widget if using floating window mode, otherwise the
    // console is placed in the bottom-most split pane.
    if (FloatingGroupDialog()) {
        g_page_console = GroupDialog_addPage("Console", 
        									 "iconConsole16.png",
        									 Console_constructWindow(GroupDialog_getWindow()), 
        									 RawStringExportCaller("Console"));
    }

#ifdef WIN32
  if( g_multimon_globals.m_bStartOnPrimMon )
  {
    PositionWindowOnPrimaryScreen(g_layout_globals.m_position);
	window_set_position(window, g_layout_globals.m_position);
  }
  else
#endif
  if(g_layout_globals.nState & GDK_WINDOW_STATE_MAXIMIZED)
  {
    gtk_window_maximize(window);
    WindowPosition default_position(-1, -1, 640, 480);
    window_set_position(window, default_position);
  }
  else
  {
    window_set_position(window, g_layout_globals.m_position);
  }

  m_window = window;

  gtk_widget_show(GTK_WIDGET(window));

  if (CurrentStyle() == eRegular || CurrentStyle() == eRegularLeft)
  {
    {
      GtkWidget* vsplit = gtk_vpaned_new();
      m_vSplit = vsplit;
      gtk_box_pack_start(GTK_BOX(hbox), vsplit, TRUE, TRUE, 0);
      gtk_widget_show (vsplit);

      // console
      GtkWidget* console_window = Console_constructWindow(window);
      gtk_paned_pack2(GTK_PANED(vsplit), console_window, FALSE, TRUE);

      {
        GtkWidget* hsplit = gtk_hpaned_new();
        gtk_widget_show (hsplit);
        m_hSplit = hsplit;
        gtk_paned_add1(GTK_PANED(vsplit), hsplit);

		// Allocate a new OrthoView and set its ViewType to XY
		XYWnd* xyWnd = GlobalXYWnd().createXY();
        xyWnd->setViewType(XY);
        
        // Create a framed window out of the view's internal widget
        GtkWidget* xy_window = GTK_WIDGET(create_framed_widget(xyWnd->getWidget()));

        {
          GtkWidget* vsplit2 = gtk_vpaned_new();
          gtk_widget_show(vsplit2);
          m_vSplit2 = vsplit2;

          if (CurrentStyle() == eRegular)
          {
            gtk_paned_add1(GTK_PANED(hsplit), xy_window);
            gtk_paned_add2(GTK_PANED(hsplit), vsplit2);
          }
          else
          {
            gtk_paned_add1(GTK_PANED(hsplit), vsplit2);
            gtk_paned_add2(GTK_PANED(hsplit), xy_window);
          }


          // camera
          m_pCamWnd = GlobalCamera().newCamWnd();
          GlobalCamera().setCamWnd(m_pCamWnd);
          GlobalCamera().setParent(m_pCamWnd, window);
          GtkFrame* camera_window = create_framed_widget(m_pCamWnd->getWidget());

          gtk_paned_add1(GTK_PANED(vsplit2), GTK_WIDGET(camera_window));

          // textures
          GtkFrame* texture_window = create_framed_widget(TextureBrowser_constructWindow(window));

          gtk_paned_add2(GTK_PANED(vsplit2), GTK_WIDGET(texture_window));
         
        }
      }
    }

    gtk_paned_set_position(GTK_PANED(m_vSplit), g_layout_globals.nXYHeight);

    if (CurrentStyle() == eRegular)
    {
      gtk_paned_set_position(GTK_PANED(m_hSplit), g_layout_globals.nXYWidth);
    }
    else
    {
      gtk_paned_set_position(GTK_PANED(m_hSplit), g_layout_globals.nCamWidth);
    }

    gtk_paned_set_position(GTK_PANED(m_vSplit2), g_layout_globals.nCamHeight);
  }
  else if (CurrentStyle() == eFloating)
  {
    {
      GtkWindow* window = create_persistent_floating_window("Camera", m_window);
	  GlobalEventManager().connectAccelGroup(window);
      GlobalCamera().restoreCamWndState(window);
      
      gtk_widget_show(GTK_WIDGET(window));

      m_pCamWnd = GlobalCamera().newCamWnd();
      GlobalCamera().setCamWnd(m_pCamWnd);

      {
        GtkFrame* frame = create_framed_widget(m_pCamWnd->getWidget());
        gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(frame));
      }
      GlobalCamera().setParent(m_pCamWnd, window);

      g_floating_windows.push_back(GTK_WIDGET(window));
    }

   	{
      GtkFrame* frame = create_framed_widget(TextureBrowser_constructWindow(GroupDialog_getWindow()));
      g_page_textures = GroupDialog_addPage("Textures", 
      										"icon_texture.png",
      										GTK_WIDGET(frame), 
      										TextureBrowserExportTitleCaller());
    }

    GroupDialog_show();
  }
  else // 4 way
  {
    m_pCamWnd = GlobalCamera().newCamWnd();
    GlobalCamera().setCamWnd(m_pCamWnd);
    GlobalCamera().setParent(m_pCamWnd, window);

    GtkWidget* camera = m_pCamWnd->getWidget();

	// Allocate the three ortho views
    XYWnd* xyWnd = GlobalXYWnd().createXY();
    xyWnd->setViewType(XY);
    GtkWidget* xy = xyWnd->getWidget();
    
    XYWnd* yzWnd = GlobalXYWnd().createXY();
    yzWnd->setViewType(YZ);
    GtkWidget* yz = yzWnd->getWidget();

    XYWnd* xzWnd = GlobalXYWnd().createXY();
    xzWnd->setViewType(XZ);
    GtkWidget* xz = xzWnd->getWidget();

    GtkHPaned* split = create_split_views(camera, yz, xy, xz);
    gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(split), TRUE, TRUE, 0);

    {      
      GtkFrame* frame = create_framed_widget(TextureBrowser_constructWindow(window));
      g_page_textures = GroupDialog_addPage("Textures", 
      										"icon_texture.png",
      										GTK_WIDGET(frame), 
      										TextureBrowserExportTitleCaller());
    }
  }
  
  EntityList_constructWindow(window);
  PreferencesDialog_constructWindow(window);
  FindTextureDialog_constructWindow(window);
  SurfaceInspector_constructWindow(window);
  PatchInspector_constructWindow(window);

  GlobalGrid().addGridChangeCallback(SetGridStatusCaller(*this));
  GlobalGrid().addGridChangeCallback(ReferenceCaller<MainFrame, XY_UpdateAllWindows>(*this));

  g_defaultToolMode = DragMode;
  g_defaultToolMode();
  SetStatusText(m_command_status, c_TranslateMode_status);

  EverySecondTimer_enable();
  //GlobalShortcuts_reportUnregistered();
  
  // Restore any floating XYViews that were active before, this applies to all view layouts
  GlobalXYWnd().restoreState();
}

void MainFrame::SaveWindowInfo()
{
  if (!FloatingGroupDialog())
  {
    g_layout_globals.nXYHeight = gtk_paned_get_position(GTK_PANED(m_vSplit));

    if(CurrentStyle() != eRegular)
    {
      g_layout_globals.nCamWidth = gtk_paned_get_position(GTK_PANED(m_hSplit));
    }
    else
    {
      g_layout_globals.nXYWidth = gtk_paned_get_position(GTK_PANED(m_hSplit));
    }

    g_layout_globals.nCamHeight = gtk_paned_get_position(GTK_PANED(m_vSplit2));
  }

  g_layout_globals.m_position = m_position_tracker.getPosition();
 
  g_layout_globals.nState = gdk_window_get_state(GTK_WIDGET(m_window)->window);
}

void MainFrame::Shutdown()
{
  EverySecondTimer_disable();

  EntityList_destroyWindow();

  g_textures_menu = 0;

	// Save the camera size to the registry
	GlobalCamera().saveCamWndState();
	
	// Save the current XYViews to the registry
	GlobalXYWnd().saveState();
	GlobalXYWnd().destroyViews();

  TextureBrowser_destroyWindow();

  GlobalCamera().deleteCamWnd(m_pCamWnd);
  m_pCamWnd = 0;

  PreferencesDialog_destroyWindow();
  SurfaceInspector_destroyWindow();
  FindTextureDialog_destroyWindow();
  PatchInspector_destroyWindow();

  // destroying group-dialog last because it may contain texture-browser
  GroupDialog_destroyWindow();
}

void MainFrame::RedrawStatusText()
{
  gtk_label_set_text(GTK_LABEL(m_pStatusLabel[c_command_status]), m_command_status.c_str());
  gtk_label_set_text(GTK_LABEL(m_pStatusLabel[c_position_status]), m_position_status.c_str());
  gtk_label_set_text(GTK_LABEL(m_pStatusLabel[c_brushcount_status]), m_brushcount_status.c_str());
  gtk_label_set_text(GTK_LABEL(m_pStatusLabel[c_texture_status]), m_texture_status.c_str());
  gtk_label_set_text(GTK_LABEL(m_pStatusLabel[c_grid_status]), m_grid_status.c_str());
}

void MainFrame::UpdateStatusText()
{
  m_idleRedrawStatusText.queueDraw();
}

void MainFrame::SetStatusText(CopiedString& status_text, const char* pText)
{
  status_text = pText;
  UpdateStatusText();
}

void Sys_Status(const char* status)
{
  if(g_pParentWnd != 0)
  {
    g_pParentWnd->SetStatusText (g_pParentWnd->m_command_status, status);
  }
}

int getRotateIncrement()
{
  return static_cast<int>(g_si_globals.rotate);
}

int getFarClipDistance() {
	return getCameraSettings()->cubicScale();
}

int (*GridStatus_getRotateIncrement)() = getRotateIncrement;
int (*GridStatus_getFarClipDistance)() = getFarClipDistance;

void MainFrame::SetGridStatus()
{
  StringOutputStream status(64);
  const char* lock = (GlobalBrush()->textureLockEnabled()) ? "ON" : "OFF";
  status << "G:" << GlobalGrid().getGridSize()
    << "  R:" << GridStatus_getRotateIncrement()
    << "  C:" << GridStatus_getFarClipDistance()
    << "  L:" << lock;
  SetStatusText(m_grid_status, status.c_str());
}

void GridStatus_onTextureLockEnabledChanged()
{
  if(g_pParentWnd != 0)
  {
    g_pParentWnd->SetGridStatus();
  }
}

namespace
{
  GLFont g_font(0, 0);
}

void GlobalGL_sharedContextCreated()
{
  // report OpenGL information
  globalOutputStream() << "GL_VENDOR: " << reinterpret_cast<const char*>(glGetString (GL_VENDOR)) << "\n";
  globalOutputStream() << "GL_RENDERER: " << reinterpret_cast<const char*>(glGetString (GL_RENDERER)) << "\n";
  globalOutputStream() << "GL_VERSION: " << reinterpret_cast<const char*>(glGetString (GL_VERSION)) << "\n";
  globalOutputStream() << "GL_EXTENSIONS: " << reinterpret_cast<const char*>(glGetString (GL_EXTENSIONS)) << "\n";

  QGL_sharedContextCreated(GlobalOpenGL());

  ShaderCache_extensionsInitialised();

  GlobalShaderCache().realise();
  Textures_Realise();

  g_font = glfont_create("courier 8");
  GlobalOpenGL().m_font = g_font.getDisplayList();
  GlobalOpenGL().m_fontHeight = g_font.getPixelHeight();
}

void GlobalGL_sharedContextDestroyed()
{
  Textures_Unrealise();
  GlobalShaderCache().unrealise();

  QGL_sharedContextDestroyed(GlobalOpenGL());
}


void Layout_constructPreferences(PrefPage* page)
{
  {
    const char* layouts[] = { "window1.bmp", "window2.bmp", "window3.bmp", "window4.bmp" };
    page->appendRadioIcons(
      "Window Layout",
      STRING_ARRAY_RANGE(layouts),
      LatchedIntImportCaller(g_Layout_viewStyle),
      IntExportCaller(g_Layout_viewStyle.m_latched)
    );
  }
  page->appendCheckBox(
    "", "Detachable Menus",
    LatchedBoolImportCaller(g_Layout_enableDetachableMenus),
    BoolExportCaller(g_Layout_enableDetachableMenus.m_latched)
  );
  if (!string_empty(g_pGameDescription->getKeyValue("no_patch")))
  {
    page->appendCheckBox(
      "", "Patch Toolbar",
      LatchedBoolImportCaller(g_Layout_enablePatchToolbar),
      BoolExportCaller(g_Layout_enablePatchToolbar.m_latched)
    );
  }
  page->appendCheckBox(
    "", "Plugin Toolbar",
    LatchedBoolImportCaller(g_Layout_enablePluginToolbar),
    BoolExportCaller(g_Layout_enablePluginToolbar.m_latched)
  );
}

void Layout_constructPage(PreferenceGroup& group)
{
  PreferencesPage* page(group.createPage("Layout", "Layout Preferences"));
  Layout_constructPreferences(reinterpret_cast<PrefPage*>(page));
}

void Layout_registerPreferencesPage()
{
  PreferencesDialog_addInterfacePage(FreeCaller1<PreferenceGroup&, Layout_constructPage>());
}

void EditColourScheme() {
	new ui::ColourSchemeEditor(); // self-destructs in GTK callback
}

#include "preferencesystem.h"
#include "stringio.h"

void MainFrame_Construct()
{
	GlobalEventManager().addCommand("Sleep", FreeCaller<thunk_OnSleep>());
	GlobalEventManager().addCommand("NewMap", FreeCaller<NewMap>());
	GlobalEventManager().addCommand("OpenMap", FreeCaller<OpenMap>());
	GlobalEventManager().addCommand("ImportMap", FreeCaller<ImportMap>());
	GlobalEventManager().addCommand("SaveMap", FreeCaller<SaveMap>());
	GlobalEventManager().addCommand("SaveMapAs", FreeCaller<SaveMapAs>());
	GlobalEventManager().addCommand("SaveSelected", FreeCaller<ExportMap>());
	GlobalEventManager().addCommand("SaveRegion", FreeCaller<SaveRegion>());
	GlobalEventManager().addCommand("RefreshReferences", FreeCaller<RefreshReferences>());
	GlobalEventManager().addCommand("ProjectSettings", FreeCaller<DoProjectSettings>());
	GlobalEventManager().addCommand("Exit", FreeCaller<Exit>());
	
	GlobalEventManager().addCommand("Undo", FreeCaller<Undo>());
	GlobalEventManager().addCommand("Redo", FreeCaller<Redo>());
	GlobalEventManager().addCommand("Copy", FreeCaller<Copy>());
	GlobalEventManager().addCommand("Paste", FreeCaller<Paste>());
	GlobalEventManager().addCommand("PasteToCamera", FreeCaller<PasteToCamera>());
	GlobalEventManager().addCommand("CloneSelection", FreeCaller<Selection_Clone>());
	GlobalEventManager().addCommand("DeleteSelection", FreeCaller<deleteSelection>());
	GlobalEventManager().addCommand("ParentSelection", FreeCaller<Scene_parentSelected>());
	GlobalEventManager().addCommand("UnSelectSelection", FreeCaller<Selection_Deselect>());
	GlobalEventManager().addCommand("InvertSelection", FreeCaller<Select_Invert>());
	GlobalEventManager().addCommand("SelectInside", FreeCaller<Select_Inside>());
	GlobalEventManager().addCommand("SelectTouching", FreeCaller<Select_Touching>());
	GlobalEventManager().addCommand("ExpandSelectionToEntities", FreeCaller<Scene_ExpandSelectionToEntities>());
	GlobalEventManager().addCommand("Preferences", FreeCaller<PreferencesDialog_showDialog>());
	
	GlobalEventManager().addCommand("ToggleConsole", FreeCaller<Console_ToggleShow>());
	GlobalEventManager().addCommand("EntityList", FreeCaller<EntityList_toggleShown>());
	
	// Entity inspector (part of Group Dialog)
	GlobalEventManager().addCommand("ToggleEntityInspector",
	                                   FreeCaller<EntityInspector_ToggleShow>());
	
	// Light inspector
	GlobalEventManager().addCommand("ToggleLightInspector",
	                                   FreeCaller<ui::LightInspector::displayDialog>());
	
	GlobalEventManager().addCommand("ShowHidden", FreeCaller<Select_ShowAllHidden>());
	GlobalEventManager().addCommand("HideSelected", FreeCaller<HideSelected>());
	
	GlobalEventManager().addToggle("DragVertices", FreeCaller<ToggleVertexMode>());
	GlobalEventManager().addToggle("DragEdges", FreeCaller<ToggleEdgeMode>());
	GlobalEventManager().addToggle("DragFaces", FreeCaller<ToggleFaceMode>());
	GlobalEventManager().setToggled("DragVertices", false);
	GlobalEventManager().setToggled("DragEdges", false);
	GlobalEventManager().setToggled("DragFaces", false); 
	
	GlobalEventManager().addCommand("MirrorSelectionX", FreeCaller<Selection_Flipx>());
	GlobalEventManager().addCommand("RotateSelectionX", FreeCaller<Selection_Rotatex>());
	GlobalEventManager().addCommand("MirrorSelectionY", FreeCaller<Selection_Flipy>());
	GlobalEventManager().addCommand("RotateSelectionY", FreeCaller<Selection_Rotatey>());
	GlobalEventManager().addCommand("MirrorSelectionZ", FreeCaller<Selection_Flipz>());
	GlobalEventManager().addCommand("RotateSelectionZ", FreeCaller<Selection_Rotatez>());
	
	GlobalEventManager().addCommand("ArbitraryRotation", FreeCaller<DoRotateDlg>());
	GlobalEventManager().addCommand("ArbitraryScale", FreeCaller<DoScaleDlg>());
	
	GlobalEventManager().addCommand("FindBrush", FreeCaller<DoFind>());
	
	GlobalEventManager().addCommand("MapInfo", FreeCaller<DoMapInfo>());
	
	GlobalEventManager().addRegistryToggle("ToggleShowSizeInfo", RKEY_SHOW_SIZE_INFO);
	GlobalEventManager().addRegistryToggle("ToggleShowAllLightRadii", "user/ui/showAllLightRadii");

	GlobalEventManager().addToggle("ToggleClipper", FreeCaller<ClipperMode>());
	
	GlobalEventManager().addToggle("MouseTranslate", FreeCaller<TranslateMode>());
	GlobalEventManager().addToggle("MouseRotate", FreeCaller<RotateMode>());
	GlobalEventManager().addToggle("MouseScale", FreeCaller<ScaleMode>());
	GlobalEventManager().addToggle("MouseDrag", FreeCaller<DragMode>());
	
	GlobalEventManager().addCommand("CSGSubtract", FreeCaller<CSG_Subtract>());
	GlobalEventManager().addCommand("CSGMerge", FreeCaller<CSG_Merge>());
	GlobalEventManager().addCommand("CSGHollow", FreeCaller<CSG_MakeHollow>());
	
	GlobalEventManager().addCommand("TextureDirectoryList", FreeCaller<DoTextureListDlg>());
	
	GlobalEventManager().addCommand("RefreshShaders", FreeCaller<RefreshShaders>());
	
	GlobalEventManager().addCommand("SnapToGrid", FreeCaller<Selection_SnapToGrid>());
	
	GlobalEventManager().addCommand("SelectAllOfType", FreeCaller<Select_AllOfType>());
	
	GlobalEventManager().addCommand("TexRotateClock", FreeCaller<Texdef_RotateClockwise>());
	GlobalEventManager().addCommand("TexRotateCounter", FreeCaller<Texdef_RotateAntiClockwise>());
	GlobalEventManager().addCommand("TexScaleUp", FreeCaller<Texdef_ScaleUp>());
	GlobalEventManager().addCommand("TexScaleDown", FreeCaller<Texdef_ScaleDown>());
	GlobalEventManager().addCommand("TexScaleLeft", FreeCaller<Texdef_ScaleLeft>());
	GlobalEventManager().addCommand("TexScaleRight", FreeCaller<Texdef_ScaleRight>());
	GlobalEventManager().addCommand("TexShiftUp", FreeCaller<Texdef_ShiftUp>());
	GlobalEventManager().addCommand("TexShiftDown", FreeCaller<Texdef_ShiftDown>());
	GlobalEventManager().addCommand("TexShiftLeft", FreeCaller<Texdef_ShiftLeft>());
	GlobalEventManager().addCommand("TexShiftRight", FreeCaller<Texdef_ShiftRight>());
	
	GlobalEventManager().addCommand("MoveSelectionDOWN", FreeCaller<Selection_MoveDown>());
	GlobalEventManager().addCommand("MoveSelectionUP", FreeCaller<Selection_MoveUp>());
	
	GlobalEventManager().addCommand("SelectNudgeLeft", FreeCaller<Selection_NudgeLeft>());
	GlobalEventManager().addCommand("SelectNudgeRight", FreeCaller<Selection_NudgeRight>());
	GlobalEventManager().addCommand("SelectNudgeUp", FreeCaller<Selection_NudgeUp>());
	GlobalEventManager().addCommand("SelectNudgeDown", FreeCaller<Selection_NudgeDown>());
	
	GlobalEventManager().addCommand("EditColourScheme", FreeCaller<EditColourScheme>());
	GlobalEventManager().addCommand("BrushExportOBJ", FreeCaller<CallBrushExportOBJ>());

	GlobalEventManager().addCommand("ShowCommandList", FreeCaller<ShowCommandListDialog>());
	GlobalEventManager().addCommand("About", FreeCaller<DoAbout>());
  
  Patch_registerCommands();

  typedef FreeCaller1<const Selectable&, ComponentMode_SelectionChanged> ComponentModeSelectionChangedCaller;
  GlobalSelectionSystem().addSelectionChangeCallback(ComponentModeSelectionChangedCaller());

  GlobalPreferenceSystem().registerPreference("DetachableMenus", BoolImportStringCaller(g_Layout_enableDetachableMenus.m_latched), BoolExportStringCaller(g_Layout_enableDetachableMenus.m_latched));
  GlobalPreferenceSystem().registerPreference("PatchToolBar", BoolImportStringCaller(g_Layout_enablePatchToolbar.m_latched), BoolExportStringCaller(g_Layout_enablePatchToolbar.m_latched));
  GlobalPreferenceSystem().registerPreference("PluginToolBar", BoolImportStringCaller(g_Layout_enablePluginToolbar.m_latched), BoolExportStringCaller(g_Layout_enablePluginToolbar.m_latched));
  GlobalPreferenceSystem().registerPreference("QE4StyleWindows", IntImportStringCaller(g_Layout_viewStyle.m_latched), IntExportStringCaller(g_Layout_viewStyle.m_latched));
  GlobalPreferenceSystem().registerPreference("XYHeight", IntImportStringCaller(g_layout_globals.nXYHeight), IntExportStringCaller(g_layout_globals.nXYHeight));
  GlobalPreferenceSystem().registerPreference("XYWidth", IntImportStringCaller(g_layout_globals.nXYWidth), IntExportStringCaller(g_layout_globals.nXYWidth));
  GlobalPreferenceSystem().registerPreference("CamWidth", IntImportStringCaller(g_layout_globals.nCamWidth), IntExportStringCaller(g_layout_globals.nCamWidth));
  GlobalPreferenceSystem().registerPreference("CamHeight", IntImportStringCaller(g_layout_globals.nCamHeight), IntExportStringCaller(g_layout_globals.nCamHeight));

  GlobalPreferenceSystem().registerPreference("State", IntImportStringCaller(g_layout_globals.nState), IntExportStringCaller(g_layout_globals.nState));
  GlobalPreferenceSystem().registerPreference("PositionX", IntImportStringCaller(g_layout_globals.m_position.x), IntExportStringCaller(g_layout_globals.m_position.x));
  GlobalPreferenceSystem().registerPreference("PositionY", IntImportStringCaller(g_layout_globals.m_position.y), IntExportStringCaller(g_layout_globals.m_position.y));
  GlobalPreferenceSystem().registerPreference("Width", IntImportStringCaller(g_layout_globals.m_position.w), IntExportStringCaller(g_layout_globals.m_position.w));
  GlobalPreferenceSystem().registerPreference("Height", IntImportStringCaller(g_layout_globals.m_position.h), IntExportStringCaller(g_layout_globals.m_position.h));

  {
    const char* ENGINEPATH_ATTRIBUTE =
#if defined(WIN32)
      "enginepath_win32"
#elif defined(__linux__) || defined (__FreeBSD__)
      "enginepath_linux"
#elif defined(__APPLE__)
      "enginepath_macos"
#else
#error "unknown platform"
#endif
    ;
    StringOutputStream path(256);
    path << DirectoryCleaned(g_pGameDescription->getRequiredKeyValue(ENGINEPATH_ATTRIBUTE));
    g_strEnginePath = path.c_str();
  }

  GlobalPreferenceSystem().registerPreference("EnginePath", CopiedStringImportStringCaller(g_strEnginePath), CopiedStringExportStringCaller(g_strEnginePath));

  g_Layout_viewStyle.useLatched();
  g_Layout_enableDetachableMenus.useLatched();
  g_Layout_enablePatchToolbar.useLatched();
  g_Layout_enablePluginToolbar.useLatched();

  Layout_registerPreferencesPage();
  Paths_registerPreferencesPage();

  g_brushCount.setCountChangedCallback(FreeCaller<QE_brushCountChanged>());
  g_entityCount.setCountChangedCallback(FreeCaller<QE_entityCountChanged>());
  GlobalEntityCreator().setCounter(&g_entityCount);

  GLWidget_sharedContextCreated = GlobalGL_sharedContextCreated;
  GLWidget_sharedContextDestroyed = GlobalGL_sharedContextDestroyed;

  GlobalEntityClassManager().attach(g_WorldspawnColourEntityClassObserver);
}

void MainFrame_Destroy()
{
  GlobalEntityClassManager().detach(g_WorldspawnColourEntityClassObserver);

  GlobalEntityCreator().setCounter(0);
  g_entityCount.setCountChangedCallback(Callback());
  g_brushCount.setCountChangedCallback(Callback());
}
