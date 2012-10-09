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
/*
void Patch_makeCaps(Patch& patch, const scene::INodePtr& parent, EPatchCap type, const std::string& shader)
{
  if((type == eCapEndCap || type == eCapIEndCap)
    && patch.getWidth() != 5)
  {
    rError() << "cannot create end-cap - patch width != 5\n";
    return;
  }
  if((type == eCapBevel || type == eCapIBevel)
    && patch.getWidth() != 3)
  {
    rError() << "cannot create bevel-cap - patch width != 3\n";
    return;
  }
  if(type == eCapCylinder
    && patch.getWidth() != 9)
  {
    rError() << "cannot create cylinder-cap - patch width != 9\n";
    return;
  }

  assert(parent != NULL);

  {
    scene::INodePtr cap(GlobalPatchCreator(DEF2).createPatch());
	parent->addChildNode(cap);

	Patch* capPatch = Node_getPatch(cap);
	assert(capPatch != NULL);

    patch.MakeCap(capPatch, type, ROW, true);
    capPatch->setShader(shader);

	// greebo: Avoid creating "degenerate" patches (all vertices merged in one 3D point)
	if (!capPatch->isDegenerate()) {
		Node_setSelected(cap, true);
	}
	else {
		parent->removeChildNode(cap);
		rWarning() << "Prevented insertion of degenerate patch." << std::endl;
	}
  }

  {
    scene::INodePtr cap(GlobalPatchCreator(DEF2).createPatch());
	parent->addChildNode(cap);

	Patch* capPatch = Node_getPatch(cap);
	assert(capPatch != NULL);

    patch.MakeCap(capPatch, type, ROW, false);
    capPatch->setShader(shader);

	// greebo: Avoid creating "degenerate" patches (all vertices merged in one 3D point)
	if (!capPatch->isDegenerate()) {
		Node_setSelected(cap, true);
	}
	else {
		parent->removeChildNode(cap);
		rWarning() << "Prevented insertion of degenerate patch." << std::endl;
	}
  }
}*/
/*
void Scene_PatchDoCap_Selected(const std::string& shader)
{
	if (GlobalSelectionSystem().getSelectionInfo().patchCount == 0)
	{
		gtkutil::MessageBox::ShowError(_("Cannot create caps, no patches selected."),
			GlobalMainFrame().getTopLevelWindow());
		return;
	}

	ui::PatchCapDialog dialog;

	if (dialog.run() == ui::IDialog::RESULT_OK)
	{
		PatchPtrVector patchNodes = selection::algorithm::getSelectedPatches();

		std::for_each(patchNodes.begin(), patchNodes.end(), [&] (PatchNodePtr& patchNode)
		{
			Patch_makeCaps(patchNode->getPatchInternal(), patchNode->getParent(), dialog.getSelectedCapType(), shader);
		});
	}
}
*/
/*
Patch* Scene_GetUltimateSelectedVisiblePatch()
{
  if(GlobalSelectionSystem().countSelected() != 0)
  {
    const scene::INodePtr& node = GlobalSelectionSystem().ultimateSelected();
    if(node->visible()) {
      return Node_getPatch(node);
    }
  }
  return 0;
}*/


/*class PatchCapTexture
{
public:
  void operator()(Patch& patch) const
  {
    patch.ProjectTexture(Patch::m_CycleCapIndex);
  }
};

void Scene_PatchCapTexture_Selected(scene::Graph& graph)
{
	GlobalSelectionSystem().foreachPatch(PatchCapTexture());
  Patch::m_CycleCapIndex = (Patch::m_CycleCapIndex == 0) ? 1 : (Patch::m_CycleCapIndex == 1) ? 2 : 0;
  SceneChangeNotify();
}*/

/*class PatchRedisperse
{
  EMatrixMajor m_major;
public:
  PatchRedisperse(EMatrixMajor major) : m_major(major)
  {
  }
  void operator()(Patch& patch) const
  {
    patch.Redisperse(m_major);
  }
};

void Scene_PatchRedisperse_Selected(scene::Graph& graph, EMatrixMajor major)
{
  GlobalSelectionSystem().foreachPatch(PatchRedisperse(major));
}*/

/*class PatchTransposeMatrix
{
public:
  void operator()(Patch& patch) const
  {
    patch.TransposeMatrix();
  }
};

void Scene_PatchTranspose_Selected(scene::Graph& graph)
{
  GlobalSelectionSystem().foreachPatch(PatchTransposeMatrix());
}*/

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

namespace patch {

/** greebo: This inserts rows or columns at the end or the beginning
 * 			of the visited patches.
 */
/*class PatchRowColumnInserter
{
	bool _columns;
	bool _atBeginning;
public:
	PatchRowColumnInserter(bool columns, bool atBeginning) :
		_columns(columns),
		_atBeginning(atBeginning)
	{}

	void operator()(Patch& patch) const {
		patch.InsertRemove(true, _columns, _atBeginning);
	}
};*/

/** greebo: This removes rows or columns from the end or the beginning
 * 			of the visited patches.
 */
/*class PatchRowColumnRemover
{
	bool _columns;
	bool _fromBeginning;
public:
	PatchRowColumnRemover(bool columns, bool fromBeginning) :
		_columns(columns),
		_fromBeginning(fromBeginning)
	{}

	void operator()(Patch& patch) const {
		patch.InsertRemove(false, _columns, _fromBeginning);
	}
};*/

/** greebo: This appends rows or columns at the end or the beginning
 * 			of the visited patches.
 */
/*class PatchRowColumnAppender
{
	bool _columns;
	bool _atBeginning;
public:
	PatchRowColumnAppender(bool columns, bool atBeginning) :
		_columns(columns),
		_atBeginning(atBeginning)
	{}

	void operator()(Patch& patch) const {
		patch.appendPoints(_columns, _atBeginning);
	}
};*/


/** greebo: The command targets
 */
/*void insertColumnsAtEnd(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchInsertColumnsAtEnd");
	// true = insert, true = columns, false = end
	GlobalSelectionSystem().foreachPatch(PatchRowColumnInserter(true, false));
}

void insertColumnsAtBeginning(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchInsertColumnsAtBeginning");
	// true = insert, true = columns, true = at beginning
	GlobalSelectionSystem().foreachPatch(PatchRowColumnInserter(true, true));
}

void insertRowsAtEnd(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchInsertRowsAtEnd");
	// true = insert, false = rows, false = at end
	GlobalSelectionSystem().foreachPatch(PatchRowColumnInserter(false, false));
}

void insertRowsAtBeginning(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchInsertRowsAtBeginning");
	// true = insert, false = rows, true = at beginning
	GlobalSelectionSystem().foreachPatch(PatchRowColumnInserter(false, true));
}

void deleteColumnsFromBeginning(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchDeleteColumnsFromBeginning");
	GlobalSelectionSystem().foreachPatch(PatchRowColumnRemover(true, true));
}

void deleteColumnsFromEnd(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchDeleteColumnsFromEnd");
	GlobalSelectionSystem().foreachPatch(PatchRowColumnRemover(true, false));
}

void deleteRowsFromBeginning(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchDeleteRowsFromBeginning");
	GlobalSelectionSystem().foreachPatch(PatchRowColumnRemover(false, true));
}

void deleteRowsFromEnd(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchDeleteRowsFromEnd");
	GlobalSelectionSystem().foreachPatch(PatchRowColumnRemover(false, false));
}

void appendColumnsAtBeginning(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchAppendColumnsAtBeginning");
	// true = columns, true = at the beginning
	GlobalSelectionSystem().foreachPatch(PatchRowColumnAppender(true, true));
}

void appendColumnsAtEnd(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchAppendColumnsAtEnd");
	// true = columns, false = at the end
	GlobalSelectionSystem().foreachPatch(PatchRowColumnAppender(true, false));
}

void appendRowsAtBeginning(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchAppendRowsAtBeginning");
	// false = rows, true = at the beginning
	GlobalSelectionSystem().foreachPatch(PatchRowColumnAppender(false, true));
}

void appendRowsAtEnd(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchAppendRowsAtEnd");
	// false = rows, false = at the end
	GlobalSelectionSystem().foreachPatch(PatchRowColumnAppender(false, false));
}*/

/*void thickenPatch(const PatchNodePtr& sourcePatch,
				  float thickness, bool createSeams, int axis)
{
	// Get a shortcut to the patchcreator
	PatchCreator& patchCreator = GlobalPatchCreator(DEF2);

	// Create a new patch node
	scene::INodePtr node(patchCreator.createPatch());

	scene::INodePtr parent = sourcePatch->getParent();
	assert(parent != NULL);

	// Insert the node into the same parent as the existing patch
	parent->addChildNode(node);

	// Retrieve the contained patch from the node
	Patch* targetPatch = Node_getPatch(node);

	// Create the opposite patch with the given thickness = distance
	targetPatch->createThickenedOpposite(sourcePatch->getPatchInternal(), thickness, axis);

	// Select the newly created patch
	Node_setSelected(node, true);

	if (createSeams && thickness > 0.0f)
	{
		// Allocate four new patches
		scene::INodePtr nodes[4] = {
			patchCreator.createPatch(),
			patchCreator.createPatch(),
			patchCreator.createPatch(),
			patchCreator.createPatch()
		};

		// Now create the four walls
		for (int i = 0; i < 4; i++)
		{
			// Insert each node into the same parent as the existing patch
			// It's vital to do this first, otherwise these patches won't have valid shaders
			parent->addChildNode(nodes[i]);

			// Retrieve the contained patch from the node
			Patch* wallPatch = Node_getPatch(nodes[i]);

			// Create the wall patch by passing i as wallIndex
			wallPatch->createThickenedWall(sourcePatch->getPatchInternal(), *targetPatch, i);

			if (!wallPatch->isDegenerate())
			{
				// Now select the newly created patch
				Node_setSelected(nodes[i], true);
			}
			else
			{
				rMessage() << "Thicken: Discarding degenerate patch." << std::endl;

				// Remove again
				parent->removeChildNode(nodes[i]);
			}
		}
	}

	// Invert the target patch so that it faces the opposite direction
	targetPatch->InvertMatrix();
}*/

/** greebo: This collects a list of all selected patches and thickens them
 * after querying the user for the thickness and the "createSeams" boolean.
 *
 * Note: I chose to populate a list first, because otherwise the visitor
 * class would get stuck in a loop (as the newly created patches get selected,
 * and they are thickened as well, and again and again).
 */
void thickenSelectedPatches(const cmd::ArgumentList& args)
{
	// Get all the selected patches
	PatchPtrVector patchList = selection::algorithm::getSelectedPatches();

	if (patchList.size() > 0)
	{
		UndoableCommand undo("patchThicken");

		ui::PatchThickenDialog dialog;

		if (dialog.run() == ui::IDialog::RESULT_OK)
		{
			// Go through the list and thicken all the found ones
			for (std::size_t i = 0; i < patchList.size(); i++)
			{
				algorithm::thicken(patchList[i], dialog.getThickness(), dialog.getCeateSeams(), dialog.getAxis());
			}
		}
	}
	else {
		gtkutil::MessageBox::ShowError(_("Cannot thicken patch. Nothing selected."),
							 GlobalMainFrame().getTopLevelWindow());
	}
}

void stitchPatchTextures(const cmd::ArgumentList& args) {
	// Get all the selected patches
	PatchPtrVector patchList = selection::algorithm::getSelectedPatches();

	if (patchList.size() == 2) {
		UndoableCommand undo("patchStitchTexture");

		// Fetch the instances from the selectionsystem
		const scene::INodePtr& targetNode =
			GlobalSelectionSystem().ultimateSelected();

		const scene::INodePtr& sourceNode =
			GlobalSelectionSystem().penultimateSelected();

		// Cast the instances onto a patch
		Patch* source = Node_getPatch(sourceNode);
		Patch* target = Node_getPatch(targetNode);

		if (source != NULL && target != NULL) {
			// Stitch the texture leaving the source patch intact
			target->stitchTextureFrom(*source);
		}
		else {
			gtkutil::MessageBox::ShowError(_("Cannot stitch textures. \nCould not cast nodes to patches."),
							 GlobalMainFrame().getTopLevelWindow());
		}

		SceneChangeNotify();
		// Update the Texture Tools
		ui::SurfaceInspector::update();
	}
	else {
		gtkutil::MessageBox::ShowError(_("Cannot stitch patch textures. \nExactly 2 patches must be selected."),
							 GlobalMainFrame().getTopLevelWindow());
	}
}

void bulgePatch(const cmd::ArgumentList& args) {
	// Get the list of selected patches
	PatchPtrVector patches = selection::algorithm::getSelectedPatches();

	if (patches.size() > 0) {
		int maxValue = 16;

		// Ask the user to enter a noise value
		if (ui::BulgePatchDialog::queryPatchNoise(maxValue)) {
			UndoableCommand cmd("BulgePatch");

			// Cycle through all patches and apply the bulge algorithm
			for (PatchPtrVector::iterator p = patches.begin(); p != patches.end(); ++p)
			{
				Patch& patch = (*p)->getPatchInternal();

				patch.undoSave();

				for (PatchControlIter i = patch.begin(); i != patch.end(); ++i) {
					PatchControl& control = *i;
					int randomNumber = int(maxValue * (float(std::rand()) / float(RAND_MAX)));
					control.vertex.set(control.vertex.x(), control.vertex.y(), control.vertex.z() + randomNumber);
				}

				patch.controlPointsChanged();
			}
		}
	}
	else {
		gtkutil::MessageBox::ShowError(_("Cannot bulge patch. No patches selected."),
			GlobalMainFrame().getTopLevelWindow());
	}
}
} // namespace patch

void Patch_registerCommands() 
{
	// First connect the commands to the code
	GlobalCommandSystem().addCommand("CreatePatchPrefab", patch::algorithm::createPrefab, cmd::ARGTYPE_STRING);

	/*GlobalCommandSystem().addCommand("PatchCylinder", patch::algorithm::createCylinder);
	GlobalCommandSystem().addCommand("PatchDenseCylinder", patch::algorithm::createDenseCylinder);
	GlobalCommandSystem().addCommand("PatchVeryDenseCylinder", patch::algorithm::createVeryDenseCylinder);
	GlobalCommandSystem().addCommand("PatchSquareCylinder", patch::algorithm::createSquareCylinder);
	GlobalCommandSystem().addCommand("PatchEndCap", patch::algorithm::createEndcap);
	GlobalCommandSystem().addCommand("PatchBevel", patch::algorithm::createBevel);
	GlobalCommandSystem().addCommand("PatchCone", patch::algorithm::createCone);
	GlobalCommandSystem().addCommand("PatchSphere", patch::algorithm::createSphere);*/

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
	GlobalCommandSystem().addCommand("ThickenPatch", patch::thickenSelectedPatches);
	GlobalCommandSystem().addCommand("StitchPatchTexture", patch::stitchPatchTextures);
	GlobalCommandSystem().addCommand("BulgePatch", patch::bulgePatch);

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
