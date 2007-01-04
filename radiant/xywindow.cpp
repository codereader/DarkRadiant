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

#include "ieventmanager.h"

#include "generic/callback.h"
#include "gtkutil/widget.h"
#include "commands.h"

#include "xyview/XYWnd.h"
#include "xyview/GlobalXYWnd.h"

void XY_Split_Focus() {
	// Re-position all available views
	GlobalXYWnd().positionAllViews(GlobalXYWnd().getFocusPosition());
}

void XY_Focus() {
	GlobalXYWnd().positionView(GlobalXYWnd().getFocusPosition());
}

void XY_Top() {
	GlobalXYWnd().setActiveViewType(XY);
	GlobalXYWnd().positionView(GlobalXYWnd().getFocusPosition());
}

void XY_Side() {
	GlobalXYWnd().setActiveViewType(XZ);
	GlobalXYWnd().positionView(GlobalXYWnd().getFocusPosition());
}

void XY_Front() {
	GlobalXYWnd().setActiveViewType(YZ);
	GlobalXYWnd().positionView(GlobalXYWnd().getFocusPosition());
}

void XY_Zoom100() {
	GlobalXYWnd().setScale(1);
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
  GlobalToggles_insert("ToggleView", ToggleShown::ToggleCaller(g_xy_top_shown), ToggleItem::AddCallbackCaller(g_xy_top_shown.m_item));
  GlobalToggles_insert("ToggleSideView", ToggleShown::ToggleCaller(g_yz_side_shown), ToggleItem::AddCallbackCaller(g_yz_side_shown.m_item));
  GlobalToggles_insert("ToggleFrontView", ToggleShown::ToggleCaller(g_xz_front_shown), ToggleItem::AddCallbackCaller(g_xz_front_shown.m_item));
  GlobalEventManager().addCommand("NextView", MemberCaller<XYWndManager, &XYWndManager::toggleActiveView>(GlobalXYWnd()));
  GlobalEventManager().addCommand("ZoomIn", MemberCaller<XYWndManager, &XYWndManager::zoomIn>(GlobalXYWnd()));
  GlobalEventManager().addCommand("ZoomOut", MemberCaller<XYWndManager, &XYWndManager::zoomOut>(GlobalXYWnd()));
  GlobalEventManager().addCommand("ViewTop", FreeCaller<XY_Top>());
  GlobalEventManager().addCommand("ViewSide", FreeCaller<XY_Side>());
  GlobalEventManager().addCommand("ViewFront", FreeCaller<XY_Front>());
  GlobalEventManager().addCommand("Zoom100", FreeCaller<XY_Zoom100>());
  GlobalEventManager().addCommand("CenterXYViews", FreeCaller<XY_Split_Focus>());
  GlobalEventManager().addCommand("CenterXYView", FreeCaller<XY_Focus>());

  GlobalPreferenceSystem().registerPreference("XZVIS", makeBoolStringImportCallback(ToggleShownImportBoolCaller(g_xz_front_shown)), makeBoolStringExportCallback(ToggleShownExportBoolCaller(g_xz_front_shown)));
  GlobalPreferenceSystem().registerPreference("YZVIS", makeBoolStringImportCallback(ToggleShownImportBoolCaller(g_yz_side_shown)), makeBoolStringExportCallback(ToggleShownExportBoolCaller(g_yz_side_shown)));

  XYWnd::captureStates();
}

void XYWindow_Destroy() {
	XYWnd::releaseStates();
}
