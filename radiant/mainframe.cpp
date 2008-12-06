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
  CamWnd& camwnd = *g_pParentWnd->GetCamWnd();
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

class SnappableSnapToGridSelected : 
	public scene::Graph::Walker
{
  float m_snap;
public:
  SnappableSnapToGridSelected(float snap)
    : m_snap(snap)
  {}
  
  bool pre(const scene::Path& path, const scene::INodePtr& node) const {
	if(path.top()->visible()) {
		SnappablePtr snappable = Node_getSnappable(path.top());
		if (snappable != NULL && Node_getSelectable(node)->isSelected()) {
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
class ComponentSnappableSnapToGridSelected : 
	public scene::Graph::Walker
{
	float _snap;
public:
	// Constructor: expects the grid size that should be snapped to
	ComponentSnappableSnapToGridSelected(float snap) : 
		_snap(snap)
	{}
  
	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
	    if (node->visible()) {
	    	// Check if the visited instance is componentSnappable
			ComponentSnappablePtr componentSnappable = Node_getComponentSnappable(node);
			
			// Call the snapComponents() method if the instance is also a _selected_ Selectable 
			if (componentSnappable != NULL  && Node_isSelected(node)) {
				componentSnappable->snapComponents(_snap);
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
  std::ostringstream command;
  command << "snapSelected -grid " << GlobalGrid().getGridSize();
  UndoableCommand undo(command.str());

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

typedef std::list<std::string> StringStack;
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

MainFrame::MainFrame() : m_window(0), m_idleRedrawStatusText(RedrawStatusTextCaller(*this))
{
  for (int n = 0;n < c_count_status;n++)
  {
    m_pStatusLabel[n] = 0;
  }

  Create();
  
  	// Broadcast the startup event
    radiant::getGlobalRadiant()->broadcastStartupEvent();
}

MainFrame::~MainFrame()
{
  SaveWindowInfo();

  gtk_widget_hide(GTK_WIDGET(m_window));
  
  Shutdown();
  
  gtk_widget_destroy(GTK_WIDGET(m_window));
}

static gint mainframe_delete (GtkWidget *widget, GdkEvent *event, gpointer data) {
	if (GlobalMap().askForSave("Exit Radiant")) {
		gtk_main_quit();
	}

	return TRUE;
}

/* Construct the main Radiant window
 */

void MainFrame::Create()
{
	GtkWindowGroup* windowGroup = gtk_window_group_new();
	
  GtkWindow* window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
  m_window = window;
  radiant::getGlobalRadiant()->setMainWindow(window);
  
  // Tell the XYManager which window the xyviews should be transient for
  GlobalXYWnd().setGlobalParentWindow(window);

  GlobalWindowObservers_connectTopLevel(window);

	gtk_window_set_transient_for(ui::Splash::Instance().getWindow(), window);

#if !defined(WIN32)
	{
		// Set the default icon for POSIX-systems 
		// (Win32 builds use the one embedded in the exe)
		std::string icon = GlobalRegistry().get(RKEY_BITMAPS_PATH) + 
  						   "darkradiant_icon_64x64.png";
		gtk_window_set_default_icon_from_file(icon.c_str(),	NULL);
	}
#endif

  gtk_widget_add_events(GTK_WIDGET(window), GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_FOCUS_CHANGE_MASK);
  g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(mainframe_delete), this);

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
    
    // Get the reference to the MenuManager class
    IMenuManager& menuManager = GlobalUIManager().getMenuManager();
    
    // Retrieve the "main" menubar from the UIManager
    GtkMenuBar* mainMenu = GTK_MENU_BAR(menuManager.get("main"));
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(mainMenu), false, false, 0);
    
    if (m_nCurrentStyle != eFloating) {
    	// Hide the camera toggle option for non-floating views
    	menuManager.setVisibility("main/view/cameraview", false);
    }
    
	if (m_nCurrentStyle != eFloating && m_nCurrentStyle != eSplit) {
		// Hide the console/texture browser toggles for non-floating/non-split views
		menuManager.setVisibility("main/view/consoleView", false);
		menuManager.setVisibility("main/view/textureBrowser", false);	
	}
    
    // Instantiate the ToolbarManager and retrieve the view toolbar widget 
	IToolbarManager& tbCreator = GlobalUIManager().getToolbarManager();
	
	GtkToolbar* viewToolbar = tbCreator.getToolbar("view");
	if (viewToolbar != NULL) {
		// Pack it into the main window
		gtk_widget_show(GTK_WIDGET(viewToolbar));
		gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(viewToolbar), FALSE, FALSE, 0);
	}
	
    GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
    
    // Get the edit toolbar widget 
	GtkToolbar* editToolbar = tbCreator.getToolbar("edit");
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
	//GlobalGroupDialog().construct(window);

    // Add entity inspector widget
    GlobalGroupDialog().addPage(
    	"entity",	// name
    	"Entity", // tab title
    	"cmenu_add_entity.png", // tab icon 
    	ui::EntityInspector::getInstance().getWidget(), // page widget
    	"Entity"
    );

	// Add the Media Browser page
	GlobalGroupDialog().addPage(
    	"mediabrowser",	// name
    	"Media", // tab title
    	"folder16.png", // tab icon 
    	ui::MediaBrowser::getInstance().getWidget(), // page widget
    	"Media"
    );
	
    // Add the console widget if using floating window mode, otherwise the
    // console is placed in the bottom-most split pane.
    if (FloatingGroupDialog()) {
    	GlobalGroupDialog().addPage(
	    	"console",	// name
	    	"Console", // tab title
	    	"iconConsole16.png", // tab icon 
			ui::Console::Instance().getWidget(), // page widget
	    	"Console"
	    );
    }

	int windowState = GDK_WINDOW_STATE_MAXIMIZED;

	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
		windowState = strToInt(windowStateList[0].getAttributeValue("state"));
	}
	
#ifdef WIN32
	// Do the settings say that we should start on the primary screen?
	if (GlobalRegistry().get(RKEY_MULTIMON_START_PRIMARY) == "1") {
		// Yes, connect the position tracker, this overrides the existing setting.
  		_windowPosition.connect(window);
  		// Load the correct coordinates into the position tracker
		_windowPosition.fitToScreen(gtkutil::MultiMonitor::getMonitor(0));
		// Apply the position
		_windowPosition.applyPosition();
	}
	else
#endif
	if (windowState & GDK_WINDOW_STATE_MAXIMIZED) {
		gtk_window_maximize(window);
	}
	else {
		_windowPosition.connect(window);
		_windowPosition.applyPosition();
	}

	gtk_widget_show(GTK_WIDGET(window));

	// Create the camera instance
	GlobalCamera().setParent(window);
	_camWnd = GlobalCamera().getCamWnd();
	
	if (CurrentStyle() == eRegular || CurrentStyle() == eRegularLeft) {
    	// Allocate a new OrthoView and set its ViewType to XY
		XYWndPtr xyWnd = GlobalXYWnd().createEmbeddedOrthoView();
        xyWnd->setViewType(XY);
        // Create a framed window out of the view's internal widget
        GtkWidget* xyView = gtkutil::FramedWidget(xyWnd->getWidget());

        // Pack in the camera window
		GtkWidget* camWindow = gtkutil::FramedWidget(_camWnd->getWidget());
		// greebo: The mainframe window acts as parent for the camwindow
	    _camWnd->setContainer(window);

        // Create the texture window
		GtkWidget* texWindow = gtkutil::FramedWidget(
			GlobalTextureBrowser().constructWindow(window)
		);

        // Create the Console
		GtkWidget* console = ui::Console::Instance().getWidget();
        
        // Now pack those widgets into the paned widgets

        // First, pack the texwindow and the camera
        _regular.texCamPane = gtkutil::Paned(camWindow, texWindow, false);
        
        // Depending on the viewstyle, pack the xy left or right
        if (CurrentStyle() == eRegularLeft) {
        	_regular.horizPane = gtkutil::Paned(_regular.texCamPane, xyView, true);
        }
        else {
        	// This is "regular", put the xyview to the left
        	_regular.horizPane = gtkutil::Paned(xyView, _regular.texCamPane, true);
        }
        
        // Finally, pack the horizontal pane plus the console window into a vpane
        _regular.vertPane = gtkutil::Paned(_regular.horizPane, console, false);
        
        // Add this to the mainframe hbox
		gtk_box_pack_start(GTK_BOX(hbox), _regular.vertPane, TRUE, TRUE, 0);
        gtk_widget_show_all(_regular.vertPane);

        // Set some default values for the width and height
        gtk_paned_set_position(GTK_PANED(_regular.vertPane), 650);
		gtk_paned_set_position(GTK_PANED(_regular.horizPane), 500);
		gtk_paned_set_position(GTK_PANED(_regular.texCamPane), 350);

		// Connect the pane position trackers
		_regular.posVPane.connect(_regular.vertPane);
		_regular.posHPane.connect(_regular.horizPane);
		_regular.posTexCamPane.connect(_regular.texCamPane);

        // Now load the paned positions from the registry
		xml::NodeList list = GlobalRegistry().findXPath("user/ui/mainFrame/regular/pane[@name='vertical']");
	
		if (list.size() > 0) {
			_regular.posVPane.loadFromNode(list[0]);
			_regular.posVPane.applyPosition();
		}
	
		list = GlobalRegistry().findXPath("user/ui/mainFrame/regular/pane[@name='horizontal']");
	
		if (list.size() > 0) {
			_regular.posHPane.loadFromNode(list[0]);
			_regular.posHPane.applyPosition();
		}
	
		list = GlobalRegistry().findXPath("user/ui/mainFrame/regular/pane[@name='texcam']");

		if (list.size() > 0) {
			_regular.posTexCamPane.loadFromNode(list[0]);
			_regular.posTexCamPane.applyPosition();
		}
	} // end if (regular)
  else if (CurrentStyle() == eFloating)
  {
	  
	  gtk_window_group_add_window(windowGroup, window);
	  
    {
     	// Get the floating window with the CamWnd packed into it
		gtkutil::PersistentTransientWindowPtr floatingWindow =
			GlobalCamera().getFloatingWindow();
		GlobalEventManager().connectAccelGroup(
			GTK_WINDOW(floatingWindow->getWindow()));
      
		floatingWindow->show();
		
		gtk_window_group_add_window(windowGroup, GTK_WINDOW(floatingWindow->getWindow()));
    }

   	{
		GtkWidget* page = gtkutil::FramedWidget(
			GlobalTextureBrowser().constructWindow(GTK_WINDOW(GlobalGroupDialog().getDialogWindow()))
		);
		// Add the Media Browser page
		GlobalGroupDialog().addPage(
	    	"textures",	// name
	    	"Textures", // tab title
	    	"icon_texture.png", // tab icon 
	    	GTK_WIDGET(page), // page widget
	    	"Texture Browser"
	    );
		
		gtk_window_group_add_window(windowGroup, GTK_WINDOW(window));
    }

    GlobalGroupDialog().showDialogWindow();
	// greebo: Now that the dialog is shown, tell the Entity Inspector to reload 
	// the position info from the Registry once again.
	ui::EntityInspector::getInstance().restoreSettings();
  }
  else // 4 way (aka Splitplane view)
  {
    GtkWidget* camera = _camWnd->getWidget();
    // greebo: The mainframe window acts as parent for the camwindow
    _camWnd->setContainer(window);

	// Allocate the three ortho views
    XYWndPtr xyWnd = GlobalXYWnd().createEmbeddedOrthoView();
    xyWnd->setViewType(XY);
    GtkWidget* xy = xyWnd->getWidget();
    
    XYWndPtr yzWnd = GlobalXYWnd().createEmbeddedOrthoView();
    yzWnd->setViewType(YZ);
    GtkWidget* yz = yzWnd->getWidget();

    XYWndPtr xzWnd = GlobalXYWnd().createEmbeddedOrthoView();
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
		GtkWidget* textureBrowser = gtkutil::FramedWidget(
			GlobalTextureBrowser().constructWindow(window)
		);

		// Add the Media Browser page
		GlobalGroupDialog().addPage(
	    	"textures",	// name
	    	"Textures", // tab title
	    	"icon_texture.png", // tab icon 
	    	GTK_WIDGET(textureBrowser), // page widget
	    	"Texture Browser"
	    );
    }
  }

  GlobalGrid().addGridChangeCallback(SetGridStatusCaller(*this));
  GlobalGrid().addGridChangeCallback(FreeCaller<XY_UpdateAllWindows>());

  g_defaultToolMode = DragMode;
  g_defaultToolMode();
  SetStatusText(m_command_status, c_TranslateMode_status);

	// Start the autosave timer so that it can periodically check the map for changes 
	map::AutoSaver().startTimer();
  
	// Restore any floating XYViews that were active before, this applies to all view layouts
	GlobalXYWnd().restoreState();
	
	// Initialise the shaderclipboard
	GlobalShaderClipboard().clear();

	ui::LayerControlDialog::init();
}

void MainFrame::SaveWindowInfo() {
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
	node.setAttributeValue(
		"state", 
		intToStr(gdk_window_get_state(GTK_WIDGET(m_window)->window))
	); 
  
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
	else if (CurrentStyle() == eRegular || CurrentStyle() == eRegularLeft) {
		std::string path("user/ui/mainFrame/regular");
		
		// Remove all previously stored pane information 
		GlobalRegistry().deleteXPath(path + "//pane");
		
		xml::Node node = GlobalRegistry().createKeyWithName(path, "pane", "vertical");
		_regular.posVPane.saveToNode(node);
		
		node = GlobalRegistry().createKeyWithName(path, "pane", "horizontal");
		_regular.posHPane.saveToNode(node);
		
		node = GlobalRegistry().createKeyWithName(path, "pane", "texcam");
		_regular.posTexCamPane.saveToNode(node);
	} 
}

void MainFrame::Shutdown()
{
	// Shutdown the console
	ui::Console::Instance().shutdown();

	// Shutdown the texturebrowser (before the GroupDialog gets shut down).
	GlobalTextureBrowser().destroyWindow();
	
	// Broadcast shutdown event to RadiantListeners
	radiant::getGlobalRadiant()->broadcastShutdownEvent();

	// Destroy the Overlay instance
	ui::Overlay::destroyInstance();
	
	// Stop the AutoSaver class from being called
	map::AutoSaver().stopTimer();

  	// Save the camera size to the registry
	GlobalCamera().saveCamWndState();
	
	// Save the current XYViews to the registry
	GlobalXYWnd().saveState();
	GlobalXYWnd().destroyViews();
	
	// greebo: Release the camera window now and advise the
	// camera manager to do so as well.
	_camWnd = CamWndPtr();
	GlobalCamera().destroy();
	
	GlobalXYWnd().destroy();
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

void MainFrame::SetStatusText(std::string& status_text, const std::string& newText)
{
  status_text = newText;
  UpdateStatusText();
}

void Sys_Status(const std::string& statusText) {
	if (g_pParentWnd != 0) {
		g_pParentWnd->SetStatusText(g_pParentWnd->m_command_status, statusText);
	}
}

int getFarClipDistance() {
	return getCameraSettings()->cubicScale();
}

void MainFrame::SetGridStatus()
{
	std::string lock = (GlobalBrush()->textureLockEnabled()) ? "ON" : "OFF";
	std::string text = "G:" + floatToStr(GlobalGrid().getGridSize());
	text += "  C:" + intToStr(getFarClipDistance());
    text += "  L:" + lock;
	SetStatusText(m_grid_status, text);
}

void GridStatus_onTextureLockEnabledChanged()
{
  if(g_pParentWnd != 0)
  {
    g_pParentWnd->SetGridStatus();
  }
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

	ui::TexTool::registerCommands();

  Patch_registerCommands();

  typedef FreeCaller1<const Selectable&, ComponentMode_SelectionChanged> ComponentModeSelectionChangedCaller;
  GlobalSelectionSystem().addSelectionChangeCallback(ComponentModeSelectionChangedCaller());

  Layout_registerPreferencesPage();
}
