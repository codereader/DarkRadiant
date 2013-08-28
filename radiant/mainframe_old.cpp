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

#include "i18n.h"
#include "imainframe.h"
#include "debugging/debugging.h"
#include "version.h"
#include "settings/PreferenceSystem.h"

#include "map/FindMapElements.h"
#include "ui/about/AboutDialog.h"
#include "ui/surfaceinspector/SurfaceInspector.h"
#include "ui/prefdialog/PrefDialog.h"
#include "ui/patch/PatchInspector.h"
#include "textool/TexTool.h"
#include "ui/lightinspector/LightInspector.h"
#include "ui/mediabrowser/MediaBrowser.h"
#include "ui/menu/FiltersMenu.h"
#include "ui/transform/TransformDialog.h"
#include "ui/mainframe/ScreenUpdateBlocker.h"
#include "ui/overlay/Overlay.h"
#include "ui/overlay/OverlayDialog.h"
#include "ui/layers/LayerControlDialog.h"
#include "ui/filterdialog/FilterDialog.h"
#include "ui/animationpreview/MD5AnimationViewer.h"
#include "selection/algorithm/Shader.h"
#include "selection/algorithm/General.h"
#include "selection/algorithm/Group.h"
#include "selection/algorithm/GroupCycle.h"
#include "selection/algorithm/Primitives.h"
#include "selection/algorithm/Transformation.h"
#include "selection/algorithm/Curves.h"
#include "selection/shaderclipboard/ShaderClipboard.h"
#include "selection/clipboard/Clipboard.h"
#include "iclipper.h"
#include "ifilesystem.h"
#include "iundo.h"
#include "igrid.h"
#include "ifilter.h"
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
#include "os/dir.h"

#include <ctime>
#include <iostream>

#include "scenelib.h"
#include "os/path.h"
#include "os/file.h"
#include "moduleobservers.h"

#include "gtkutil/clipboard.h"
#include "gtkutil/GLWidget.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/FramedWidget.h"
#include "gtkutil/dialog/MessageBox.h"
#include "gtkutil/SourceView.h"

#include "map/AutoSaver.h"
#include "brush/BrushModule.h"
#include "brush/csg/CSG.h"
#include "log/Console.h"
#include "entity.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "camera/GlobalCamera.h"
#include "camera/CameraSettings.h"
#include "xyview/GlobalXYWnd.h"
#include "ui/commandlist/CommandList.h"
#include "ui/findshader/FindShader.h"
#include "ui/mapinfo/MapInfoDialog.h"
#include "ui/splash/Splash.h"
#include "brush/FaceInstance.h"
#include "settings/GameManager.h"
#include "modulesystem/ModuleRegistry.h"
#include "RadiantModule.h"

#include "imd5anim.h"

#if 0
#include "debugging/ScopedDebugTimer.h"

void BenchmarkPatches(const cmd::ArgumentList& args) {
	// Disable screen updates for the scope of this function
	ui::ScreenUpdateBlocker blocker("Processing...", "Performing Patch Benchmark");

	scene::INodePtr node = GlobalPatchCreator(DEF2).createPatch();
	Patch* patch = Node_getPatch(node);
	GlobalMap().findOrInsertWorldspawn()->addChildNode(node);

	patch->setDims(15, 15);

	// Retrieve the boundaries
	AABB bounds(Vector3(-512, -512, 0), Vector3(512, 512, 0));
	patch->ConstructPrefab(bounds, ePlane, GlobalXYWnd().getActiveViewType(), 15, 15);

	for (PatchControlIter i = patch->begin(); i != patch->end(); ++i)
	{
		PatchControl& control = *i;
		int randomNumber = int(250 * (float(std::rand()) / float(RAND_MAX)));
		control.vertex.set(control.vertex.x(), control.vertex.y(), control.vertex.z() + randomNumber);
	}

	patch->controlPointsChanged();

	{
		ScopedDebugTimer timer("Benchmark");
		for (std::size_t i = 0; i < 200; i++)
		{
			patch->UpdateCachedData();
		}
	}
}
#endif

void MainFrame_Construct()
{
#if 0
	GlobalCommandSystem().addCommand("BenchmarkPatches", BenchmarkPatches);
#endif

	GlobalCommandSystem().addCommand("Exit", radiant::RadiantModule::exitCmd);
	GlobalCommandSystem().addCommand("ReloadSkins", ReloadSkins);
	GlobalCommandSystem().addCommand("ReloadDefs", ReloadDefs);
	GlobalCommandSystem().addCommand("ProjectSettings", ui::PrefDialog::showProjectSettings);
	GlobalCommandSystem().addCommand("Copy", selection::clipboard::copy);
	GlobalCommandSystem().addCommand("Paste", selection::clipboard::paste);
	GlobalCommandSystem().addCommand("PasteToCamera", selection::clipboard::pasteToCamera);

	GlobalCommandSystem().addCommand("Preferences", ui::PrefDialog::toggle);
	GlobalCommandSystem().addCommand("ToggleConsole", ui::Console::toggle);

	GlobalCommandSystem().addCommand("ToggleMediaBrowser", ui::MediaBrowser::toggle);
	GlobalCommandSystem().addCommand("ToggleLightInspector", ui::LightInspector::toggleInspector);
	GlobalCommandSystem().addCommand("SurfaceInspector", ui::SurfaceInspector::toggle);
	GlobalCommandSystem().addCommand("PatchInspector", ui::PatchInspector::toggle);
	GlobalCommandSystem().addCommand("OverlayDialog", ui::OverlayDialog::display);
	GlobalCommandSystem().addCommand("TransformDialog", ui::TransformDialog::toggle);

	GlobalCommandSystem().addCommand("FindBrush", DoFind);
	
	GlobalCommandSystem().addCommand("MapInfo", ui::MapInfoDialog::showDialog);
	GlobalCommandSystem().addCommand("EditFiltersDialog", ui::FilterDialog::showDialog);

	GlobalCommandSystem().addCommand("CSGSubtract", brush::algorithm::subtractBrushesFromUnselected);
	GlobalCommandSystem().addCommand("CSGMerge", brush::algorithm::mergeSelectedBrushes);
	GlobalCommandSystem().addCommand("CSGHollow", brush::algorithm::hollowSelectedBrushes);
	GlobalCommandSystem().addCommand("CSGRoom", brush::algorithm::makeRoomForSelectedBrushes);

	GlobalCommandSystem().addCommand("AnimationPreview", ui::MD5AnimationViewer::Show);
	GlobalCommandSystem().addCommand("FindReplaceTextures", ui::FindAndReplaceShader::showDialog);
	GlobalCommandSystem().addCommand("ShowCommandList", ui::CommandList::showDialog);
	GlobalCommandSystem().addCommand("About", ui::AboutDialog::showDialog);

	// ----------------------- Bind Events ---------------------------------------

	GlobalEventManager().addCommand("Exit", "Exit");
	GlobalEventManager().addCommand("ReloadSkins", "ReloadSkins");
	GlobalEventManager().addCommand("ReloadDefs", "ReloadDefs");
	GlobalEventManager().addCommand("ProjectSettings", "ProjectSettings");

	GlobalEventManager().addCommand("Copy", "Copy");
	GlobalEventManager().addCommand("Paste", "Paste");
	GlobalEventManager().addCommand("PasteToCamera", "PasteToCamera");

	GlobalEventManager().addCommand("Preferences", "Preferences");

	GlobalEventManager().addCommand("ToggleConsole", "ToggleConsole");

	GlobalEventManager().addCommand("ToggleMediaBrowser", "ToggleMediaBrowser");
	GlobalEventManager().addCommand("ToggleLightInspector",	"ToggleLightInspector");
	GlobalEventManager().addCommand("SurfaceInspector", "SurfaceInspector");
	GlobalEventManager().addCommand("PatchInspector", "PatchInspector");
	GlobalEventManager().addCommand("OverlayDialog", "OverlayDialog");
	GlobalEventManager().addCommand("TransformDialog", "TransformDialog");

	GlobalEventManager().addCommand("FindBrush", "FindBrush");
	
	GlobalEventManager().addCommand("MapInfo", "MapInfo");
	GlobalEventManager().addCommand("EditFiltersDialog", "EditFiltersDialog");

	GlobalEventManager().addCommand("CSGSubtract", "CSGSubtract");
	GlobalEventManager().addCommand("CSGMerge", "CSGMerge");
	GlobalEventManager().addCommand("CSGHollow", "CSGHollow");
	GlobalEventManager().addCommand("CSGRoom", "CSGRoom");

	GlobalEventManager().addCommand("AnimationPreview", "AnimationPreview");
	GlobalEventManager().addCommand("FindReplaceTextures", "FindReplaceTextures");
	GlobalEventManager().addCommand("ShowCommandList", "ShowCommandList");
	GlobalEventManager().addCommand("About", "About");
}
