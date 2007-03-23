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
#include "environment.h"

#include "ui/surfaceinspector/SurfaceInspector.h"
#include "ui/patch/PatchInspector.h"
#include "textool/TexTool.h"
#include "brushexport/BrushExportOBJ.h"
#include "ui/einspector/EntityInspector.h"
#include "ui/lightinspector/LightInspector.h"
#include "ui/mediabrowser/MediaBrowser.h"
#include "ui/common/ToolbarCreator.h"
#include "ui/colourscheme/ColourSchemeManager.h"
#include "ui/colourscheme/ColourSchemeEditor.h"
#include "ui/menu/FiltersMenu.h"
#include "ui/transform/TransformDialog.h"
#include "ui/overlay/OverlayDialog.h"
#include "selection/algorithm/Shader.h"
#include "selection/algorithm/Group.h"
#include "selection/algorithm/Primitives.h"
#include "selection/shaderclipboard/ShaderClipboard.h"
#include "iclipper.h"
#include "ifilesystem.h"
#include "iundo.h"
#include "igrid.h"
#include "ifilter.h"
#include "itoolbar.h"
#include "editable.h"
#include "ientity.h"
#include "iuimanager.h"
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
#include "gtkutil/Paned.h"
#include "gtkutil/widget.h"
#include "gtkutil/IconTextMenuToggle.h"
#include "gtkutil/TextMenuItem.h"
#include "gtkutil/FramedWidget.h"
#include "gtkutil/messagebox.h"
#include "gtkutil/dialog.h"

#include "map/AutoSaver.h"
#include "brushmanip.h"
#include "brush/BrushModule.h"
#include "csg.h"
#include "console.h"
#include "entity.h"
#include "entitylist.h"
#include "groupdialog.h"
#include "gtkdlgs.h"
#include "gtkmisc.h"
#include "map.h"
#include "multimon.h"
#include "patchmanip.h"
#include "plugin.h"
#include "points.h"
#include "preferences.h"
#include "qe3.h"
#include "qgl.h"
#include "select.h"
#include "server.h"
#include "texwindow.h"
#include "windowobservers.h"
#include "renderstate.h"
#include "referencecache.h"
#include "camera/GlobalCamera.h"
#include "camera/CameraSettings.h"
#include "xyview/GlobalXYWnd.h"
#include "ui/mru/MRU.h"
#include "ui/commandlist/CommandList.h"
#include "ui/findshader/FindShader.h"
#include "brush/FaceInstance.h"
#include "settings/GameManager.h"

extern FaceInstanceSet g_SelectedFaceInstances;

static GtkWindow *splash_screen = 0;

	namespace {
		const std::string RKEY_WINDOW_LAYOUT = "user/ui/mainFrame/windowLayout";
	}

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

// Home Paths
/*
void HomePaths_Realise() {
	// Linux has the userengine path under the home dir
#if defined(POSIX)
	std::string prefix = game::Manager::Instance().currentGame()->getKeyValue("prefix");
	if (!prefix.empty()) { 
		std::string path = os::standardPathWithSlash(g_get_home_dir()) + prefix + "/";
		g_qeglobals.m_userEnginePath = path.c_str();
		Q_mkdir(g_qeglobals.m_userEnginePath.c_str());
	}
	else
#endif
	{
		g_qeglobals.m_userEnginePath = EnginePath_get();
	}

	{
		std::string path = g_qeglobals.m_userEnginePath.c_str();
		path += std::string(gamename_get()) + "/";
		g_qeglobals.m_userGamePath = path.c_str();
	}
	ASSERT_MESSAGE(!string_empty(g_qeglobals.m_userGamePath.c_str()), "HomePaths_Realise: user-game-path is empty");
	Q_mkdir(g_qeglobals.m_userGamePath.c_str());
}*/

/*void EnginePath_Realise() {
	HomePaths_Realise();
	QE_InitVFS();
	
	// Rebuild the map path basing on the userGamePath (TODO: remove that global)
	std::string newMapPath = g_qeglobals.m_userGamePath.c_str();
	newMapPath += "maps/";
	Q_mkdir(newMapPath.c_str());
	Environment::Instance().setMapsPath(newMapPath);
}*/

/*const char* EnginePath_get() {
	return game::Manager::Instance().getEnginePath().c_str();
}*/

/*void EnginePath_Unrealise() {
	GlobalFileSystem().shutdown();
	Environment::Instance().setMapsPath("");
}*/

/*void setEnginePath(const char* path) {
  StringOutputStream buffer(256);
  buffer << DirectoryCleaned(path);
  if(!path_equal(buffer.c_str(), g_strEnginePath.c_str()))
  {
    ScopeDisableScreenUpdates disableScreenUpdates("Processing...", "Changing Engine Path");

    EnginePath_Unrealise();

    g_strEnginePath = buffer.c_str();

    EnginePath_Realise();
  }
}*/

/*class PathsDialog : public Dialog
{
public:
  GtkWindow* BuildDialog()
  {
    GtkFrame* frame = create_dialog_frame("Path settings", GTK_SHADOW_ETCHED_IN);

    GtkVBox* vbox2 = create_dialog_vbox(0, 4);
    gtk_container_add(GTK_CONTAINER(frame), GTK_WIDGET(vbox2));

    {
    	PathEntry pathEntry = PathEntry_new();
		g_signal_connect(
			G_OBJECT(pathEntry.m_button), 
			"clicked", 
			G_CALLBACK(button_clicked_entry_browse_directory), 
			pathEntry.m_entry
		);
	
		// Connect the registry key to the newly created input field
		_registryConnector.connectGtkObject(GTK_OBJECT(pathEntry.m_entry), RKEY_ENGINE_PATH);
	
		GtkTable* row = DialogRow_new("Engine Path", GTK_WIDGET(pathEntry.m_frame));
		DialogVBox_packRow(GTK_VBOX(vbox2), GTK_WIDGET(row));
    }

    return create_simple_modal_dialog_window("Engine Path Not Found", m_modal, GTK_WIDGET(frame));
  }
  
  virtual void PostModal(EMessageBoxReturn code) { 
  	if (code == eIDOK) {
  		_registryConnector.exportValues();
  	}
  }
};

PathsDialog g_PathsDialog;*/

void EnginePath_verify() {
	/*std::string enginePath = game::Manager::Instance().getEnginePath();
	
	if (!file_exists(enginePath.c_str())) {
		// Engine path doesn't exist, ask the user
		g_PathsDialog.Create();
		g_PathsDialog.DoModal();
		g_PathsDialog.Destroy();
	}*/
}

namespace
{
  //CopiedString g_gamename;
  CopiedString g_gamemode;
  ModuleObservers g_gameNameObservers;
  ModuleObservers g_gameModeObservers;
}

const char* basegame_get()
{
  return game::Manager::Instance().currentGame()->getRequiredKeyValue("basegame");
}

const char* gamename_get() {
	std::string fsGame = game::Manager::Instance().getFSGame();
	if (fsGame.empty()) {
		return basegame_get();
	}
	return fsGame.c_str();
}

void gamename_set(const char* gamename) {
	if (std::string(gamename) != game::Manager::Instance().getFSGame()) {
		g_gameNameObservers.unrealise();
		//g_gamename = gamename;
		game::Manager::Instance().setFSGame(gamename);
		g_gameNameObservers.realise();
	}
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

//! Make COLOR_BRUSHES override worldspawn eclass colour.
void SetWorldspawnColour(const Vector3& colour)
{
	IEntityClassPtr worldspawn = GlobalEntityClassManager().findOrInsert(
										"worldspawn", true);
	worldspawn->setColour(colour);
}

//! Make colourscheme definition override light volume eclass colour.
void SetLightVolumeColour(const Vector3& colour)
{
	IEntityClassPtr light = GlobalEntityClassManager().findOrInsert("light", 
																	true);
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

/*
 * Load XML config files into the XML registry.
 */
void populateRegistry() {
	
	// Load the XML files from the installation directory
	std::string base = GlobalRegistry().get(RKEY_APP_PATH);

	try {
		// Load all of the required XML files
		GlobalRegistry().import(base + "user.xml", "", Registry::treeStandard);
		GlobalRegistry().import(base + "upgradepaths.xml", "user", Registry::treeStandard);
		GlobalRegistry().import(base + "colours.xml", "user/ui", Registry::treeStandard);
		GlobalRegistry().import(base + "input.xml", "user/ui", Registry::treeStandard);
		GlobalRegistry().import(base + "menu.xml", "user/ui", Registry::treeStandard);
		
		// Load the debug.xml file only if the relevant key is set in user.xml
		if (GlobalRegistry().get("user/debug") == "1")
			GlobalRegistry().import(base + "debug.xml", "", Registry::treeStandard);
	}
	catch (std::runtime_error e) {
		gtkutil::fatalErrorDialog("XML registry population failed:\n\n"
								  + std::string(e.what()),
								  MainFrame_getWindow());
	}
	
	// Load user preferences, these overwrite any values that have defined before
	// The called method also checks for any upgrades that have to be performed
	const std::string userSettingsFile = GlobalRegistry().get(RKEY_SETTINGS_PATH) + "user.xml";
	if (file_exists(userSettingsFile.c_str())) {
		GlobalRegistry().import(userSettingsFile, "", Registry::treeUser);
	}
	
	const std::string userColoursFile = GlobalRegistry().get(RKEY_SETTINGS_PATH) + "colours.xml";
	if (file_exists(userColoursFile.c_str())) {
		GlobalRegistry().import(userColoursFile, "user/ui", Registry::treeUser);
	}
	
	const std::string userInputFile = GlobalRegistry().get(RKEY_SETTINGS_PATH) + "input.xml";
	if (file_exists(userInputFile.c_str())) {
		GlobalRegistry().import(userInputFile, "user/ui", Registry::treeUser);
	}
}

// This is called from main() to start up the Radiant stuff.
void Radiant_Initialise() 
{
	// Load the ColourSchemes from the registry
	ColourSchemes().loadColourSchemes();

	Preferences_Load();

	// Load the other modules
	Radiant_Construct(GlobalModuleServer_get());
	
	g_gameToolsPathObservers.realise();
	g_gameModeObservers.realise();
	g_gameNameObservers.realise();
	
	// Construct the MRU commands and menu structure
	GlobalMRU().constructMenu();
	
	// Initialise the most recently used files list
	GlobalMRU().loadRecentFiles();
}

void Radiant_Shutdown() {
	GlobalMRU().saveRecentFiles();
	
	// Export the colour schemes and remove them from the registry
	GlobalRegistry().exportToFile("user/ui/colourschemes", GlobalRegistry().get(RKEY_SETTINGS_PATH) + "colours.xml");
	GlobalRegistry().deleteXPath("user/ui/colourschemes");
	
	// Save the current event set to the Registry and export it 
	GlobalEventManager().saveEventListToRegistry();
	
	// Export the input definitions into the user's settings folder and remove them as well
	GlobalRegistry().exportToFile("user/ui/input", GlobalRegistry().get(RKEY_SETTINGS_PATH) + "input.xml");
	GlobalRegistry().deleteXPath("user/ui/input");
	
	// Delete all nodes marked as "transient", they are NOT exported into the user's xml file
	GlobalRegistry().deleteXPath("user/*[@transient='1']");
	
	// Remove any remaining upgradePaths (from older registry files)
	GlobalRegistry().deleteXPath("user/upgradePaths");
	// Remove legacy <interface> node
	GlobalRegistry().deleteXPath("user/ui/interface");
	
	// Save the remaining /darkradiant/user tree to user.xml so that the current settings are preserved
	GlobalRegistry().exportToFile("user", GlobalRegistry().get(RKEY_SETTINGS_PATH) + "user.xml");

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
  ui::SurfaceInspector::Instance().update();
  ui::PatchInspector::Instance().update();
  ui::LightInspector::Instance().update();
  GlobalShaderClipboard().clear();
}

void Redo()
{
  GlobalUndoSystem().redo();
  SceneChangeNotify();
  ui::SurfaceInspector::Instance().update();
  ui::PatchInspector::Instance().update();
  ui::LightInspector::Instance().update();
  GlobalShaderClipboard().clear();
}

void deleteSelection()
{
  UndoableCommand undo("deleteSelected");
  Select_Delete();
  GlobalShaderClipboard().clear();
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

void Copy() {
	if (g_SelectedFaceInstances.empty()) {
		Selection_Copy();
	}
	else {
		selection::algorithm::pickShaderFromSelection();
	}
}

void Paste() {
	if (g_SelectedFaceInstances.empty()) {
		UndoableCommand undo("paste");

		GlobalSelectionSystem().setSelectedAll(false);
		Selection_Paste();
	}
	else {
		selection::algorithm::pasteShaderToSelection();
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

void Restart() {
	Radiant_Shutdown();
	Radiant_Initialise();
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

void ToggleEntityMode() {
	if (GlobalSelectionSystem().Mode() == SelectionSystem::eEntity) {
		SelectionSystem_DefaultMode();
	}
	else {
		GlobalSelectionSystem().SetMode(SelectionSystem::eEntity);
		GlobalSelectionSystem().SetComponentMode(SelectionSystem::eDefault);
	}

	ComponentModeChanged();

	ModeChangeNotify();
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
	//GlobalEventManager().setToggled("MouseScale", GlobalSelectionSystem().ManipulatorMode() == SelectionSystem::eScale);
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
  /*if(g_currentToolMode == ScaleMode && g_defaultToolMode != ScaleMode)
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
  }*/
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
    map::AutoSaver().stopTimer();

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
    map::AutoSaver().startTimer();
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

void XY_UpdateAllWindows()
{
  if(g_pParentWnd != 0)
  {
    GlobalXYWnd().updateAllViews();
    
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


GtkWidget* g_toggle_z_item = 0;
GtkWidget* g_toggle_console_item = 0;
GtkWidget* g_toggle_entity_item = 0;
GtkWidget* g_toggle_entitylist_item = 0;

// The "Flush & Reload Shaders" command target 
void RefreshShaders() {
	ScopeDisableScreenUpdates disableScreenUpdates("Processing...", "Loading Shaders");
	GlobalShaderSystem().refresh();
	ui::MediaBrowser::getInstance().reloadMedia();
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
  	// Maybe the splash screen is visible?
  	if (splash_screen != NULL) {
  		return splash_screen;
  	}
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

void show_splash()
{
  splash_screen = create_splash();

  process_gui();
}

void hide_splash()
{
  gtk_widget_destroy(GTK_WIDGET(splash_screen));
  splash_screen = NULL;
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
    
    GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
    
    gtk_container_add(GTK_CONTAINER(window), vbox);
    gtk_widget_show(vbox);
    
    GlobalEventManager().connect(GTK_OBJECT(window));
    GlobalEventManager().connectAccelGroup(GTK_WINDOW(window));
    
    int viewStyle = GlobalRegistry().getInt(RKEY_WINDOW_LAYOUT);
    
    switch (viewStyle) {
    	case 0: m_nCurrentStyle = eRegular; break;
    	case 1: m_nCurrentStyle = eFloating; break;
    	case 2: m_nCurrentStyle = eSplit; break;
    	case 3: m_nCurrentStyle = eRegularLeft; break;
    	default: m_nCurrentStyle = eFloating; break;
    };

    // Create the Filter menu entries
    ui::FiltersMenu::addItems();
    
    // Retrieve the "main" menubar from the UIManager
    GtkMenuBar* mainMenu = GTK_MENU_BAR(GlobalUIManager().getMenuManager().get("main"));
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(mainMenu), false, false, 0);
    
    if (m_nCurrentStyle != eFloating) {
    	// Hide the camera toggle option for non-floating views
    	GlobalUIManager().getMenuManager().setVisibility("main/view/cameraview", false);
    }
    
	if (m_nCurrentStyle != eFloating && m_nCurrentStyle != eSplit) {
		// Hide the console/texture browser toggles for non-floating/non-split views
		GlobalUIManager().getMenuManager().setVisibility("main/view/consoleView", false);
		GlobalUIManager().getMenuManager().setVisibility("main/view/textureBrowser", false);	
	}
    
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
  else // 4 way (aka Splitplane view)
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

	// Arrange the widgets into the paned views
	_splitPane.vertPane1 = gtkutil::Paned(gtkutil::FramedWidget(camera), 
										  gtkutil::FramedWidget(yz), 
										  false);
	_splitPane.vertPane2 = gtkutil::Paned(gtkutil::FramedWidget(xy), 
										  gtkutil::FramedWidget(xz), 
										  false);
	_splitPane.horizPane = gtkutil::Paned(_splitPane.vertPane1, _splitPane.vertPane2, true);

	gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(_splitPane.horizPane), TRUE, TRUE, 0);

	gtk_paned_set_position(GTK_PANED(_splitPane.horizPane), 200);
	gtk_paned_set_position(GTK_PANED(_splitPane.vertPane1), 200);
	gtk_paned_set_position(GTK_PANED(_splitPane.vertPane2), 400);

	_splitPane.posHPane.connect(_splitPane.horizPane);
	_splitPane.posVPane1.connect(_splitPane.vertPane1);
	_splitPane.posVPane2.connect(_splitPane.vertPane2);
	
	// TODO: Move this whole stuff into a class (maybe some deriving from a Layout class)
	xml::NodeList list = GlobalRegistry().findXPath("user/ui/mainFrame/splitPane/pane[@name='horizontal']");
	
	if (list.size() > 0) {
		_splitPane.posHPane.loadFromNode(list[0]);
		_splitPane.posHPane.applyPosition();
	}
	
	list = GlobalRegistry().findXPath("user/ui/mainFrame/splitPane/pane[@name='vertical1']");
	
	if (list.size() > 0) {
		_splitPane.posVPane1.loadFromNode(list[0]);
		_splitPane.posVPane1.applyPosition();
	}
	
	list = GlobalRegistry().findXPath("user/ui/mainFrame/splitPane/pane[@name='vertical2']");
	
	if (list.size() > 0) {
		_splitPane.posVPane2.loadFromNode(list[0]);
		_splitPane.posVPane2.applyPosition();
	}
	
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
  
  GlobalGrid().addGridChangeCallback(SetGridStatusCaller(*this));
  GlobalGrid().addGridChangeCallback(FreeCaller<XY_UpdateAllWindows>());

  g_defaultToolMode = DragMode;
  g_defaultToolMode();
  SetStatusText(m_command_status, c_TranslateMode_status);

	// Start the autosave timer so that it can periodically check the map for changes 
	map::AutoSaver().startTimer();
  
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
  
	// Save the splitpane widths/heights 
	if (CurrentStyle() == eSplit) {
		// TODO: Move this whole stuff into a class 
		// (maybe some deriving from a Layout class)
		
		std::string path("user/ui/mainFrame/splitPane");
		
		// Remove all previously stored pane information 
		GlobalRegistry().deleteXPath(path + "//pane");
		
		xml::Node node = GlobalRegistry().createKeyWithName(path, "pane", "horizontal");
		_splitPane.posHPane.saveToNode(node);
		
		node = GlobalRegistry().createKeyWithName(path, "pane", "vertical1");
		_splitPane.posVPane1.saveToNode(node);
		
		node = GlobalRegistry().createKeyWithName(path, "pane", "vertical2");
		_splitPane.posVPane2.saveToNode(node);
	}
}

void MainFrame::Shutdown()
{
	// Tell the inspectors to safely shutdown (de-register callbacks, etc.)
	ui::SurfaceInspector::Instance().shutdown();
	ui::PatchInspector::Instance().shutdown();
	ui::LightInspector::Instance().shutdown();
	ui::TexTool::Instance().shutdown();
	ui::TransformDialog::Instance().shutdown();
	PrefsDlg::Instance().shutdown();
	
	// Stop the AutoSaver class from being called
	map::AutoSaver().stopTimer();

  EntityList_destroyWindow();

  	// Save the camera size to the registry
	GlobalCamera().saveCamWndState();
	
	// Save the current XYViews to the registry
	GlobalXYWnd().saveState();
	GlobalXYWnd().destroyViews();

  TextureBrowser_destroyWindow();

  GlobalCamera().deleteCamWnd(m_pCamWnd);
  m_pCamWnd = 0;

  PreferencesDialog_destroyWindow();

  // destroying group-dialog last because it may contain texture-browser
  GroupDialog_destroyWindow();
}

void MainFrame::RedrawStatusText()
{
  gtk_label_set_markup(GTK_LABEL(m_pStatusLabel[c_command_status]), m_command_status.c_str());
  gtk_label_set_markup(GTK_LABEL(m_pStatusLabel[c_position_status]), m_position_status.c_str());
  gtk_label_set_markup(GTK_LABEL(m_pStatusLabel[c_brushcount_status]), m_brushcount_status.c_str());
  gtk_label_set_markup(GTK_LABEL(m_pStatusLabel[c_texture_status]), m_texture_status.c_str());
  gtk_label_set_markup(GTK_LABEL(m_pStatusLabel[c_grid_status]), m_grid_status.c_str());
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

void Sys_Status(const std::string& statusText) {
	if (g_pParentWnd != 0) {
		g_pParentWnd->SetStatusText(g_pParentWnd->m_command_status, statusText.c_str());
	}
}

int getFarClipDistance() {
	return getCameraSettings()->cubicScale();
}

int (*GridStatus_getFarClipDistance)() = getFarClipDistance;

void MainFrame::SetGridStatus()
{
  StringOutputStream status(64);
  const char* lock = (GlobalBrush()->textureLockEnabled()) ? "ON" : "OFF";
  status << "G:" << GlobalGrid().getGridSize()
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

  g_font = glfont_create("courier 8");
  GlobalOpenGL().m_font = g_font.getDisplayList();
  GlobalOpenGL().m_fontHeight = g_font.getPixelHeight();
}

void GlobalGL_sharedContextDestroyed()
{
  GlobalShaderCache().unrealise();

  QGL_sharedContextDestroyed(GlobalOpenGL());
}

void Layout_registerPreferencesPage() {
	PreferencesPagePtr page = GlobalPreferenceSystem().getPage("Interface");
	
	IconList icons;
	IconDescriptionList descriptions;
	
	icons.push_back("window_regular.bmp"); descriptions.push_back("Regular");
	icons.push_back("window_floating.bmp"); descriptions.push_back("Floating");
	icons.push_back("window_split.bmp"); descriptions.push_back("Split");
	icons.push_back("window_regular_left.bmp"); descriptions.push_back("Regular Left");
	
	page->appendRadioIcons("Window Layout", RKEY_WINDOW_LAYOUT, icons, descriptions);
	page->appendLabel("<b>Note</b>: You will have to restart DarkRadiant for the changes to take effect.");
}

void EditColourScheme() {
	new ui::ColourSchemeEditor(); // self-destructs in GTK callback
}

#include "preferencesystem.h"
#include "stringio.h"

void MainFrame_Construct()
{
	// Tell the FilterSystem to register its commands
	GlobalFilterSystem().initialise();
	
	GlobalEventManager().addCommand("Sleep", FreeCaller<thunk_OnSleep>());
	GlobalEventManager().addCommand("NewMap", FreeCaller<NewMap>());
	GlobalEventManager().addCommand("OpenMap", FreeCaller<OpenMap>());
	GlobalEventManager().addCommand("ImportMap", FreeCaller<ImportMap>());
	GlobalEventManager().addCommand("LoadPrefab", FreeCaller<map::loadPrefab>());
	GlobalEventManager().addCommand("SaveSelectedAsPrefab", FreeCaller<map::saveSelectedAsPrefab>());
	GlobalEventManager().addCommand("SaveMap", FreeCaller<SaveMap>());
	GlobalEventManager().addCommand("SaveMapAs", FreeCaller<SaveMapAs>());
	GlobalEventManager().addCommand("SaveSelected", FreeCaller<ExportMap>());
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
	GlobalEventManager().addCommand("SelectCompleteTall", FreeCaller<Select_Complete_Tall>());
	GlobalEventManager().addCommand("ExpandSelectionToEntities", FreeCaller<Scene_ExpandSelectionToEntities>());
	GlobalEventManager().addCommand("Preferences", FreeCaller<PreferencesDialog_showDialog>());
	
	GlobalEventManager().addCommand("ToggleConsole", FreeCaller<Console_ToggleShow>());
	GlobalEventManager().addCommand("EntityList", FreeCaller<EntityList_toggleShown>());
	
	// Entity inspector (part of Group Dialog)
	GlobalEventManager().addCommand("ToggleEntityInspector",
									FreeCaller<EntityInspector_ToggleShow>());
	
	// Light inspector
	GlobalEventManager().addCommand("ToggleLightInspector",	
							FreeCaller<ui::LightInspector::toggleInspector>());
	
	GlobalEventManager().addCommand("SurfaceInspector", FreeCaller<ui::SurfaceInspector::toggle>());
	GlobalEventManager().addCommand("PatchInspector", FreeCaller<ui::PatchInspector::toggle>());
	
	// Overlay dialog
	GlobalEventManager().addCommand("OverlayDialog",
									FreeCaller<ui::OverlayDialog::display>());
	
	GlobalEventManager().addCommand("ShowHidden", FreeCaller<Select_ShowAllHidden>());
	GlobalEventManager().addCommand("HideSelected", FreeCaller<HideSelected>());
	
	GlobalEventManager().addToggle("DragVertices", FreeCaller<ToggleVertexMode>());
	GlobalEventManager().addToggle("DragEdges", FreeCaller<ToggleEdgeMode>());
	GlobalEventManager().addToggle("DragFaces", FreeCaller<ToggleFaceMode>());
	GlobalEventManager().addToggle("DragEntities", FreeCaller<ToggleEntityMode>());
	GlobalEventManager().setToggled("DragVertices", false);
	GlobalEventManager().setToggled("DragEdges", false);
	GlobalEventManager().setToggled("DragFaces", false); 
	GlobalEventManager().setToggled("DragEntities", false);
	
	GlobalEventManager().addCommand("MirrorSelectionX", FreeCaller<Selection_Flipx>());
	GlobalEventManager().addCommand("RotateSelectionX", FreeCaller<Selection_Rotatex>());
	GlobalEventManager().addCommand("MirrorSelectionY", FreeCaller<Selection_Flipy>());
	GlobalEventManager().addCommand("RotateSelectionY", FreeCaller<Selection_Rotatey>());
	GlobalEventManager().addCommand("MirrorSelectionZ", FreeCaller<Selection_Flipz>());
	GlobalEventManager().addCommand("RotateSelectionZ", FreeCaller<Selection_Rotatez>());
	
	GlobalEventManager().addCommand("TransformDialog", FreeCaller<ui::TransformDialog::toggle>());
	
	GlobalEventManager().addCommand("FindBrush", FreeCaller<DoFind>());
	GlobalEventManager().addCommand("RevertToWorldspawn", FreeCaller<selection::algorithm::revertGroupToWorldSpawn>());
	GlobalEventManager().addCommand("MapInfo", FreeCaller<DoMapInfo>());
	
	GlobalEventManager().addRegistryToggle("ToggleShowSizeInfo", RKEY_SHOW_SIZE_INFO);
	GlobalEventManager().addRegistryToggle("ToggleShowAllLightRadii", "user/ui/showAllLightRadii");

	GlobalEventManager().addToggle("ToggleClipper", FreeCaller<ClipperMode>());
	
	GlobalEventManager().addToggle("MouseTranslate", FreeCaller<TranslateMode>());
	GlobalEventManager().addToggle("MouseRotate", FreeCaller<RotateMode>());
	//GlobalEventManager().addToggle("MouseScale", FreeCaller<ScaleMode>());
	GlobalEventManager().addToggle("MouseDrag", FreeCaller<DragMode>());
	
	GlobalEventManager().addCommand("CSGSubtract", FreeCaller<CSG_Subtract>());
	GlobalEventManager().addCommand("CSGMerge", FreeCaller<CSG_Merge>());
	GlobalEventManager().addCommand("CSGHollow", FreeCaller<CSG_MakeHollow>());
	
	GlobalEventManager().addCommand("TextureDirectoryList", FreeCaller<DoTextureListDlg>());
	
	GlobalEventManager().addCommand("RefreshShaders", FreeCaller<RefreshShaders>());
	
	GlobalEventManager().addCommand("SnapToGrid", FreeCaller<Selection_SnapToGrid>());
	
	GlobalEventManager().addCommand("SelectAllOfType", FreeCaller<Select_AllOfType>());
	GlobalEventManager().addCommand("GroupCycleForward", FreeCaller<selection::GroupCycle::cycleForward>());
	GlobalEventManager().addCommand("GroupCycleBackward", FreeCaller<selection::GroupCycle::cycleBackward>());
	
	GlobalEventManager().addCommand("TexRotateClock", FreeCaller<selection::algorithm::rotateTextureClock>());
	GlobalEventManager().addCommand("TexRotateCounter", FreeCaller<selection::algorithm::rotateTextureCounter>());
	GlobalEventManager().addCommand("TexScaleUp", FreeCaller<selection::algorithm::scaleTextureUp>());
	GlobalEventManager().addCommand("TexScaleDown", FreeCaller<selection::algorithm::scaleTextureDown>());
	GlobalEventManager().addCommand("TexScaleLeft", FreeCaller<selection::algorithm::scaleTextureLeft>());
	GlobalEventManager().addCommand("TexScaleRight", FreeCaller<selection::algorithm::scaleTextureRight>());
	GlobalEventManager().addCommand("TexShiftUp", FreeCaller<selection::algorithm::shiftTextureUp>());
	GlobalEventManager().addCommand("TexShiftDown", FreeCaller<selection::algorithm::shiftTextureDown>());
	GlobalEventManager().addCommand("TexShiftLeft", FreeCaller<selection::algorithm::shiftTextureLeft>());
	GlobalEventManager().addCommand("TexShiftRight", FreeCaller<selection::algorithm::shiftTextureRight>());

	GlobalEventManager().addCommand("NormaliseTexture", FreeCaller<selection::algorithm::normaliseTexture>());

	GlobalEventManager().addCommand("CopyShader", FreeCaller<selection::algorithm::pickShaderFromSelection>());
	GlobalEventManager().addCommand("PasteShader", FreeCaller<selection::algorithm::pasteShaderToSelection>());
	GlobalEventManager().addCommand("PasteShaderNatural", FreeCaller<selection::algorithm::pasteShaderNaturalToSelection>());
  
	GlobalEventManager().addCommand("FlipTextureX", FreeCaller<selection::algorithm::flipTextureS>());
	GlobalEventManager().addCommand("FlipTextureY", FreeCaller<selection::algorithm::flipTextureT>());
	
	GlobalEventManager().addCommand("MoveSelectionDOWN", FreeCaller<Selection_MoveDown>());
	GlobalEventManager().addCommand("MoveSelectionUP", FreeCaller<Selection_MoveUp>());
	
	GlobalEventManager().addCommand("SelectNudgeLeft", FreeCaller<Selection_NudgeLeft>());
	GlobalEventManager().addCommand("SelectNudgeRight", FreeCaller<Selection_NudgeRight>());
	GlobalEventManager().addCommand("SelectNudgeUp", FreeCaller<Selection_NudgeUp>());
	GlobalEventManager().addCommand("SelectNudgeDown", FreeCaller<Selection_NudgeDown>());
	GlobalEventManager().addRegistryToggle("ToggleRotationPivot", "user/ui/rotationPivotIsOrigin");
	
	GlobalEventManager().addCommand("EditColourScheme", FreeCaller<EditColourScheme>());
	GlobalEventManager().addCommand("BrushExportOBJ", FreeCaller<CallBrushExportOBJ>());
	GlobalEventManager().addCommand("BrushExportCM", FreeCaller<selection::algorithm::createCMFromSelection>());

	GlobalEventManager().addCommand("FindReplaceTextures", FreeCaller<ui::FindAndReplaceShader::show>());
	GlobalEventManager().addCommand("ShowCommandList", FreeCaller<ui::CommandList::show>());
	GlobalEventManager().addCommand("About", FreeCaller<DoAbout>());

	ui::TexTool::registerCommands();

  Patch_registerCommands();

  typedef FreeCaller1<const Selectable&, ComponentMode_SelectionChanged> ComponentModeSelectionChangedCaller;
  GlobalSelectionSystem().addSelectionChangeCallback(ComponentModeSelectionChangedCaller());

  GlobalPreferenceSystem().registerPreference("XYHeight", IntImportStringCaller(g_layout_globals.nXYHeight), IntExportStringCaller(g_layout_globals.nXYHeight));
  GlobalPreferenceSystem().registerPreference("XYWidth", IntImportStringCaller(g_layout_globals.nXYWidth), IntExportStringCaller(g_layout_globals.nXYWidth));
  GlobalPreferenceSystem().registerPreference("CamWidth", IntImportStringCaller(g_layout_globals.nCamWidth), IntExportStringCaller(g_layout_globals.nCamWidth));
  GlobalPreferenceSystem().registerPreference("CamHeight", IntImportStringCaller(g_layout_globals.nCamHeight), IntExportStringCaller(g_layout_globals.nCamHeight));

  GlobalPreferenceSystem().registerPreference("State", IntImportStringCaller(g_layout_globals.nState), IntExportStringCaller(g_layout_globals.nState));
  GlobalPreferenceSystem().registerPreference("PositionX", IntImportStringCaller(g_layout_globals.m_position.x), IntExportStringCaller(g_layout_globals.m_position.x));
  GlobalPreferenceSystem().registerPreference("PositionY", IntImportStringCaller(g_layout_globals.m_position.y), IntExportStringCaller(g_layout_globals.m_position.y));
  GlobalPreferenceSystem().registerPreference("Width", IntImportStringCaller(g_layout_globals.m_position.w), IntExportStringCaller(g_layout_globals.m_position.w));
  GlobalPreferenceSystem().registerPreference("Height", IntImportStringCaller(g_layout_globals.m_position.h), IntExportStringCaller(g_layout_globals.m_position.h));

  Layout_registerPreferencesPage();

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
