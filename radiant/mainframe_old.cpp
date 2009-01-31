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

#include "mainframe_old.h"

#include "debugging/debugging.h"
#include "version.h"
#include "settings/PreferenceSystem.h"

#include "map/FindMapElements.h"
#include "ui/about/AboutDialog.h"
#include "ui/surfaceinspector/SurfaceInspector.h"
#include "ui/prefdialog/PrefDialog.h"
#include "ui/patch/PatchInspector.h"
#include "textool/TexTool.h"
#include "brushexport/BrushExportOBJ.h"
#include "ui/einspector/EntityInspector.h"
#include "ui/lightinspector/LightInspector.h"
#include "ui/mediabrowser/MediaBrowser.h"
#include "ui/menu/FiltersMenu.h"
#include "ui/transform/TransformDialog.h"
#include "ui/overlay/Overlay.h"
#include "ui/overlay/OverlayDialog.h"
#include "ui/layers/LayerControlDialog.h"
#include "ui/filterdialog/FilterDialog.h"
#include "selection/algorithm/Shader.h"
#include "selection/algorithm/General.h"
#include "selection/algorithm/Group.h"
#include "selection/algorithm/GroupCycle.h"
#include "selection/algorithm/Primitives.h"
#include "selection/algorithm/Transformation.h"
#include "selection/algorithm/Curves.h"
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
#include "igroupdialog.h"
#include "ieventmanager.h"
#include "ieclass.h"
#include "iregistry.h"
#include "irender.h"
#include "ishaders.h"
#include "igl.h"
#include "moduleobserver.h"
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

#include "scenelib.h"
#include "signal/isignal.h"
#include "os/path.h"
#include "os/file.h"
#include "moduleobservers.h"

#include "gtkutil/clipboard.h"
#include "gtkutil/glfont.h"
#include "gtkutil/GLWidget.h"
#include "gtkutil/Paned.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/widget.h"
#include "gtkutil/FramedWidget.h"
#include "gtkutil/messagebox.h"
#include "gtkutil/dialog.h"

#include "map/AutoSaver.h"
#include "brushmanip.h"
#include "brush/BrushModule.h"
#include "brush/csg/CSG.h"
#include "log/Console.h"
#include "entity.h"
#include "gtkdlgs.h"
#include "gtkmisc.h"
#include "patchmanip.h"
#include "select.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "windowobservers.h"
#include "referencecache.h"
#include "camera/GlobalCamera.h"
#include "camera/CameraSettings.h"
#include "xyview/GlobalXYWnd.h"
#include "ui/mru/MRU.h"
#include "ui/commandlist/CommandList.h"
#include "ui/findshader/FindShader.h"
#include "ui/mapinfo/MapInfoDialog.h"
#include "ui/splash/Splash.h"
#include "brush/FaceInstance.h"
#include "settings/GameManager.h"
#include "modulesystem/ModuleRegistry.h"
#include "RadiantModule.h"

extern FaceInstanceSet g_SelectedFaceInstances;

	namespace {
		const std::string RKEY_WINDOW_LAYOUT = "user/ui/mainFrame/windowLayout";
		const std::string RKEY_WINDOW_STATE = "user/ui/mainFrame/window";
		const std::string RKEY_MULTIMON_START_PRIMARY = "user/ui/multiMonitor/startOnPrimaryMonitor";
	}

// This is called from main() to start up the Radiant stuff.
void Radiant_Initialise() 
{
	// Create the empty Settings node and set the title to empty.
	ui::PrefDialog::Instance().createOrFindPage("Game");
	ui::PrefDialog::Instance().createOrFindPage("Interface");
	ui::PrefPagePtr settingsPage = ui::PrefDialog::Instance().createOrFindPage("Settings");
	settingsPage->setTitle("");
	
	ui::Splash::Instance().setProgressAndText("Constructing Menu", 0.89f);
	
	// Construct the MRU commands and menu structure
	GlobalMRU().constructMenu();
	
	// Initialise the most recently used files list
	GlobalMRU().loadRecentFiles();

	gtkutil::MultiMonitor::printMonitorInfo();
}

void Exit() {
	if (GlobalMap().askForSave("Exit Radiant")) {
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

void Map_ExportSelected(std::ostream& ostream) {
	GlobalMap().exportSelected(ostream);
}

void Map_ImportSelected(TextInputStream& istream) {
	GlobalMap().importSelected(istream);
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
	CamWnd& camwnd = *GlobalCamera().getCamWnd();
  GlobalSelectionSystem().setSelectedAll(false);
  
  UndoableCommand undo("pasteToCamera");
  
  Selection_Paste();
  
  // Work out the delta
  Vector3 mid = selection::algorithm::getCurrentSelectionCenter();
  Vector3 delta = vector3_snapped(camwnd.getCameraOrigin(), GlobalGrid().getGridSize()) - mid;
  
  // Move to camera
  GlobalSelectionSystem().translateSelected(delta);
}

void updateTextureBrowser() {
	GlobalTextureBrowser().queueDraw();
}

void SetClipMode(bool enable);
void ModeChangeNotify();

void DragMode();

typedef void(*ToolMode)();
ToolMode g_currentToolMode = DragMode;
bool g_currentToolModeSupportsComponentEditing = false;
ToolMode g_defaultToolMode = DragMode;

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
		// De-select all the selected edges before switching back
		GlobalSelectionSystem().setSelectedAllComponents(false);
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
		// De-select all the selected vertices before switching back
		GlobalSelectionSystem().setSelectedAllComponents(false);
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
		// De-select all the selected faces before switching back
		GlobalSelectionSystem().setSelectedAllComponents(false);
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

    GlobalSelectionSystem().SetManipulatorMode(SelectionSystem::eDrag);
    ToolChanged();
    ModeChangeNotify();
  }
}

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

    GlobalSelectionSystem().SetManipulatorMode(SelectionSystem::eTranslate);
    ToolChanged();
    ModeChangeNotify();
  }
}

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

    GlobalSelectionSystem().SetManipulatorMode(SelectionSystem::eRotate);
    ToolChanged();
    ModeChangeNotify();
  }
}

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

    GlobalSelectionSystem().SetManipulatorMode(SelectionSystem::eScale);
    ToolChanged();
    ModeChangeNotify();
  }*/
}


void ClipperMode() {
	if (g_currentToolMode == ClipperMode && g_defaultToolMode != ClipperMode) {
		g_defaultToolMode();
	}
	else {
		g_currentToolMode = ClipperMode;
		g_currentToolModeSupportsComponentEditing = false;

		SelectionSystem_DefaultMode();

		GlobalClipper().onClipMode(true);

		GlobalSelectionSystem().SetManipulatorMode(SelectionSystem::eClip);
		ToolChanged();
		ModeChangeNotify();
	}
}

class SnappableSnapToGridSelected : 
	public SelectionSystem::Visitor
{
	float _snap;
public:
	SnappableSnapToGridSelected(float snap) : 
		_snap(snap)
	{}

	void visit(const scene::INodePtr& node) const {
		// Don't do anything with hidden nodes
		if (!node->visible()) return;

		SnappablePtr snappable = Node_getSnappable(node);

		if (snappable != NULL) {
			snappable->snapto(_snap);
		}
	}
};

/* greebo: This is the visitor class to snap all components of a selected instance to the grid
 * While visiting all the instances, it checks if the instance derives from ComponentSnappable 
 */
class ComponentSnappableSnapToGridSelected : 
	public SelectionSystem::Visitor
{
	float _snap;
public:
	// Constructor: expects the grid size that should be snapped to
	ComponentSnappableSnapToGridSelected(float snap) : 
		_snap(snap)
	{}
  
	void visit(const scene::INodePtr& node) const {
		// Don't do anything with hidden nodes
	    if (!node->visible()) return;

    	// Check if the visited instance is componentSnappable
		ComponentSnappablePtr componentSnappable = Node_getComponentSnappable(node);
		
		if (componentSnappable != NULL) {
			componentSnappable->snapComponents(_snap);
		}
	}
}; // ComponentSnappableSnapToGridSelected

void Selection_SnapToGrid()
{
	std::ostringstream command;
	command << "snapSelected -grid " << GlobalGrid().getGridSize();
	UndoableCommand undo(command.str());

	if (GlobalSelectionSystem().Mode() == SelectionSystem::eComponent) {
		// Component mode
		ComponentSnappableSnapToGridSelected walker(GlobalGrid().getGridSize());
		GlobalSelectionSystem().foreachSelectedComponent(walker);
	}
	else {
		// Non-component mode
		SnappableSnapToGridSelected walker(GlobalGrid().getGridSize());
		GlobalSelectionSystem().foreachSelected(walker);
	}
}

void XY_UpdateAllWindows()
{
  if(g_pParentWnd != 0)
  {
    GlobalXYWnd().updateAllViews();
  }
}

void UpdateAllWindows()
{
  GlobalCamera().update();
  XY_UpdateAllWindows();
}


void ModeChangeNotify()
{
  SceneChangeNotify();
}

void ClipperChangeNotify() {
	UpdateAllWindows();
}


GtkWidget* g_toggle_z_item = 0;
GtkWidget* g_toggle_console_item = 0;
GtkWidget* g_toggle_entity_item = 0;
GtkWidget* g_toggle_entitylist_item = 0;

// The "Flush & Reload Shaders" command target 
void RefreshShaders() {
	ScopeDisableScreenUpdates disableScreenUpdates("Processing...", "Loading Shaders");
	
	// Destroy all the OpenGLShader objects
	GlobalShaderCache().unrealise();
	
	// Reload the Shadersystem
	GlobalShaderSystem().refresh();
	
	// Now realise the OpenGLShader objects again
	GlobalShaderCache().realise();
	
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

#include "ui/mainframe/MainFrame.h"

// =============================================================================
// MainFrame class

ui::MainFrame* g_pParentWnd = 0;

GtkWindow* MainFrame_getWindow()
{
  if(g_pParentWnd == 0)
  {
  	return 0;
  }
  return g_pParentWnd->_window;
}

int getFarClipDistance() {
	return getCameraSettings()->cubicScale();
}

void Layout_registerPreferencesPage() {
	PreferencesPagePtr page = GlobalPreferenceSystem().getPage("Interface");
	
	IconList icons;
	IconDescriptionList descriptions;
	
	icons.push_back("layout_regular.png"); descriptions.push_back("Regular");
	icons.push_back("layout_floating.png"); descriptions.push_back("Floating");
	icons.push_back("layout_splitpane.png"); descriptions.push_back("Split");
	icons.push_back("layout_regular_left.png"); descriptions.push_back("Regular Left");
	
	page->appendRadioIcons("", RKEY_WINDOW_LAYOUT, icons, descriptions);
	page->appendLabel("<b>Note</b>: You will have to restart DarkRadiant for the changes to take effect.");

	// Add another page for Multi-Monitor stuff
	page = GlobalPreferenceSystem().getPage("Interface/Multi Monitor");
	
	page->appendCheckBox("", "Start on Primary Monitor", RKEY_MULTIMON_START_PRIMARY);
}

#include "stringio.h"

void MainFrame_Construct()
{
	DragMode();

	GlobalEventManager().addCommand("ReloadSkins", FreeCaller<ReloadSkins>());
	GlobalEventManager().addCommand("ProjectSettings", FreeCaller<ui::PrefDialog::showProjectSettings>());
	GlobalEventManager().addCommand("Exit", FreeCaller<Exit>());
	
	GlobalEventManager().addCommand("Undo", FreeCaller<Undo>());
	GlobalEventManager().addCommand("Redo", FreeCaller<Redo>());
	GlobalEventManager().addCommand("Copy", FreeCaller<Copy>());
	GlobalEventManager().addCommand("Paste", FreeCaller<Paste>());
	GlobalEventManager().addCommand("PasteToCamera", FreeCaller<PasteToCamera>());
	GlobalEventManager().addCommand("CloneSelection", FreeCaller<selection::algorithm::cloneSelected>(), true); // react on keyUp
	GlobalEventManager().addCommand("DeleteSelection", FreeCaller<selection::algorithm::deleteSelectionCmd>());
	GlobalEventManager().addCommand("ParentSelection", FreeCaller<selection::algorithm::parentSelection>());
	GlobalEventManager().addCommand("ParentSelectionToWorldspawn", FreeCaller<selection::algorithm::parentSelectionToWorldspawn>());

	GlobalEventManager().addCommand("UnSelectSelection", FreeCaller<Selection_Deselect>());
	GlobalEventManager().addCommand("InvertSelection", FreeCaller<selection::algorithm::invertSelection>());

	GlobalEventManager().addCommand("SelectInside", FreeCaller<selection::algorithm::selectInside>());
	GlobalEventManager().addCommand("SelectTouching", FreeCaller<selection::algorithm::selectTouching>());
	GlobalEventManager().addCommand("SelectCompleteTall", FreeCaller<selection::algorithm::selectCompleteTall>());

	GlobalEventManager().addCommand("ExpandSelectionToEntities", FreeCaller<selection::algorithm::expandSelectionToEntities>());
	GlobalEventManager().addCommand("SelectChildren", FreeCaller<selection::algorithm::selectChildren>());
	GlobalEventManager().addCommand("Preferences", FreeCaller<ui::PrefDialog::toggle>());
	
	GlobalEventManager().addCommand("ToggleConsole", FreeCaller<ui::Console::toggle>());
	
	// Entity inspector (part of Group Dialog)
	GlobalEventManager().addCommand("ToggleEntityInspector",
		FreeCaller<ui::EntityInspector::toggle>());

	GlobalEventManager().addCommand("ToggleMediaBrowser",
		FreeCaller<ui::MediaBrowser::toggle>());
	
	// Light inspector
	GlobalEventManager().addCommand("ToggleLightInspector",	
							FreeCaller<ui::LightInspector::toggleInspector>());
	
	GlobalEventManager().addCommand("SurfaceInspector", FreeCaller<ui::SurfaceInspector::toggle>());
	GlobalEventManager().addCommand("PatchInspector", FreeCaller<ui::PatchInspector::toggle>());
	
	// Overlay dialog
	GlobalEventManager().addCommand("OverlayDialog",
									FreeCaller<ui::OverlayDialog::display>());
	
	GlobalEventManager().addCommand("ShowHidden", FreeCaller<selection::algorithm::showAllHidden>());
	GlobalEventManager().addCommand("HideSelected", FreeCaller<selection::algorithm::hideSelected>());
	GlobalEventManager().addCommand("HideDeselected", FreeCaller<selection::algorithm::hideDeselected>());
	
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
	GlobalEventManager().addCommand("MapInfo", FreeCaller<ui::MapInfoDialog::showDialog>());
	GlobalEventManager().addCommand("EditFiltersDialog", FreeCaller<ui::FilterDialog::showDialog>());
	
	GlobalEventManager().addRegistryToggle("ToggleShowSizeInfo", RKEY_SHOW_SIZE_INFO);
	GlobalEventManager().addRegistryToggle("ToggleShowAllLightRadii", "user/ui/showAllLightRadii");
	GlobalEventManager().addRegistryToggle("ToggleShowAllSpeakerRadii", "user/ui/showAllSpeakerRadii");

	GlobalEventManager().addToggle("ToggleClipper", FreeCaller<ClipperMode>());
	
	GlobalEventManager().addToggle("MouseTranslate", FreeCaller<TranslateMode>());
	GlobalEventManager().addToggle("MouseRotate", FreeCaller<RotateMode>());
	//GlobalEventManager().addToggle("MouseScale", FreeCaller<ScaleMode>());
	GlobalEventManager().addToggle("MouseDrag", FreeCaller<DragMode>());
	
	GlobalEventManager().addCommand("CSGSubtract", FreeCaller<brush::algorithm::subtractBrushesFromUnselected>());
	GlobalEventManager().addCommand("CSGMerge", FreeCaller<brush::algorithm::mergeSelectedBrushes>());
	GlobalEventManager().addCommand("CSGHollow", FreeCaller<brush::algorithm::hollowSelectedBrushes>());
	GlobalEventManager().addCommand("CSGRoom", FreeCaller<brush::algorithm::makeRoomForSelectedBrushes>());
	
	GlobalEventManager().addCommand("RefreshShaders", FreeCaller<RefreshShaders>());
	
	GlobalEventManager().addCommand("SnapToGrid", FreeCaller<Selection_SnapToGrid>());
	
	GlobalEventManager().addCommand("SelectAllOfType", FreeCaller<selection::algorithm::selectAllOfType>());
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
	
	GlobalEventManager().addCommand(
		"CurveAppendControlPoint", 
		FreeCaller<selection::algorithm::appendCurveControlPoint>()
	);
	GlobalEventManager().addCommand(
		"CurveRemoveControlPoint", 
		FreeCaller<selection::algorithm::removeCurveControlPoints>()
	);
	GlobalEventManager().addCommand(
		"CurveInsertControlPoint", 
		FreeCaller<selection::algorithm::insertCurveControlPoints>()
	);
	GlobalEventManager().addCommand(
		"CurveConvertType", 
		FreeCaller<selection::algorithm::convertCurveTypes>()
	);
	
	GlobalEventManager().addCommand("BrushExportOBJ", FreeCaller<CallBrushExportOBJ>());
	GlobalEventManager().addCommand("BrushExportCM", FreeCaller<selection::algorithm::createCMFromSelection>());
	
	GlobalEventManager().addCommand(
		"CreateDecalsForFaces",
		FreeCaller<selection::algorithm::createDecalsForSelectedFaces>()
	);

	GlobalEventManager().addCommand("FindReplaceTextures", FreeCaller<ui::FindAndReplaceShader::showDialog>());
	GlobalEventManager().addCommand("ShowCommandList", FreeCaller<ui::CommandList::showDialog>());
	GlobalEventManager().addCommand("About", FreeCaller<ui::AboutDialog::showDialog>());

	ui::LayerControlDialog::registerCommands();

	ui::TexTool::registerCommands();

  Patch_registerCommands();

  typedef FreeCaller1<const Selectable&, ComponentMode_SelectionChanged> ComponentModeSelectionChangedCaller;
  GlobalSelectionSystem().addSelectionChangeCallback(ComponentModeSelectionChangedCaller());

  Layout_registerPreferencesPage();
}
