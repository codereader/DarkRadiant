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
// XY Window
//
// Leonardo Zide (leo@lokigames.com)
//

#include "xywindow.h"

#include "debugging/debugging.h"

#include "iclipper.h"
#include "ientity.h"
#include "ieclass.h"
#include "igl.h"
#include "brush/TexDef.h"
#include "ibrush.h"
#include "iundo.h"
#include "iregistry.h"

#include <gtk/gtklabel.h>
#include <gtk/gtkmenuitem.h>

#include "generic/callback.h"
#include "string/string.h"
#include "stream/stringstream.h"

#include "scenelib.h"
#include "renderer.h"
#include "moduleobserver.h"

#include "gtkutil/menu.h"
#include "gtkutil/container.h"
#include "gtkutil/widget.h"
#include "gtkutil/glwidget.h"
#include "gtkmisc.h"
#include "select.h"
#include "csg.h"
#include "brushmanip.h"
#include "entity.h"
#include "camera/GlobalCamera.h"
#include "texwindow.h"
#include "mainframe.h"
#include "preferences.h"
#include "commands.h"
#include "grid.h"
#include "windowobservers.h"
#include "plugin.h"
#include "ui/colourscheme/ColourScheme.h"
#include "ui/eventmapper/EventMapper.h"

#include "selection/SelectionBox.h"

#include "xyview/XYWnd.h"

EViewType GlobalXYWnd_getCurrentViewType()
{
  ASSERT_NOTNULL(g_pParentWnd);
  ASSERT_NOTNULL(g_pParentWnd->ActiveXY());
  return g_pParentWnd->ActiveXY()->getViewType();
}

// =============================================================================
// variables

bool g_bCrossHairs = false;

void GetFocusPosition(Vector3& position)
{
  if(GlobalSelectionSystem().countSelected() != 0)
  {
    Select_GetMid(position);
  }
  else
  {
    position = g_pParentWnd->GetCamWnd()->getCameraOrigin();
  }
}

void XYWnd_Focus(XYWnd* xywnd)
{
  Vector3 position;
  GetFocusPosition(position);
  xywnd->positionView(position);
}

void XY_Split_Focus()
{
  Vector3 position;
  GetFocusPosition(position);
  g_pParentWnd->GetXYWnd()->positionView(position);
  g_pParentWnd->GetXZWnd()->positionView(position);
  g_pParentWnd->GetYZWnd()->positionView(position);
}

void XY_Focus()
{
  XYWnd* xywnd = g_pParentWnd->GetXYWnd();
  XYWnd_Focus(xywnd);
}

void XY_Top()
{
  XYWnd* xywnd = g_pParentWnd->GetXYWnd();
  xywnd->setViewType(XY);
  XYWnd_Focus(xywnd);
}

void XY_Side()
{
  XYWnd* xywnd = g_pParentWnd->GetXYWnd();
  xywnd->setViewType(XZ);
  XYWnd_Focus(xywnd);
}

void XY_Front()
{
  g_pParentWnd->GetXYWnd()->setViewType(YZ);
  XYWnd_Focus(g_pParentWnd->GetXYWnd());
}

void XY_Next()
{
  XYWnd* xywnd = g_pParentWnd->GetXYWnd();
  if (xywnd->getViewType() == XY)
    xywnd->setViewType(XZ);
  else if (xywnd->getViewType() ==  XZ)
    xywnd->setViewType(YZ);
  else
    xywnd->setViewType(XY);
  XYWnd_Focus(xywnd);
}

void XY_Zoom100()
{
  if (g_pParentWnd->GetXYWnd())
    g_pParentWnd->GetXYWnd()->setScale(1);
  if (g_pParentWnd->GetXZWnd())
    g_pParentWnd->GetXZWnd()->setScale(1);
  if (g_pParentWnd->GetYZWnd())
    g_pParentWnd->GetYZWnd()->setScale(1);
}

void XY_ZoomIn() {
	g_pParentWnd->ActiveXY()->zoomIn();
}

// NOTE: the zoom out factor is 4/5, we could think about customizing it
//  we don't go below a zoom factor corresponding to 10% of the max world size
//  (this has to be computed against the window size)
void XY_ZoomOut() {
	g_pParentWnd->ActiveXY()->zoomOut();
}



void ToggleShowCrosshair()
{
  g_bCrossHairs ^= 1; 
  XY_UpdateAllWindows();
}

void ToggleShowGrid()
{
  g_xywindow_globals_private.d_showgrid = !g_xywindow_globals_private.d_showgrid;
  XY_UpdateAllWindows();
}

ToggleShown g_xy_top_shown(true);

void XY_Top_Shown_Construct(GtkWindow* parent)
{
  g_xy_top_shown.connect(GTK_WIDGET(parent));
}

ToggleShown g_yz_side_shown(false);

void YZ_Side_Shown_Construct(GtkWindow* parent)
{
  g_yz_side_shown.connect(GTK_WIDGET(parent));
}

ToggleShown g_xz_front_shown(false);

void XZ_Front_Shown_Construct(GtkWindow* parent)
{
  g_xz_front_shown.connect(GTK_WIDGET(parent));
}

void ShowNamesToggle()
{
  GlobalEntityCreator().setShowNames(!GlobalEntityCreator().getShowNames());
  XY_UpdateAllWindows();
}
typedef FreeCaller<ShowNamesToggle> ShowNamesToggleCaller;
void ShowNamesExport(const BoolImportCallback& importer)
{
  importer(GlobalEntityCreator().getShowNames());
}
typedef FreeCaller1<const BoolImportCallback&, ShowNamesExport> ShowNamesExportCaller;

void ShowAnglesToggle()
{
  GlobalEntityCreator().setShowAngles(!GlobalEntityCreator().getShowAngles());
  XY_UpdateAllWindows();
}
typedef FreeCaller<ShowAnglesToggle> ShowAnglesToggleCaller;
void ShowAnglesExport(const BoolImportCallback& importer)
{
  importer(GlobalEntityCreator().getShowAngles());
}
typedef FreeCaller1<const BoolImportCallback&, ShowAnglesExport> ShowAnglesExportCaller;

void ShowBlocksToggle()
{
  g_xywindow_globals_private.show_blocks ^= 1;
  XY_UpdateAllWindows();
}
typedef FreeCaller<ShowBlocksToggle> ShowBlocksToggleCaller;
void ShowBlocksExport(const BoolImportCallback& importer)
{
  importer(g_xywindow_globals_private.show_blocks);
}
typedef FreeCaller1<const BoolImportCallback&, ShowBlocksExport> ShowBlocksExportCaller;

void ShowCoordinatesToggle()
{
  g_xywindow_globals_private.show_coordinates ^= 1;
  XY_UpdateAllWindows();
}
typedef FreeCaller<ShowCoordinatesToggle> ShowCoordinatesToggleCaller;
void ShowCoordinatesExport(const BoolImportCallback& importer)
{
  importer(g_xywindow_globals_private.show_coordinates);
}
typedef FreeCaller1<const BoolImportCallback&, ShowCoordinatesExport> ShowCoordinatesExportCaller;

void ShowOutlineToggle()
{
  g_xywindow_globals_private.show_outline ^= 1;
  XY_UpdateAllWindows();
}
typedef FreeCaller<ShowOutlineToggle> ShowOutlineToggleCaller;
void ShowOutlineExport(const BoolImportCallback& importer)
{
  importer(g_xywindow_globals_private.show_outline);
}
typedef FreeCaller1<const BoolImportCallback&, ShowOutlineExport> ShowOutlineExportCaller;

void ShowAxesToggle()
{
  g_xywindow_globals_private.show_axis ^= 1;
  XY_UpdateAllWindows();
}
typedef FreeCaller<ShowAxesToggle> ShowAxesToggleCaller;
void ShowAxesExport(const BoolImportCallback& importer)
{
  importer(g_xywindow_globals_private.show_axis);
}
typedef FreeCaller1<const BoolImportCallback&, ShowAxesExport> ShowAxesExportCaller;

void ShowWorkzoneToggle()
{
  g_xywindow_globals_private.d_show_work ^= 1;
  XY_UpdateAllWindows();
}
typedef FreeCaller<ShowWorkzoneToggle> ShowWorkzoneToggleCaller;
void ShowWorkzoneExport(const BoolImportCallback& importer)
{
  importer(g_xywindow_globals_private.d_show_work);
}
typedef FreeCaller1<const BoolImportCallback&, ShowWorkzoneExport> ShowWorkzoneExportCaller;

ShowNamesExportCaller g_show_names_caller;
BoolExportCallback g_show_names_callback(g_show_names_caller);
ToggleItem g_show_names(g_show_names_callback);

ShowAnglesExportCaller g_show_angles_caller;
BoolExportCallback g_show_angles_callback(g_show_angles_caller);
ToggleItem g_show_angles(g_show_angles_callback);

ShowBlocksExportCaller g_show_blocks_caller;
BoolExportCallback g_show_blocks_callback(g_show_blocks_caller);
ToggleItem g_show_blocks(g_show_blocks_callback);

ShowCoordinatesExportCaller g_show_coordinates_caller;
BoolExportCallback g_show_coordinates_callback(g_show_coordinates_caller);
ToggleItem g_show_coordinates(g_show_coordinates_callback);

ShowOutlineExportCaller g_show_outline_caller;
BoolExportCallback g_show_outline_callback(g_show_outline_caller);
ToggleItem g_show_outline(g_show_outline_callback);

ShowAxesExportCaller g_show_axes_caller;
BoolExportCallback g_show_axes_callback(g_show_axes_caller);
ToggleItem g_show_axes(g_show_axes_callback);

ShowWorkzoneExportCaller g_show_workzone_caller;
BoolExportCallback g_show_workzone_callback(g_show_workzone_caller);
ToggleItem g_show_workzone(g_show_workzone_callback);

void XYShow_registerCommands()
{
  GlobalToggles_insert("ShowAngles", ShowAnglesToggleCaller(), ToggleItem::AddCallbackCaller(g_show_angles));
  GlobalToggles_insert("ShowNames", ShowNamesToggleCaller(), ToggleItem::AddCallbackCaller(g_show_names));
  GlobalToggles_insert("ShowBlocks", ShowBlocksToggleCaller(), ToggleItem::AddCallbackCaller(g_show_blocks));
  GlobalToggles_insert("ShowCoordinates", ShowCoordinatesToggleCaller(), ToggleItem::AddCallbackCaller(g_show_coordinates));
  GlobalToggles_insert("ShowWindowOutline", ShowOutlineToggleCaller(), ToggleItem::AddCallbackCaller(g_show_outline));
  GlobalToggles_insert("ShowAxes", ShowAxesToggleCaller(), ToggleItem::AddCallbackCaller(g_show_axes));
  GlobalToggles_insert("ShowWorkzone", ShowWorkzoneToggleCaller(), ToggleItem::AddCallbackCaller(g_show_workzone));
}

void XYWnd_registerShortcuts()
{
  command_connect_accelerator("ToggleCrosshairs");
}

void Orthographic_constructPreferences(PrefPage* page)
{
  page->appendCheckBox("", "Solid selection boxes", g_xywindow_globals.m_bNoStipple);
  page->appendCheckBox("", "Chase mouse during drags", g_xywindow_globals_private.m_bChaseMouse);
  page->appendCheckBox("", "Update views on camera move", g_xywindow_globals_private.m_bCamXYUpdate);
}
void Orthographic_constructPage(PreferenceGroup& group)
{
  PreferencesPage* page(group.createPage("Orthographic", "Orthographic View Preferences"));
  Orthographic_constructPreferences(reinterpret_cast<PrefPage*>(page));
}
void Orthographic_registerPreferencesPage()
{
  PreferencesDialog_addSettingsPage(FreeCaller1<PreferenceGroup&, Orthographic_constructPage>());
}

#include "preferencesystem.h"
#include "stringio.h"

void ToggleShown_importBool(ToggleShown& self, bool value)
{
  self.set(value);
}
typedef ReferenceCaller1<ToggleShown, bool, ToggleShown_importBool> ToggleShownImportBoolCaller;
void ToggleShown_exportBool(const ToggleShown& self, const BoolImportCallback& importer)
{
  importer(self.active());
}
typedef ConstReferenceCaller1<ToggleShown, const BoolImportCallback&, ToggleShown_exportBool> ToggleShownExportBoolCaller;


void XYWindow_Construct()
{
  GlobalCommands_insert("ToggleCrosshairs", FreeCaller<ToggleShowCrosshair>(), Accelerator('X', (GdkModifierType)GDK_SHIFT_MASK));
  GlobalCommands_insert("ToggleGrid", FreeCaller<ToggleShowGrid>(), Accelerator('0'));

  GlobalToggles_insert("ToggleView", ToggleShown::ToggleCaller(g_xy_top_shown), ToggleItem::AddCallbackCaller(g_xy_top_shown.m_item), Accelerator('V', (GdkModifierType)(GDK_SHIFT_MASK|GDK_CONTROL_MASK)));
  GlobalToggles_insert("ToggleSideView", ToggleShown::ToggleCaller(g_yz_side_shown), ToggleItem::AddCallbackCaller(g_yz_side_shown.m_item));
  GlobalToggles_insert("ToggleFrontView", ToggleShown::ToggleCaller(g_xz_front_shown), ToggleItem::AddCallbackCaller(g_xz_front_shown.m_item));
  GlobalCommands_insert("NextView", FreeCaller<XY_Next>(), Accelerator(GDK_Tab, (GdkModifierType)GDK_CONTROL_MASK));
  GlobalCommands_insert("ZoomIn", FreeCaller<XY_ZoomIn>(), Accelerator(GDK_Delete));
  GlobalCommands_insert("ZoomOut", FreeCaller<XY_ZoomOut>(), Accelerator(GDK_Insert));
  GlobalCommands_insert("ViewTop", FreeCaller<XY_Top>());
  GlobalCommands_insert("ViewSide", FreeCaller<XY_Side>());
  GlobalCommands_insert("ViewFront", FreeCaller<XY_Front>());
  GlobalCommands_insert("Zoom100", FreeCaller<XY_Zoom100>());
  GlobalCommands_insert("CenterXYViews", FreeCaller<XY_Split_Focus>(), Accelerator(GDK_Tab, (GdkModifierType)(GDK_SHIFT_MASK|GDK_CONTROL_MASK)));
  GlobalCommands_insert("CenterXYView", FreeCaller<XY_Focus>(), Accelerator(GDK_Tab, (GdkModifierType)(GDK_SHIFT_MASK|GDK_CONTROL_MASK)));

  GlobalPreferenceSystem().registerPreference("ChaseMouse", BoolImportStringCaller(g_xywindow_globals_private.m_bChaseMouse), BoolExportStringCaller(g_xywindow_globals_private.m_bChaseMouse));
  GlobalPreferenceSystem().registerPreference("NoStipple", BoolImportStringCaller(g_xywindow_globals.m_bNoStipple), BoolExportStringCaller(g_xywindow_globals.m_bNoStipple));
  GlobalPreferenceSystem().registerPreference("SI_ShowCoords", BoolImportStringCaller(g_xywindow_globals_private.show_coordinates), BoolExportStringCaller(g_xywindow_globals_private.show_coordinates));
  GlobalPreferenceSystem().registerPreference("SI_ShowOutlines", BoolImportStringCaller(g_xywindow_globals_private.show_outline), BoolExportStringCaller(g_xywindow_globals_private.show_outline));
  GlobalPreferenceSystem().registerPreference("SI_ShowAxis", BoolImportStringCaller(g_xywindow_globals_private.show_axis), BoolExportStringCaller(g_xywindow_globals_private.show_axis));
  GlobalPreferenceSystem().registerPreference("CamXYUpdate", BoolImportStringCaller(g_xywindow_globals_private.m_bCamXYUpdate), BoolExportStringCaller(g_xywindow_globals_private.m_bCamXYUpdate));

  GlobalPreferenceSystem().registerPreference("XZVIS", makeBoolStringImportCallback(ToggleShownImportBoolCaller(g_xz_front_shown)), makeBoolStringExportCallback(ToggleShownExportBoolCaller(g_xz_front_shown)));
  GlobalPreferenceSystem().registerPreference("YZVIS", makeBoolStringImportCallback(ToggleShownImportBoolCaller(g_yz_side_shown)), makeBoolStringExportCallback(ToggleShownExportBoolCaller(g_yz_side_shown)));

  Orthographic_registerPreferencesPage();  

  XYWnd::captureStates();
}

void XYWindow_Destroy() {
	XYWnd::releaseStates();
}
