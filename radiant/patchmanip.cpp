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

#include "patchmanip.h"

#include "debugging/debugging.h"

#include "i18n.h"
#include "imainframe.h"
#include "ieventmanager.h"
#include "iselection.h"
#include "ipatch.h"

#include "math/AABB.h"
#include "gdk/gdkkeysyms.h"
#include "gtkutil/dialog/MessageBox.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "select.h"
#include "shaderlib.h"
#include "igrid.h"
#include "patch/PatchNode.h"
#include "patch/PatchSceneWalk.h"
#include "patch/PatchCreators.h"
#include "xyview/GlobalXYWnd.h"
#include "ui/patch/BulgePatchDialog.h"
#include "ui/patch/PatchThickenDialog.h"
#include "ui/patch/PatchCreateDialog.h"
#include "ui/surfaceinspector/SurfaceInspector.h"
#include "ui/patch/PatchInspector.h"
#include "selection/algorithm/Primitives.h"
#include "selection/algorithm/General.h"
#include "ui/patch/CapDialog.h"

#include "patch/algorithm/Prefab.h"
#include "patch/algorithm/General.h"
#include "selection/algorithm/Patch.h"

class PatchSelectByShader :
	public scene::NodeVisitor
{
	std::string _name;
public:
	PatchSelectByShader(const std::string& name) :
		_name(name)
	{}

	bool pre(const scene::INodePtr& node) {
		// Don't traverse hidden nodes
		if (!node->visible()) return false;

		Patch* patch = Node_getPatch(node);

		if (patch != NULL && shader_equal(patch->getShader(), _name)) {
			Node_setSelected(node, true);
			return false;
		}

		return true;
	}
};

void Scene_PatchSelectByShader(scene::Graph& graph, const std::string& name) {
	PatchSelectByShader walker(name);
	Node_traverseSubgraph(GlobalSceneGraph().root(), walker);
}

void Patch_registerCommands() 
{
	// First connect the commands to the code
	GlobalCommandSystem().addCommand("CreatePatchPrefab", patch::algorithm::createPrefab, cmd::ARGTYPE_STRING);

	// Two optional integer arguments
	GlobalCommandSystem().addCommand("SimplePatchMesh", patch::algorithm::createSimplePatch,
		cmd::Signature(cmd::ARGTYPE_INT|cmd::ARGTYPE_OPTIONAL, cmd::ARGTYPE_INT|cmd::ARGTYPE_OPTIONAL));

	GlobalCommandSystem().addCommand("PatchInsertColumnEnd", selection::algorithm::insertPatchColumnsAtEnd);
	GlobalCommandSystem().addCommand("PatchInsertColumnBeginning", selection::algorithm::insertPatchColumnsAtBeginning);
	GlobalCommandSystem().addCommand("PatchInsertRowEnd", selection::algorithm::insertPatchRowsAtEnd);
	GlobalCommandSystem().addCommand("PatchInsertRowBeginning", selection::algorithm::insertPatchRowsAtBeginning);

	GlobalCommandSystem().addCommand("PatchDeleteColumnBeginning", selection::algorithm::deletePatchColumnsFromBeginning);
	GlobalCommandSystem().addCommand("PatchDeleteColumnEnd", selection::algorithm::deletePatchColumnsFromEnd);
	GlobalCommandSystem().addCommand("PatchDeleteRowBeginning", selection::algorithm::deletePatchRowsFromBeginning);
	GlobalCommandSystem().addCommand("PatchDeleteRowEnd", selection::algorithm::deletePatchRowsFromEnd);

	GlobalCommandSystem().addCommand("PatchAppendColumnBeginning", selection::algorithm::appendPatchColumnsAtBeginning);
	GlobalCommandSystem().addCommand("PatchAppendColumnEnd", selection::algorithm::appendPatchColumnsAtEnd);
	GlobalCommandSystem().addCommand("PatchAppendRowBeginning", selection::algorithm::appendPatchRowsAtBeginning);
	GlobalCommandSystem().addCommand("PatchAppendRowEnd", selection::algorithm::appendPatchRowsAtEnd);

	GlobalCommandSystem().addCommand("InvertCurve", selection::algorithm::invertPatch);
	GlobalCommandSystem().addCommand("RedisperseRows", selection::algorithm::redispersePatchRows);
	GlobalCommandSystem().addCommand("RedisperseCols", selection::algorithm::redispersePatchCols);
	GlobalCommandSystem().addCommand("MatrixTranspose", selection::algorithm::transposePatch);
	GlobalCommandSystem().addCommand("CapCurrentCurve", selection::algorithm::capPatch);
	GlobalCommandSystem().addCommand("CycleCapTexturePatch", selection::algorithm::cyclePatchProjection);
	GlobalCommandSystem().addCommand("ThickenPatch", selection::algorithm::thickenPatches);
	GlobalCommandSystem().addCommand("StitchPatchTexture", patch::algorithm::stitchTextures);
	GlobalCommandSystem().addCommand("BulgePatch", patch::algorithm::bulge);

	// Then, connect the Events to the commands
	GlobalEventManager().addCommand("PatchCylinder", "PatchCylinder");
	GlobalEventManager().addCommand("PatchDenseCylinder", "PatchDenseCylinder");
	GlobalEventManager().addCommand("PatchVeryDenseCylinder", "PatchVeryDenseCylinder");
	GlobalEventManager().addCommand("PatchSquareCylinder", "PatchSquareCylinder");
	GlobalEventManager().addCommand("PatchEndCap", "PatchEndCap");
	GlobalEventManager().addCommand("PatchBevel", "PatchBevel");
	GlobalEventManager().addCommand("PatchCone", "PatchCone");
	GlobalEventManager().addCommand("PatchSphere", "PatchSphere");
	GlobalEventManager().addCommand("SimplePatchMesh", "SimplePatchMesh");

	GlobalEventManager().addCommand("PatchInsertColumnEnd", "PatchInsertColumnEnd");
	GlobalEventManager().addCommand("PatchInsertColumnBeginning", "PatchInsertColumnBeginning");
	GlobalEventManager().addCommand("PatchInsertRowEnd", "PatchInsertRowEnd");
	GlobalEventManager().addCommand("PatchInsertRowBeginning", "PatchInsertRowBeginning");

	GlobalEventManager().addCommand("PatchDeleteColumnBeginning", "PatchDeleteColumnBeginning");
	GlobalEventManager().addCommand("PatchDeleteColumnEnd", "PatchDeleteColumnEnd");
	GlobalEventManager().addCommand("PatchDeleteRowBeginning", "PatchDeleteRowBeginning");
	GlobalEventManager().addCommand("PatchDeleteRowEnd", "PatchDeleteRowEnd");

	GlobalEventManager().addCommand("PatchAppendColumnBeginning", "PatchAppendColumnBeginning");
	GlobalEventManager().addCommand("PatchAppendColumnEnd", "PatchAppendColumnEnd");
	GlobalEventManager().addCommand("PatchAppendRowBeginning", "PatchAppendRowBeginning");
	GlobalEventManager().addCommand("PatchAppendRowEnd", "PatchAppendRowEnd");

	GlobalEventManager().addCommand("InvertCurve", "InvertCurve");
	GlobalEventManager().addCommand("RedisperseRows", "RedisperseRows");
	GlobalEventManager().addCommand("RedisperseCols", "RedisperseCols");
	GlobalEventManager().addCommand("MatrixTranspose", "MatrixTranspose");
	GlobalEventManager().addCommand("CapCurrentCurve", "CapCurrentCurve");
	GlobalEventManager().addCommand("CycleCapTexturePatch", "CycleCapTexturePatch");
	GlobalEventManager().addCommand("ThickenPatch", "ThickenPatch");
	GlobalEventManager().addCommand("StitchPatchTexture", "StitchPatchTexture");
	GlobalEventManager().addCommand("BulgePatch", "BulgePatch");
}
