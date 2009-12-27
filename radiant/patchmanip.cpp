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

#include "imainframe.h"
#include "ieventmanager.h"
#include "iselection.h"
#include "ipatch.h"

#include "math/aabb.h"
#include "generic/callback.h"

#include "gdk/gdkkeysyms.h"
#include "gtkutil/dialog.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "select.h"
#include "igrid.h"
#include "patch/PatchNode.h"
#include "patch/PatchSceneWalk.h"
#include "patch/PatchCreators.h"
#include "xyview/GlobalXYWnd.h"
#include "ui/patch/BulgePatchDialog.h"
#include "ui/patch/PatchThickenDialog.h"
#include "ui/patch/PatchCreateDialog.h"
#include "ui/surfaceinspector/SurfaceInspector.h"
#include "selection/algorithm/Primitives.h"
#include "selection/algorithm/General.h"
#include "ui/patch/CapDialog.h"

void Scene_PatchConstructPrefab(scene::Graph& graph, const AABB& aabb, const std::string& shader, EPatchPrefab eType, EViewType viewType, std::size_t width = 3, std::size_t height = 3)
{
  GlobalSelectionSystem().setSelectedAll(false);

  scene::INodePtr node(GlobalPatchCreator(DEF2).createPatch());
  GlobalMap().findOrInsertWorldspawn()->addChildNode(node);

  Patch* patch = Node_getPatch(node);
  patch->setShader(shader);

  patch->ConstructPrefab(aabb, eType, viewType, width, height);
  patch->controlPointsChanged();

	Node_setSelected(node, true);
}


void Patch_makeCaps(Patch& patch, const scene::INodePtr& parent, EPatchCap type, const std::string& shader)
{
  if((type == eCapEndCap || type == eCapIEndCap)
    && patch.getWidth() != 5)
  {
    globalErrorStream() << "cannot create end-cap - patch width != 5\n";
    return;
  }
  if((type == eCapBevel || type == eCapIBevel)
    && patch.getWidth() != 3)
  {
    globalErrorStream() << "cannot create bevel-cap - patch width != 3\n";
    return;
  }
  if(type == eCapCylinder
    && patch.getWidth() != 9)
  {
    globalErrorStream() << "cannot create cylinder-cap - patch width != 9\n";
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
		globalWarningStream() << "Prevented insertion of degenerate patch." << std::endl;
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
		globalWarningStream() << "Prevented insertion of degenerate patch." << std::endl;
	}
  }
}

typedef std::vector<scene::INodePtr> NodeVector;

class PatchCollector :
	public SelectionSystem::Visitor
{
	mutable NodeVector _patches;
public:
	NodeVector& getPatchNodes() {
		return _patches;
	}

	virtual void visit(const scene::INodePtr& node) const {
		if (Node_isPatch(node)) {
			_patches.push_back(node);
		}
	}
};

void Scene_PatchDoCap_Selected(scene::Graph& graph, const std::string& shader)
{
	if (GlobalSelectionSystem().getSelectionInfo().patchCount == 0)
	{
		gtkutil::errorDialog("Cannot create caps, no patches selected.", 
			GlobalMainFrame().getTopLevelWindow());
		return;
	}

	ui::PatchCapDialog dialog;

	if (dialog.run() == ui::IDialog::RESULT_OK)
	{
		PatchCollector collector;
		GlobalSelectionSystem().foreachSelected(collector);

		NodeVector& patchNodes = collector.getPatchNodes();

		for (NodeVector::const_iterator i = patchNodes.begin(); 
			 i != patchNodes.end(); ++i)
		{
			Patch_makeCaps(*Node_getPatch(*i), (*i)->getParent(), dialog.getSelectedCapType(), shader);
		}
	}
}

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
}


class PatchCapTexture
{
public:
  void operator()(Patch& patch) const
  {
    patch.ProjectTexture(Patch::m_CycleCapIndex);
  }
};

void Scene_PatchCapTexture_Selected(scene::Graph& graph)
{
  Scene_forEachSelectedPatch(PatchCapTexture());
  Patch::m_CycleCapIndex = (Patch::m_CycleCapIndex == 0) ? 1 : (Patch::m_CycleCapIndex == 1) ? 2 : 0;
  SceneChangeNotify();
}

class PatchInvertMatrix
{
public:
  void operator()(Patch& patch) const
  {
    patch.InvertMatrix();
  }
};

void Scene_PatchInvert_Selected(scene::Graph& graph)
{
  Scene_forEachSelectedPatch(PatchInvertMatrix());
}

class PatchRedisperse
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
  Scene_forEachSelectedPatch(PatchRedisperse(major));
}

class PatchTransposeMatrix
{
public:
  void operator()(Patch& patch) const
  {
    patch.TransposeMatrix();
  }
};

void Scene_PatchTranspose_Selected(scene::Graph& graph)
{
  Scene_forEachSelectedPatch(PatchTransposeMatrix());
}

class PatchSetShader
{
  const std::string& m_name;
public:
  PatchSetShader(const std::string& name)
    : m_name(name)
  {
  }
  void operator()(Patch& patch) const
  {
    patch.setShader(m_name);
  }
};

void Scene_PatchSetShader_Selected(scene::Graph& graph, const std::string& name)
{
  Scene_forEachSelectedPatch(PatchSetShader(name));
  SceneChangeNotify();
}

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

AABB PatchCreator_getBounds()
{
	AABB aabb = GlobalSelectionSystem().getWorkZone().bounds;
 
  float gridSize = GlobalGrid().getGridSize();

  if(aabb.extents[0] == 0)
  {
    aabb.extents[0] = gridSize;
  }
  if(aabb.extents[1] == 0)
  {
    aabb.extents[1] = gridSize;
  }
  if(aabb.extents[2] == 0)
  {
    aabb.extents[2] = gridSize;
  }

  if(aabb.isValid())
  {
    return aabb;
  }
  return AABB(Vector3(0, 0, 0), Vector3(64, 64, 64));
}

void Patch_Cylinder(const cmd::ArgumentList& args)
{
  UndoableCommand undo("patchCreateCylinder");

  Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), GlobalTextureBrowser().getSelectedShader(), eCylinder, GlobalXYWnd().getActiveViewType());
}

void Patch_DenseCylinder(const cmd::ArgumentList& args)
{
  UndoableCommand undo("patchCreateDenseCylinder");

  Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), GlobalTextureBrowser().getSelectedShader(), eDenseCylinder, GlobalXYWnd().getActiveViewType());
}

void Patch_VeryDenseCylinder(const cmd::ArgumentList& args)
{
  UndoableCommand undo("patchCreateVeryDenseCylinder");

  Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), GlobalTextureBrowser().getSelectedShader(), eVeryDenseCylinder, GlobalXYWnd().getActiveViewType());
}

void Patch_SquareCylinder(const cmd::ArgumentList& args)
{
  UndoableCommand undo("patchCreateSquareCylinder");

  Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), GlobalTextureBrowser().getSelectedShader(), eSqCylinder, GlobalXYWnd().getActiveViewType());
}

void Patch_Endcap(const cmd::ArgumentList& args)
{
  UndoableCommand undo("patchCreateCaps");

  Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), GlobalTextureBrowser().getSelectedShader(), eEndCap, GlobalXYWnd().getActiveViewType());
}

void Patch_Bevel(const cmd::ArgumentList& args)
{
  UndoableCommand undo("patchCreateBevel");

  Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), GlobalTextureBrowser().getSelectedShader(), eBevel, GlobalXYWnd().getActiveViewType());
}

void Patch_Cone(const cmd::ArgumentList& args)
{
  UndoableCommand undo("patchCreateCone");

  Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), GlobalTextureBrowser().getSelectedShader(), eCone, GlobalXYWnd().getActiveViewType());
}

void Patch_Invert(const cmd::ArgumentList& args)
{
  UndoableCommand undo("patchInvert");

  Scene_PatchInvert_Selected(GlobalSceneGraph());
}

void Patch_RedisperseRows(const cmd::ArgumentList& args)
{
  UndoableCommand undo("patchRedisperseRows");

  Scene_PatchRedisperse_Selected(GlobalSceneGraph(), COL);
}

void Patch_RedisperseCols(const cmd::ArgumentList& args)
{
  UndoableCommand undo("patchRedisperseColumns");

  Scene_PatchRedisperse_Selected(GlobalSceneGraph(), COL);
}

void Patch_Transpose(const cmd::ArgumentList& args)
{
  UndoableCommand undo("patchTranspose");

  Scene_PatchTranspose_Selected(GlobalSceneGraph());
}

void Patch_Cap(const cmd::ArgumentList& args)
{
  // FIXME: add support for patch cap creation
  // Patch_CapCurrent();
  UndoableCommand undo("patchCreateCaps");

  Scene_PatchDoCap_Selected(GlobalSceneGraph(), GlobalTextureBrowser().getSelectedShader());
}

void Patch_CycleProjection(const cmd::ArgumentList& args)
{
  UndoableCommand undo("patchCycleUVProjectionAxis");

  Scene_PatchCapTexture_Selected(GlobalSceneGraph());
}

namespace patch {

/** greebo: This inserts rows or columns at the end or the beginning
 * 			of the visited patches.
 */
class PatchRowColumnInserter
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
};

/** greebo: This removes rows or columns from the end or the beginning
 * 			of the visited patches.
 */
class PatchRowColumnRemover
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
};

/** greebo: This appends rows or columns at the end or the beginning
 * 			of the visited patches.
 */
class PatchRowColumnAppender
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
};

/** greebo: The command targets
 */
void insertColumnsAtEnd(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchInsertColumnsAtEnd");
	// true = insert, true = columns, false = end
	Scene_forEachSelectedPatch(PatchRowColumnInserter(true, false));
}

void insertColumnsAtBeginning(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchInsertColumnsAtBeginning");
	// true = insert, true = columns, true = at beginning
	Scene_forEachSelectedPatch(PatchRowColumnInserter(true, true));
}

void insertRowsAtEnd(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchInsertRowsAtEnd");
	// true = insert, false = rows, false = at end
	Scene_forEachSelectedPatch(PatchRowColumnInserter(false, false));
}

void insertRowsAtBeginning(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchInsertRowsAtBeginning");
	// true = insert, false = rows, true = at beginning
	Scene_forEachSelectedPatch(PatchRowColumnInserter(false, true));
}

void deleteColumnsFromBeginning(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchDeleteColumnsFromBeginning");
	Scene_forEachSelectedPatch(PatchRowColumnRemover(true, true));
}

void deleteColumnsFromEnd(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchDeleteColumnsFromEnd");
	Scene_forEachSelectedPatch(PatchRowColumnRemover(true, false));
}

void deleteRowsFromBeginning(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchDeleteRowsFromBeginning");
	Scene_forEachSelectedPatch(PatchRowColumnRemover(false, true));
}

void deleteRowsFromEnd(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchDeleteRowsFromEnd");
	Scene_forEachSelectedPatch(PatchRowColumnRemover(false, false));
}

void appendColumnsAtBeginning(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchAppendColumnsAtBeginning");
	// true = columns, true = at the beginning
	Scene_forEachSelectedPatch(PatchRowColumnAppender(true, true));
}

void appendColumnsAtEnd(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchAppendColumnsAtEnd");
	// true = columns, false = at the end
	Scene_forEachSelectedPatch(PatchRowColumnAppender(true, false));
}

void appendRowsAtBeginning(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchAppendRowsAtBeginning");
	// false = rows, true = at the beginning
	Scene_forEachSelectedPatch(PatchRowColumnAppender(false, true));
}

void appendRowsAtEnd(const cmd::ArgumentList& args) {
	UndoableCommand undo("patchAppendRowsAtEnd");
	// false = rows, false = at the end
	Scene_forEachSelectedPatch(PatchRowColumnAppender(false, false));
}

void thickenPatch(const PatchNodePtr& sourcePatch, 
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
	
	if (createSeams && thickness > 0.0f) {
		// Allocate four new patches
		scene::INodePtr nodes[4] = {
			patchCreator.createPatch(),
			patchCreator.createPatch(),
			patchCreator.createPatch(),
			patchCreator.createPatch()
		};
		
		// Now create the four walls
		for (int i = 0; i < 4; i++) {
			// Insert each node into the same parent as the existing patch
			parent->addChildNode(nodes[i]);
			
			// Retrieve the contained patch from the node
			Patch* wallPatch = Node_getPatch(nodes[i]);
			
			// Create the wall patch by passing i as wallIndex
			wallPatch->createThickenedWall(sourcePatch->getPatchInternal(), *targetPatch, i);
			
			// Now select the newly created patch
			Node_setSelected(nodes[i], true);
		}
	}
	
	// Invert the target patch so that it faces the opposite direction
	targetPatch->InvertMatrix();
}

/** greebo: This collects a list of all selected patches and thickens them
 * after querying the user for the thickness and the "createSeams" boolean.
 * 
 * Note: I chose to populate a list first, because otherwise the visitor
 * class would get stuck in a loop (as the newly created patches get selected,
 * and they are thickened as well, and again and again).  
 */
void thickenSelectedPatches(const cmd::ArgumentList& args) {
	// Get all the selected patches
	PatchPtrVector patchList = selection::algorithm::getSelectedPatches();
	
	if (patchList.size() > 0) {
		UndoableCommand undo("patchThicken");
		
		ui::PatchThickenDialog dialog;
		
		bool createSeams = false;
		float thickness = 0.0f;
		// Extrude along normals is the default (axis=3)
		int axis = 3;
		
		if (dialog.queryPatchThickness(thickness, createSeams, axis)) {
			// Go through the list and thicken all the found ones
			for (std::size_t i = 0; i < patchList.size(); i++) {
				thickenPatch(patchList[i], thickness, createSeams, axis);
			}
		}
	}
	else {
		gtkutil::errorDialog("Cannot thicken patch. Nothing selected.",
							 GlobalMainFrame().getTopLevelWindow());
	}
}

void createSimplePatch(const cmd::ArgumentList& args) {
	ui::PatchCreateDialog dialog;
	
	int width = 3;
	int height = 3;
	bool removeSelectedBrush = false;
	
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	if (dialog.queryPatchDimensions(width, height, 
									info.brushCount, 
									removeSelectedBrush)) 
	{
		UndoableCommand undo("patchCreatePlane");
		
		// Retrieve the boundaries 
		AABB bounds = PatchCreator_getBounds();
		
		if (removeSelectedBrush) {
			// Delete the selection, the should be only one brush selected
			selection::algorithm::deleteSelection();
		}
		
		// Call the PatchConstruct routine (GtkRadiant legacy)
		Scene_PatchConstructPrefab(GlobalSceneGraph(), bounds, 
								   GlobalTextureBrowser().getSelectedShader(), 
								   ePlane, GlobalXYWnd().getActiveViewType(), 
								   width, height);
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
			gtkutil::errorDialog("Cannot stitch textures. \nCould not cast nodes to patches.",
							 GlobalMainFrame().getTopLevelWindow());
		}
		
		SceneChangeNotify();
		// Update the Texture Tools
		ui::SurfaceInspector::Instance().queueUpdate();
	}
	else {
		gtkutil::errorDialog("Cannot stitch patch textures. \nExactly 2 patches must be selected.",
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
		gtkutil::errorDialog("Cannot bulge patch. No patches selected.",
			GlobalMainFrame().getTopLevelWindow());
	}
}
} // namespace patch

#include "generic/callback.h"

void Patch_registerCommands() {
	// First connect the commands to the code
	GlobalCommandSystem().addCommand("PatchCylinder", Patch_Cylinder);
	GlobalCommandSystem().addCommand("PatchDenseCylinder", Patch_DenseCylinder);
	GlobalCommandSystem().addCommand("PatchVeryDenseCylinder", Patch_VeryDenseCylinder);
	GlobalCommandSystem().addCommand("PatchSquareCylinder", Patch_SquareCylinder);
	GlobalCommandSystem().addCommand("PatchEndCap", Patch_Endcap);
	GlobalCommandSystem().addCommand("PatchBevel", Patch_Bevel);
	GlobalCommandSystem().addCommand("PatchCone", Patch_Cone);
	GlobalCommandSystem().addCommand("SimplePatchMesh", patch::createSimplePatch);

	GlobalCommandSystem().addCommand("PatchInsertColumnEnd", patch::insertColumnsAtEnd);
	GlobalCommandSystem().addCommand("PatchInsertColumnBeginning", patch::insertColumnsAtBeginning);
	GlobalCommandSystem().addCommand("PatchInsertRowEnd", patch::insertRowsAtEnd);
	GlobalCommandSystem().addCommand("PatchInsertRowBeginning", patch::insertRowsAtBeginning);

	GlobalCommandSystem().addCommand("PatchDeleteColumnBeginning", patch::deleteColumnsFromBeginning);
	GlobalCommandSystem().addCommand("PatchDeleteColumnEnd", patch::deleteColumnsFromEnd);
	GlobalCommandSystem().addCommand("PatchDeleteRowBeginning", patch::deleteRowsFromBeginning);
	GlobalCommandSystem().addCommand("PatchDeleteRowEnd", patch::deleteRowsFromEnd);

	GlobalCommandSystem().addCommand("PatchAppendColumnBeginning", patch::appendColumnsAtBeginning);
	GlobalCommandSystem().addCommand("PatchAppendColumnEnd", patch::appendColumnsAtEnd);
	GlobalCommandSystem().addCommand("PatchAppendRowBeginning", patch::appendRowsAtBeginning);
	GlobalCommandSystem().addCommand("PatchAppendRowEnd", patch::appendRowsAtEnd);

	GlobalCommandSystem().addCommand("InvertCurve", Patch_Invert);
	GlobalCommandSystem().addCommand("RedisperseRows", Patch_RedisperseRows);
	GlobalCommandSystem().addCommand("RedisperseCols", Patch_RedisperseCols);
	GlobalCommandSystem().addCommand("MatrixTranspose", Patch_Transpose);
	GlobalCommandSystem().addCommand("CapCurrentCurve", Patch_Cap);
	GlobalCommandSystem().addCommand("CycleCapTexturePatch", Patch_CycleProjection);
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
