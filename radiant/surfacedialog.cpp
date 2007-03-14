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
// Surface Dialog
//
// Leonardo Zide (leo@lokigames.com)
//

#include "surfacedialog.h"

#include "select.h"
#include "generic/callback.h"
#include "ieventmanager.h"
#include "brush/FaceInstance.h"
#include "ui/surfaceinspector/SurfaceInspector.h"
#include "selection/algorithm/Shader.h"
#include "textool/TexTool.h"

extern FaceInstanceSet g_SelectedFaceInstances;

void SurfaceInspector_toggleShown() {
	ui::SurfaceInspector::Instance().toggle();
}

bool SelectedFaces_empty() {
	return g_SelectedFaceInstances.empty();
}

void ToggleTexTool() {
	// Call the toggle() method of the static instance
	ui::TexTool::Instance().toggle();
}

void TexToolGridUp() {
	ui::TexTool::Instance().gridUp();
}

void TexToolGridDown() {
	ui::TexTool::Instance().gridDown();
}

void TexToolSnapToGrid() {
	ui::TexTool::Instance().snapToGrid();
}

void TexToolMergeItems() {
	ui::TexTool::Instance().mergeSelectedItems();
}

void TexToolFlipS() {
	ui::TexTool::Instance().flipSelected(0);
}

void TexToolFlipT() {
	ui::TexTool::Instance().flipSelected(1);
}

void SurfaceInspector_registerCommands()
{
  GlobalEventManager().addCommand("NormaliseTexture", FreeCaller<selection::algorithm::normaliseTexture>());
  GlobalEventManager().addCommand("SurfaceInspector", FreeCaller<SurfaceInspector_toggleShown>());
  GlobalEventManager().addCommand("TextureTool", FreeCaller<ToggleTexTool>());

	GlobalEventManager().addCommand("CopyShader", FreeCaller<selection::algorithm::pickShaderFromSelection>());
	GlobalEventManager().addCommand("PasteShader", FreeCaller<selection::algorithm::pasteShaderToSelection>());
	GlobalEventManager().addCommand("PasteShaderNatural", FreeCaller<selection::algorithm::pasteShaderNaturalToSelection>());
  
	GlobalEventManager().addCommand("FlipTextureX", FreeCaller<selection::algorithm::flipTextureS>());
	GlobalEventManager().addCommand("FlipTextureY", FreeCaller<selection::algorithm::flipTextureT>());
  
  GlobalEventManager().addCommand("TexToolGridUp", FreeCaller<TexToolGridUp>());
  GlobalEventManager().addCommand("TexToolGridDown", FreeCaller<TexToolGridDown>());
  GlobalEventManager().addCommand("TexToolSnapToGrid", FreeCaller<TexToolSnapToGrid>());
  GlobalEventManager().addCommand("TexToolMergeItems", FreeCaller<TexToolMergeItems>());
  GlobalEventManager().addCommand("TexToolFlipS", FreeCaller<TexToolFlipS>());
  GlobalEventManager().addCommand("TexToolFlipT", FreeCaller<TexToolFlipT>());
  GlobalEventManager().addRegistryToggle("TexToolToggleGrid", "user/ui/textures/texTool/gridActive");
}

void SurfaceInspector_Construct() {
	SurfaceInspector_registerCommands();
}
