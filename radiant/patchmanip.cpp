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

#include "ieventmanager.h"
#include "iselection.h"
#include "ipatch.h"

#include "math/aabb.h"
#include "generic/callback.h"

#include "gdk/gdkkeysyms.h"
#include "gtkutil/dialog.h"
#include "map.h"
#include "mainframe.h"
#include "gtkmisc.h"
#include "gtkdlgs.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "select.h"
#include "igrid.h"
#include "patch/PatchNode.h"
#include "patch/PatchSceneWalk.h"
#include "patch/PatchCreators.h"
#include "xyview/GlobalXYWnd.h"
#include "ui/patch/PatchThickenDialog.h"
#include "ui/patch/PatchCreateDialog.h"
#include "selection/algorithm/Primitives.h"

PatchCreator* g_patchCreator = 0;

void Scene_PatchConstructPrefab(scene::Graph& graph, const AABB& aabb, const std::string& shader, EPatchPrefab eType, int axis, std::size_t width = 3, std::size_t height = 3)
{
  GlobalSelectionSystem().setSelectedAll(false);

  scene::INodePtr node(g_patchCreator->createPatch());
  Node_getTraversable(Map_FindOrInsertWorldspawn(g_map))->insert(node);

  Patch* patch = Node_getPatch(node);
  patch->SetShader(shader);

  patch->ConstructPrefab(aabb, eType, axis, width, height);
  patch->controlPointsChanged();

  {
    scene::Path patchpath(GlobalSceneGraph().root());
    patchpath.push(Map_GetWorldspawn(g_map));
    patchpath.push(node);
    Instance_getSelectable(*graph.find(patchpath))->setSelected(true);
  }
}


void Patch_makeCaps(Patch& patch, scene::Instance& instance, EPatchCap type, const std::string& shader)
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

  {
    scene::INodePtr cap(g_patchCreator->createPatch());
    Node_getTraversable(instance.path().parent())->insert(cap);

    patch.MakeCap(Node_getPatch(cap), type, ROW, true);
    Node_getPatch(cap)->SetShader(shader);

    scene::Path path(instance.path());
    path.pop();
    path.push(cap);
    selectPath(path, true);
  }

  {
    scene::INodePtr cap(g_patchCreator->createPatch());
    Node_getTraversable(instance.path().parent())->insert(cap);

    patch.MakeCap(Node_getPatch(cap), type, ROW, false);
    Node_getPatch(cap)->SetShader(shader);

    scene::Path path(instance.path());
    path.pop();
    path.push(cap);
    selectPath(path, true);
  }
}

typedef std::vector<scene::Instance*> InstanceVector;

class PatchStoreInstance
{
  InstanceVector& m_instances;
public:
  PatchStoreInstance(InstanceVector& instances) : m_instances(instances)
  {
  }
  void operator()(PatchInstance& patch) const
  {
    m_instances.push_back(&patch);
  }
};

enum ECapDialog {
  PATCHCAP_BEVEL = 0,
  PATCHCAP_ENDCAP,
  PATCHCAP_INVERTED_BEVEL,
  PATCHCAP_INVERTED_ENDCAP,
  PATCHCAP_CYLINDER
};

EMessageBoxReturn DoCapDlg(ECapDialog *type);

void Scene_PatchDoCap_Selected(scene::Graph& graph, const std::string& shader)
{
  ECapDialog nType;

  if(DoCapDlg(&nType) == eIDOK)
  {
    EPatchCap eType;
    switch(nType)
    {
    case PATCHCAP_INVERTED_BEVEL:
      eType = eCapIBevel;
      break;
    case PATCHCAP_BEVEL:
      eType = eCapBevel;
      break;
    case PATCHCAP_INVERTED_ENDCAP:
      eType = eCapIEndCap;
      break;
    case PATCHCAP_ENDCAP:
      eType = eCapEndCap;
      break;
    case PATCHCAP_CYLINDER:
      eType = eCapCylinder;
      break;
    default:
      ERROR_MESSAGE("invalid patch cap type");
      return;
    }
  
    InstanceVector instances;
    Scene_forEachVisibleSelectedPatchInstance(PatchStoreInstance(instances));
    for(InstanceVector::const_iterator i = instances.begin(); i != instances.end(); ++i)
    {
      Patch_makeCaps(* Node_getPatch((*i)->path().top()), *(*i), eType, shader);
    }
  }
}

Patch* Scene_GetUltimateSelectedVisiblePatch()
{
  if(GlobalSelectionSystem().countSelected() != 0)
  {
    scene::INodePtr node = GlobalSelectionSystem().ultimateSelected().path().top();
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
  Scene_forEachVisibleSelectedPatch(PatchCapTexture());
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
  Scene_forEachVisibleSelectedPatch(PatchInvertMatrix());
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
  Scene_forEachVisibleSelectedPatch(PatchRedisperse(major));
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
  Scene_forEachVisibleSelectedPatch(PatchTransposeMatrix());
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
    patch.SetShader(m_name);
  }
};

void Scene_PatchSetShader_Selected(scene::Graph& graph, const std::string& name)
{
  Scene_forEachVisibleSelectedPatch(PatchSetShader(name));
  SceneChangeNotify();
}

class PatchSelectByShader
{
  std::string m_name;
public:
  inline PatchSelectByShader(const std::string& name)
    : m_name(name)
  {
  }
  void operator()(PatchInstance& patch) const
  {
    if(shader_equal(patch.getPatch().GetShader(), m_name))
    {
      patch.setSelected(true);
    }
  }
};

void Scene_PatchSelectByShader(scene::Graph& graph, const std::string& name)
{
  Scene_forEachVisiblePatchInstance(PatchSelectByShader(name));
}

AABB PatchCreator_getBounds()
{
  AABB aabb(AABB::createFromMinMax(Select_getWorkZone().d_work_min, 
  								   Select_getWorkZone().d_work_max));

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

void Patch_Cylinder()
{
  UndoableCommand undo("patchCreateCylinder");

  Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), GlobalTextureBrowser().getSelectedShader(), eCylinder, GlobalXYWnd().getActiveViewType());
}

void Patch_DenseCylinder()
{
  UndoableCommand undo("patchCreateDenseCylinder");

  Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), GlobalTextureBrowser().getSelectedShader(), eDenseCylinder, GlobalXYWnd().getActiveViewType());
}

void Patch_VeryDenseCylinder()
{
  UndoableCommand undo("patchCreateVeryDenseCylinder");

  Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), GlobalTextureBrowser().getSelectedShader(), eVeryDenseCylinder, GlobalXYWnd().getActiveViewType());
}

void Patch_SquareCylinder()
{
  UndoableCommand undo("patchCreateSquareCylinder");

  Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), GlobalTextureBrowser().getSelectedShader(), eSqCylinder, GlobalXYWnd().getActiveViewType());
}

void Patch_Endcap()
{
  UndoableCommand undo("patchCreateCaps");

  Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), GlobalTextureBrowser().getSelectedShader(), eEndCap, GlobalXYWnd().getActiveViewType());
}

void Patch_Bevel()
{
  UndoableCommand undo("patchCreateBevel");

  Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), GlobalTextureBrowser().getSelectedShader(), eBevel, GlobalXYWnd().getActiveViewType());
}

void Patch_Cone()
{
  UndoableCommand undo("patchCreateCone");

  Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), GlobalTextureBrowser().getSelectedShader(), eCone, GlobalXYWnd().getActiveViewType());
}

void Patch_Invert()
{
  UndoableCommand undo("patchInvert");

  Scene_PatchInvert_Selected(GlobalSceneGraph());
}

void Patch_RedisperseRows()
{
  UndoableCommand undo("patchRedisperseRows");

  Scene_PatchRedisperse_Selected(GlobalSceneGraph(), COL);
}

void Patch_RedisperseCols()
{
  UndoableCommand undo("patchRedisperseColumns");

  Scene_PatchRedisperse_Selected(GlobalSceneGraph(), COL);
}

void Patch_Transpose()
{
  UndoableCommand undo("patchTranspose");

  Scene_PatchTranspose_Selected(GlobalSceneGraph());
}

void Patch_Cap()
{
  // FIXME: add support for patch cap creation
  // Patch_CapCurrent();
  UndoableCommand undo("patchCreateCaps");

  Scene_PatchDoCap_Selected(GlobalSceneGraph(), GlobalTextureBrowser().getSelectedShader());
}

void Patch_CycleProjection()
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
void insertColumnsAtEnd() {
	UndoableCommand undo("patchInsertColumnsAtEnd");
	// true = insert, true = columns, false = end
	Scene_forEachVisibleSelectedPatch(PatchRowColumnInserter(true, false));
}

void insertColumnsAtBeginning() {
	UndoableCommand undo("patchInsertColumnsAtBeginning");
	// true = insert, true = columns, true = at beginning
	Scene_forEachVisibleSelectedPatch(PatchRowColumnInserter(true, true));
}

void insertRowsAtEnd() {
	UndoableCommand undo("patchInsertRowsAtEnd");
	// true = insert, false = rows, false = at end
	Scene_forEachVisibleSelectedPatch(PatchRowColumnInserter(false, false));
}

void insertRowsAtBeginning() {
	UndoableCommand undo("patchInsertRowsAtBeginning");
	// true = insert, false = rows, true = at beginning
	Scene_forEachVisibleSelectedPatch(PatchRowColumnInserter(false, true));
}

void deleteColumnsFromBeginning() {
	UndoableCommand undo("patchDeleteColumnsFromBeginning");
	Scene_forEachVisibleSelectedPatch(PatchRowColumnRemover(true, true));
}

void deleteColumnsFromEnd() {
	UndoableCommand undo("patchDeleteColumnsFromEnd");
	Scene_forEachVisibleSelectedPatch(PatchRowColumnRemover(true, false));
}

void deleteRowsFromBeginning() {
	UndoableCommand undo("patchDeleteRowsFromBeginning");
	Scene_forEachVisibleSelectedPatch(PatchRowColumnRemover(false, true));
}

void deleteRowsFromEnd() {
	UndoableCommand undo("patchDeleteRowsFromEnd");
	Scene_forEachVisibleSelectedPatch(PatchRowColumnRemover(false, false));
}

void appendColumnsAtBeginning() {
	UndoableCommand undo("patchAppendColumnsAtBeginning");
	// true = columns, true = at the beginning
	Scene_forEachVisibleSelectedPatch(PatchRowColumnAppender(true, true));
}

void appendColumnsAtEnd() {
	UndoableCommand undo("patchAppendColumnsAtEnd");
	// true = columns, false = at the end
	Scene_forEachVisibleSelectedPatch(PatchRowColumnAppender(true, false));
}

void appendRowsAtBeginning() {
	UndoableCommand undo("patchAppendRowsAtBeginning");
	// false = rows, true = at the beginning
	Scene_forEachVisibleSelectedPatch(PatchRowColumnAppender(false, true));
}

void appendRowsAtEnd() {
	UndoableCommand undo("patchAppendRowsAtEnd");
	// false = rows, false = at the end
	Scene_forEachVisibleSelectedPatch(PatchRowColumnAppender(false, false));
}

void thickenPatches(PatchPtrVector patchList, 
					const float& thickness, 
					const bool& createSeams, 
					const int& axis) 
{
	// Go through the list and thicken all the found ones
	for (unsigned int i = 0; i < patchList.size(); i++) {
		// Retrieve the reference from the pointer
		Patch& sourcePatch = *patchList[i]; 
		
		// Create a new patch node
		scene::INodePtr node(g_patchCreator->createPatch());
		// Insert the node into worldspawn
		Node_getTraversable(Map_FindOrInsertWorldspawn(g_map))->insert(node);
	
		// Retrieve the contained patch from the node
		Patch* targetPatch = Node_getPatch(node);
	
		// Create the opposite patch with the given thickness = distance
		targetPatch->createThickenedOpposite(sourcePatch, thickness, axis);
	
		// Now select the newly created patches
		{
			scene::Path patchpath(GlobalSceneGraph().root());
			patchpath.push(Map_GetWorldspawn(g_map));
			patchpath.push(node);
			Instance_getSelectable(*GlobalSceneGraph().find(patchpath))->setSelected(true);
		}
		
		if (createSeams && thickness > 0.0f) {
			// Allocate four new patches
			scene::INodePtr nodes[4] = {
				scene::INodePtr(g_patchCreator->createPatch()),
				scene::INodePtr(g_patchCreator->createPatch()),
				scene::INodePtr(g_patchCreator->createPatch()),
				scene::INodePtr(g_patchCreator->createPatch())
			};
			
			// Now create the four walls
			for (int i = 0; i < 4; i++) {
				// Insert each node into worldspawn 
				Node_getTraversable(Map_FindOrInsertWorldspawn(g_map))->insert(nodes[i]);
				
				// Retrieve the contained patch from the node
				Patch* wallPatch = Node_getPatch(nodes[i]);
				
				// Create the wall patch by passing i as wallIndex
				wallPatch->createThickenedWall(sourcePatch, *targetPatch, i);
				
				// Now select the newly created patches
				{
					scene::Path patchpath(GlobalSceneGraph().root());
					patchpath.push(Map_GetWorldspawn(g_map));
					patchpath.push(nodes[i]);
					Instance_getSelectable(*GlobalSceneGraph().find(patchpath))->setSelected(true);
				}
			}
		}
		
		// Invert the target patch so that it faces the opposite direction
		targetPatch->InvertMatrix();
	}
}

/** greebo: This collects a list of all selected patches and thickens them
 * after querying the user for the thickness and the "createSeams" boolean.
 * 
 * Note: I chose to populate a list first, because otherwise the visitor
 * class would get stuck in a loop (as the newly created patches get selected,
 * and they are thickened as well, and again and again).  
 */
void thickenSelectedPatches() {
	
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
			// Thicken all the patches in the list
			thickenPatches(patchList, thickness, createSeams, axis);
		}
	}
	else {
		gtkutil::errorDialog("Cannot thicken patch. Nothing selected.",
							 MainFrame_getWindow());
	}
}

void createSimplePatch() {
	ui::PatchCreateDialog dialog;
	
	int width = 3;
	int height = 3;
	bool removeSelectedBrush = false;
	
	if (dialog.queryPatchDimensions(width, height, 
									map::countSelectedBrushes(), 
									removeSelectedBrush)) 
	{
		UndoableCommand undo("patchCreatePlane");
		
		// Retrieve the boundaries 
		AABB bounds = PatchCreator_getBounds();
		
		if (removeSelectedBrush) {
			// Delete the selection, the should be only one brush selected
			Select_Delete();
		}
		
		// Call the PatchConstruct routine (GtkRadiant legacy)
		Scene_PatchConstructPrefab(GlobalSceneGraph(), bounds, 
								   GlobalTextureBrowser().getSelectedShader(), 
								   ePlane, GlobalXYWnd().getActiveViewType(), 
								   width, height);
	}
}

void stitchPatchTextures() {
	// Get all the selected patches
	PatchPtrVector patchList = selection::algorithm::getSelectedPatches();
	
	if (patchList.size() == 2) {
		UndoableCommand undo("patchStitchTexture");

		// Fetch the instances from the selectionsystem		
		scene::Instance& targetInstance = 
			GlobalSelectionSystem().ultimateSelected();
			
		scene::Instance& sourceInstance =
			GlobalSelectionSystem().penultimateSelected();
		
		// Cast the instances onto a patch
		Patch* source = Node_getPatch(sourceInstance.path().top());
		Patch* target = Node_getPatch(targetInstance.path().top());
		
		if (source != NULL && target != NULL) {		
			// Stitch the texture leaving the source patch intact 
			target->stitchTextureFrom(*source);
		}
		else {
			gtkutil::errorDialog("Cannot stitch textures. \nCould not cast nodes to patches.",
							 MainFrame_getWindow());
		}
	}
	else {
		gtkutil::errorDialog("Cannot stitch patch textures. \nExactly 2 patches must be selected.",
							 MainFrame_getWindow());
	}
}

} // namespace patch

#include "generic/callback.h"

void Patch_registerCommands()
{
  GlobalEventManager().addCommand("PatchCylinder", FreeCaller<Patch_Cylinder>());
  GlobalEventManager().addCommand("PatchDenseCylinder", FreeCaller<Patch_DenseCylinder>());
  GlobalEventManager().addCommand("PatchVeryDenseCylinder", FreeCaller<Patch_VeryDenseCylinder>());
  GlobalEventManager().addCommand("PatchSquareCylinder", FreeCaller<Patch_SquareCylinder>());
  GlobalEventManager().addCommand("PatchEndCap", FreeCaller<Patch_Endcap>());
  GlobalEventManager().addCommand("PatchBevel", FreeCaller<Patch_Bevel>());
  GlobalEventManager().addCommand("PatchCone", FreeCaller<Patch_Cone>());
  GlobalEventManager().addCommand("SimplePatchMesh", FreeCaller<patch::createSimplePatch>());
  
  GlobalEventManager().addCommand("PatchInsertColumnEnd", FreeCaller<patch::insertColumnsAtEnd>());
  GlobalEventManager().addCommand("PatchInsertColumnBeginning", FreeCaller<patch::insertColumnsAtBeginning>());
  GlobalEventManager().addCommand("PatchInsertRowEnd", FreeCaller<patch::insertRowsAtEnd>());
  GlobalEventManager().addCommand("PatchInsertRowBeginning", FreeCaller<patch::insertRowsAtBeginning>());
  
  GlobalEventManager().addCommand("PatchDeleteColumnBeginning", FreeCaller<patch::deleteColumnsFromBeginning>());
  GlobalEventManager().addCommand("PatchDeleteColumnEnd", FreeCaller<patch::deleteColumnsFromEnd>());
  GlobalEventManager().addCommand("PatchDeleteRowBeginning", FreeCaller<patch::deleteRowsFromBeginning>());
  GlobalEventManager().addCommand("PatchDeleteRowEnd", FreeCaller<patch::deleteRowsFromEnd>());
  
  GlobalEventManager().addCommand("PatchAppendColumnBeginning", FreeCaller<patch::appendColumnsAtBeginning>());
  GlobalEventManager().addCommand("PatchAppendColumnEnd", FreeCaller<patch::appendColumnsAtEnd>());
  GlobalEventManager().addCommand("PatchAppendRowBeginning", FreeCaller<patch::appendRowsAtBeginning>());
  GlobalEventManager().addCommand("PatchAppendRowEnd", FreeCaller<patch::appendRowsAtEnd>());
  
  GlobalEventManager().addCommand("InvertCurve", FreeCaller<Patch_Invert>());
  GlobalEventManager().addCommand("RedisperseRows", FreeCaller<Patch_RedisperseRows>());
  GlobalEventManager().addCommand("RedisperseCols", FreeCaller<Patch_RedisperseCols>());
  GlobalEventManager().addCommand("MatrixTranspose", FreeCaller<Patch_Transpose>());
  GlobalEventManager().addCommand("CapCurrentCurve", FreeCaller<Patch_Cap>());
  GlobalEventManager().addCommand("CycleCapTexturePatch", FreeCaller<Patch_CycleProjection>());
  GlobalEventManager().addCommand("ThickenPatch", FreeCaller<patch::thickenSelectedPatches>());
  GlobalEventManager().addCommand("StitchPatchTexture", FreeCaller<patch::stitchPatchTextures>());
}

#include <gtk/gtkbox.h>
#include <gtk/gtktable.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtklabel.h>
#include "gtkutil/dialog.h"
#include "gtkutil/widget.h"

EMessageBoxReturn DoCapDlg(ECapDialog* type)
{
  ModalDialog dialog;
  ModalDialogButton ok_button(dialog, eIDOK);
  ModalDialogButton cancel_button(dialog, eIDCANCEL);
  GtkWidget* bevel;
  GtkWidget* ibevel;
  GtkWidget* endcap;
  GtkWidget* iendcap;
  GtkWidget* cylinder;
 
  GtkWindow* window = create_modal_dialog_window(MainFrame_getWindow(), "Cap", dialog);

  GtkAccelGroup *accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group(window, accel_group);

  {
    GtkHBox* hbox = create_dialog_hbox(4, 4);
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(hbox));

    {
      // Gef: Added a vbox to contain the toggle buttons
      GtkVBox* radio_vbox = create_dialog_vbox(4);
      gtk_container_add(GTK_CONTAINER(hbox), GTK_WIDGET(radio_vbox));
      
      {
        GtkTable* table = GTK_TABLE(gtk_table_new(5, 2, FALSE));
        gtk_widget_show(GTK_WIDGET(table));
        gtk_box_pack_start(GTK_BOX(radio_vbox), GTK_WIDGET(table), TRUE, TRUE, 0);
        gtk_table_set_row_spacings(table, 5);
        gtk_table_set_col_spacings(table, 5);
 
        {
          GtkWidget* image = gtk_image_new_from_pixbuf(GlobalRadiant().getLocalPixbuf("cap_bevel.bmp"));
          gtk_widget_show(image);
          gtk_table_attach(table, image, 0, 1, 0, 1,
                            (GtkAttachOptions) (GTK_FILL),
                            (GtkAttachOptions) (0), 0, 0);
        }
        {
          GtkWidget* image = gtk_image_new_from_pixbuf(GlobalRadiant().getLocalPixbuf("cap_endcap.bmp"));
          gtk_widget_show(image);
          gtk_table_attach(table, image, 0, 1, 1, 2,
                            (GtkAttachOptions) (GTK_FILL),
                            (GtkAttachOptions) (0), 0, 0);
        }
        {
          GtkWidget* image = gtk_image_new_from_pixbuf(GlobalRadiant().getLocalPixbuf("cap_ibevel.bmp"));
          gtk_widget_show(image);
          gtk_table_attach(table, image, 0, 1, 2, 3,
                            (GtkAttachOptions) (GTK_FILL),
                            (GtkAttachOptions) (0), 0, 0);
        }
        {
          GtkWidget* image = gtk_image_new_from_pixbuf(GlobalRadiant().getLocalPixbuf("cap_iendcap.bmp"));
          gtk_widget_show(image);
          gtk_table_attach(table, image, 0, 1, 3, 4,
                            (GtkAttachOptions) (GTK_FILL),
                            (GtkAttachOptions) (0), 0, 0);
        }
        {
          GtkWidget* image = gtk_image_new_from_pixbuf(GlobalRadiant().getLocalPixbuf("cap_cylinder.bmp"));
          gtk_widget_show(image);
          gtk_table_attach(table, image, 0, 1, 4, 5,
                            (GtkAttachOptions) (GTK_FILL),
                            (GtkAttachOptions) (0), 0, 0);
        }

        GSList* group = 0;
        {
          GtkWidget* button = gtk_radio_button_new_with_label (group, "Bevel");
          gtk_widget_show (button);
          gtk_table_attach(table, button, 1, 2, 0, 1,
                            (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                            (GtkAttachOptions) (0), 0, 0);
          group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

          bevel = button;
        }
        {
          GtkWidget* button = gtk_radio_button_new_with_label (group, "Endcap");
          gtk_widget_show (button);
          gtk_table_attach(table, button, 1, 2, 1, 2,
                            (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                            (GtkAttachOptions) (0), 0, 0);
          group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

          endcap = button;
        }
        {
          GtkWidget* button = gtk_radio_button_new_with_label (group, "Inverted Bevel");
          gtk_widget_show (button);
          gtk_table_attach(table, button, 1, 2, 2, 3,
                            (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                            (GtkAttachOptions) (0), 0, 0);
          group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

          ibevel = button;
        }
        {
          GtkWidget* button = gtk_radio_button_new_with_label (group, "Inverted Endcap");
          gtk_widget_show (button);
          gtk_table_attach(table, button, 1, 2, 3, 4,
                            (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                            (GtkAttachOptions) (0), 0, 0);
          group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

          iendcap = button;
        }
        {
          GtkWidget* button = gtk_radio_button_new_with_label (group, "Cylinder");
          gtk_widget_show (button);
          gtk_table_attach(table, button, 1, 2, 4, 5,
                            (GtkAttachOptions) (GTK_FILL | GTK_EXPAND),
                            (GtkAttachOptions) (0), 0, 0);
          group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

          cylinder = button;
        }
      }
    }
    
    {
      GtkVBox* vbox = create_dialog_vbox(4);
      gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(vbox), FALSE, FALSE, 0);
      {
        GtkButton* button = create_modal_dialog_button("OK", ok_button);
        gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(button), FALSE, FALSE, 0);
        widget_make_default(GTK_WIDGET(button));
        gtk_widget_add_accelerator(GTK_WIDGET(button), "clicked", accel_group, GDK_Return, (GdkModifierType)0, GTK_ACCEL_VISIBLE);
      }
      {
        GtkButton* button = create_modal_dialog_button("Cancel", cancel_button);
        gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(button), FALSE, FALSE, 0);
        gtk_widget_add_accelerator(GTK_WIDGET(button), "clicked", accel_group, GDK_Escape, (GdkModifierType)0, GTK_ACCEL_VISIBLE);
      }
    }
  }

  // Initialize dialog
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bevel), TRUE);
  
  EMessageBoxReturn ret = modal_dialog_show(window, dialog);
  if (ret == eIDOK)
  {
    if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (bevel)))
      *type = PATCHCAP_BEVEL;
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(endcap)))
      *type = PATCHCAP_ENDCAP;
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ibevel)))
      *type = PATCHCAP_INVERTED_BEVEL;
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(iendcap)))
      *type = PATCHCAP_INVERTED_ENDCAP;
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cylinder)))
      *type = PATCHCAP_CYLINDER;
  }

  gtk_widget_destroy(GTK_WIDGET(window));

  return ret;
}
