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
#include "gtkutil/menu.h"
#include "gtkutil/image.h"
#include "gtkutil/dialog.h"
#include "map.h"
#include "mainframe.h"
#include "gtkmisc.h"
#include "gtkdlgs.h"
#include "texwindow.h"
#include "select.h"
#include "patch.h"
#include "igrid.h"
#include "xyview/GlobalXYWnd.h"
#include "ui/patch/PatchThickenDialog.h"

PatchCreator* g_patchCreator = 0;

void Scene_PatchConstructPrefab(scene::Graph& graph, const AABB& aabb, const char* shader, EPatchPrefab eType, int axis, std::size_t width = 3, std::size_t height = 3)
{
  GlobalSelectionSystem().setSelectedAll(false);

  NodeSmartReference node(g_patchCreator->createPatch());
  Node_getTraversable(Map_FindOrInsertWorldspawn(g_map))->insert(node);

  Patch* patch = Node_getPatch(node);
  patch->SetShader(shader);

  patch->ConstructPrefab(aabb, eType, axis, width, height);
  patch->controlPointsChanged();

  {
    scene::Path patchpath(makeReference(GlobalSceneGraph().root()));
    patchpath.push(makeReference(*Map_GetWorldspawn(g_map)));
    patchpath.push(makeReference(node.get()));
    Instance_getSelectable(*graph.find(patchpath))->setSelected(true);
  }
}


void Patch_makeCaps(Patch& patch, scene::Instance& instance, EPatchCap type, const char* shader)
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
    NodeSmartReference cap(g_patchCreator->createPatch());
    Node_getTraversable(instance.path().parent())->insert(cap);

    patch.MakeCap(Node_getPatch(cap), type, ROW, true);
    Node_getPatch(cap)->SetShader(shader);

    scene::Path path(instance.path());
    path.pop();
    path.push(makeReference(cap.get()));
    selectPath(path, true);
  }

  {
    NodeSmartReference cap(g_patchCreator->createPatch());
    Node_getTraversable(instance.path().parent())->insert(cap);

    patch.MakeCap(Node_getPatch(cap), type, ROW, false);
    Node_getPatch(cap)->SetShader(shader);

    scene::Path path(instance.path());
    path.pop();
    path.push(makeReference(cap.get()));
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

void Scene_PatchDoCap_Selected(scene::Graph& graph, const char* shader)
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
    scene::Node& node = GlobalSelectionSystem().ultimateSelected().path().top();
    if(node.visible())
    {
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

class PatchNaturalTexture
{
public:
  void operator()(Patch& patch) const
  {
    patch.NaturalTexture();
  }
};

void Scene_PatchNaturalTexture_Selected(scene::Graph& graph)
{
  Scene_forEachVisibleSelectedPatch(PatchNaturalTexture());
  SceneChangeNotify();
}


class PatchInsertRemove
{
  bool m_insert, m_column, m_first;
public:
  PatchInsertRemove(bool insert, bool column, bool first) : m_insert(insert), m_column(column), m_first(first)
  {
  }
  void operator()(Patch& patch) const
  {
    patch.InsertRemove(m_insert, m_column, m_first);
  }
};

void Scene_PatchInsertRemove_Selected(scene::Graph& graph, bool bInsert, bool bColumn, bool bFirst)
{
  Scene_forEachVisibleSelectedPatch(PatchInsertRemove(bInsert, bColumn, bFirst));
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

void Scene_PatchGetShader_Selected(scene::Graph& graph, std::string& name)
{
  Patch* patch = Scene_GetUltimateSelectedVisiblePatch();
  if(patch != 0)
  {
    name = patch->GetShader();
  }
}

class PatchSelectByShader
{
  const char* m_name;
public:
  inline PatchSelectByShader(const char* name)
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

void Scene_PatchSelectByShader(scene::Graph& graph, const char* name)
{
  Scene_forEachVisiblePatchInstance(PatchSelectByShader(name));
}


class PatchFindReplaceShader
{
  const char* m_find;
  const char* m_replace;
public:
  PatchFindReplaceShader(const char* find, const char* replace) : m_find(find), m_replace(replace)
  {
  }
  void operator()(Patch& patch) const
  {
    if(shader_equal(patch.GetShader(), m_find))
    {
      patch.SetShader(m_replace);
    }
  }
};

void Scene_PatchFindReplaceShader(scene::Graph& graph, const char* find, const char* replace)
{
  Scene_forEachVisiblePatch(PatchFindReplaceShader(find, replace));
}

void Scene_PatchFindReplaceShader_Selected(scene::Graph& graph, const char* find, const char* replace)
{
  Scene_forEachVisibleSelectedPatch(PatchFindReplaceShader(find, replace));
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

  Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), TextureBrowser_GetSelectedShader(GlobalTextureBrowser()), eCylinder, GlobalXYWnd().getActiveViewType());
}

void Patch_DenseCylinder()
{
  UndoableCommand undo("patchCreateDenseCylinder");

  Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), TextureBrowser_GetSelectedShader(GlobalTextureBrowser()), eDenseCylinder, GlobalXYWnd().getActiveViewType());
}

void Patch_VeryDenseCylinder()
{
  UndoableCommand undo("patchCreateVeryDenseCylinder");

  Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), TextureBrowser_GetSelectedShader(GlobalTextureBrowser()), eVeryDenseCylinder, GlobalXYWnd().getActiveViewType());
}

void Patch_SquareCylinder()
{
  UndoableCommand undo("patchCreateSquareCylinder");

  Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), TextureBrowser_GetSelectedShader(GlobalTextureBrowser()), eSqCylinder, GlobalXYWnd().getActiveViewType());
}

void Patch_Endcap()
{
  UndoableCommand undo("patchCreateCaps");

  Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), TextureBrowser_GetSelectedShader(GlobalTextureBrowser()), eEndCap, GlobalXYWnd().getActiveViewType());
}

void Patch_Bevel()
{
  UndoableCommand undo("patchCreateBevel");

  Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), TextureBrowser_GetSelectedShader(GlobalTextureBrowser()), eBevel, GlobalXYWnd().getActiveViewType());
}

void Patch_Cone()
{
  UndoableCommand undo("patchCreateCone");

  Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), TextureBrowser_GetSelectedShader(GlobalTextureBrowser()), eCone, GlobalXYWnd().getActiveViewType());
}

void DoNewPatchDlg();

void Patch_Plane()
{
  UndoableCommand undo("patchCreatePlane");

  DoNewPatchDlg();
}

void Patch_InsertInsertColumn()
{
  UndoableCommand undo("patchInsertColumns");

  Scene_PatchInsertRemove_Selected(GlobalSceneGraph(), true, true, false);
}

void Patch_InsertAddColumn()
{
  UndoableCommand undo("patchAddColumns");

  Scene_PatchInsertRemove_Selected(GlobalSceneGraph(), true, true, true);
}

void Patch_InsertInsertRow()
{
  UndoableCommand undo("patchInsertRows");

  Scene_PatchInsertRemove_Selected(GlobalSceneGraph(), true, false, false);
}

void Patch_InsertAddRow()
{
  UndoableCommand undo("patchAddRows");

  Scene_PatchInsertRemove_Selected(GlobalSceneGraph(), true, false, true);
}

void Patch_DeleteFirstColumn()
{
  UndoableCommand undo("patchDeleteFirstColumns");

  Scene_PatchInsertRemove_Selected(GlobalSceneGraph(), false, true, true);
}

void Patch_DeleteLastColumn()
{
  UndoableCommand undo("patchDeleteLastColumns");

  Scene_PatchInsertRemove_Selected(GlobalSceneGraph(), false, true, false);
}

void Patch_DeleteFirstRow()
{
  UndoableCommand undo("patchDeleteFirstRows");

  Scene_PatchInsertRemove_Selected(GlobalSceneGraph(), false, false, true);
}

void Patch_DeleteLastRow()
{
  UndoableCommand undo("patchDeleteLastRows");

  Scene_PatchInsertRemove_Selected(GlobalSceneGraph(), false, false, false);
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

  Scene_PatchDoCap_Selected(GlobalSceneGraph(), TextureBrowser_GetSelectedShader(GlobalTextureBrowser()));
}

void Patch_CycleProjection()
{
  UndoableCommand undo("patchCycleUVProjectionAxis");

  Scene_PatchCapTexture_Selected(GlobalSceneGraph());
}

void Patch_NaturalTexture()
{
  UndoableCommand undo("patchNaturalTexture");

  Scene_PatchNaturalTexture_Selected(GlobalSceneGraph());
}

namespace patch {

/** greebo: This populates the given list with patchinstance references
 */
class PatchFinder
{
public:
	typedef std::list<PatchInstance*> PatchList;

	// The list that is going to be populated
	mutable PatchList& _list;
	
	PatchFinder(PatchList& list) : 
		_list(list) 
	{}

	void operator()(PatchInstance& patch) const {
		_list.push_back(&patch);
	}
};

void thickenPatches(PatchFinder::PatchList list, 
					const float& thickness, 
					const bool& createSeams, 
					const int& axis) 
{
	// Go through the list and thicken all the found ones
	for (PatchFinder::PatchList::iterator i = list.begin();
		 i != list.end();
		 i++)
	{
		// Retrieve the reference from the pointer
		PatchInstance& sourcePatch = *(*i); 
		
		// Create a new patch node
		NodeSmartReference node(g_patchCreator->createPatch());
		// Insert the node into worldspawn
		Node_getTraversable(Map_FindOrInsertWorldspawn(g_map))->insert(node);
	
		// Retrieve the contained patch from the node
		Patch* targetPatch = Node_getPatch(node);
	
		// Create the opposite patch with the given thickness = distance
		targetPatch->createThickenedOpposite(sourcePatch.getPatch(), thickness, axis);
	
		// Now select the newly created patches
		{
			scene::Path patchpath(makeReference(GlobalSceneGraph().root()));
			patchpath.push(makeReference(*Map_GetWorldspawn(g_map)));
			patchpath.push(makeReference(node.get()));
			Instance_getSelectable(*GlobalSceneGraph().find(patchpath))->setSelected(true);
		}
		
		if (createSeams && thickness > 0.0f) {
			// Allocate four new patches
			NodeSmartReference nodes[4] = {
				NodeSmartReference(g_patchCreator->createPatch()),
				NodeSmartReference(g_patchCreator->createPatch()),
				NodeSmartReference(g_patchCreator->createPatch()),
				NodeSmartReference(g_patchCreator->createPatch())
			};
			
			// Now create the four walls
			for (int i = 0; i < 4; i++) {
				// Insert each node into worldspawn 
				Node_getTraversable(Map_FindOrInsertWorldspawn(g_map))->insert(nodes[i]);
				
				// Retrieve the contained patch from the node
				Patch* wallPatch = Node_getPatch(nodes[i]);
				
				// Create the wall patch by passing i as wallIndex
				wallPatch->createThickenedWall(sourcePatch.getPatch(), *targetPatch, i);
				
				// Now select the newly created patches
				{
					scene::Path patchpath(makeReference(GlobalSceneGraph().root()));
					patchpath.push(makeReference(*Map_GetWorldspawn(g_map)));
					patchpath.push(makeReference(nodes[i].get()));
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
	// The list of all the selected patches
	PatchFinder::PatchList list;
	
	// Now populate the list
	Scene_forEachVisibleSelectedPatchInstance(
		PatchFinder(list)
	);
	
	if (list.size() > 0) {
		UndoableCommand undo("patchThicken");
		
		ui::PatchThickenDialog dialog;
		
		bool createSeams = false;
		float thickness = 0.0f;
		// Extrude along normals is the default (axis=3)
		int axis = 3;
		
		if (dialog.queryPatchThickness(thickness, createSeams, axis)) {
			// Thicken all the patches in the list
			thickenPatches(list, thickness, createSeams, axis);
		}
	}
	else {
		gtkutil::errorDialog("Cannot thicken patch. Nothing selected.",
							 MainFrame_getWindow());
	}
}

} // namespace patch

#include "preferences.h"

void Patch_constructPreferences(PrefPage* page)
{
  page->appendEntry("Patch Subdivide Threshold", g_PatchSubdivideThreshold);
}
void Patch_constructPage(PreferenceGroup& group)
{
  PreferencesPage* page(group.createPage("Patches", "Patch Display Preferences"));
  Patch_constructPreferences(reinterpret_cast<PrefPage*>(page));
}
void Patch_registerPreferencesPage()
{
  PreferencesDialog_addDisplayPage(FreeCaller1<PreferenceGroup&, Patch_constructPage>());
}


#include "preferencesystem.h"

void PatchPreferences_construct()
{
  GlobalPreferenceSystem().registerPreference("Subdivisions", IntImportStringCaller(g_PatchSubdivideThreshold), IntExportStringCaller(g_PatchSubdivideThreshold));
}


#include "generic/callback.h"

void Patch_registerCommands()
{
  GlobalEventManager().addCommand("IncPatchColumn", FreeCaller<Patch_InsertInsertColumn>());
  GlobalEventManager().addCommand("IncPatchRow", FreeCaller<Patch_InsertInsertRow>());
  GlobalEventManager().addCommand("DecPatchColumn", FreeCaller<Patch_DeleteLastColumn>());
  GlobalEventManager().addCommand("DecPatchRow", FreeCaller<Patch_DeleteLastRow>());
  GlobalEventManager().addCommand("NaturalizePatch", FreeCaller<Patch_NaturalTexture>());
  GlobalEventManager().addCommand("PatchCylinder", FreeCaller<Patch_Cylinder>());
  GlobalEventManager().addCommand("PatchDenseCylinder", FreeCaller<Patch_DenseCylinder>());
  GlobalEventManager().addCommand("PatchVeryDenseCylinder", FreeCaller<Patch_VeryDenseCylinder>());
  GlobalEventManager().addCommand("PatchSquareCylinder", FreeCaller<Patch_SquareCylinder>());
  GlobalEventManager().addCommand("PatchEndCap", FreeCaller<Patch_Endcap>());
  GlobalEventManager().addCommand("PatchBevel", FreeCaller<Patch_Bevel>());
  GlobalEventManager().addCommand("PatchCone", FreeCaller<Patch_Cone>());
  GlobalEventManager().addCommand("SimplePatchMesh", FreeCaller<Patch_Plane>());
  GlobalEventManager().addCommand("PatchInsertInsertColumn", FreeCaller<Patch_InsertInsertColumn>());
  GlobalEventManager().addCommand("PatchInsertAddColumn", FreeCaller<Patch_InsertAddColumn>());
  GlobalEventManager().addCommand("PatchInsertInsertRow", FreeCaller<Patch_InsertInsertRow>());
  GlobalEventManager().addCommand("PatchInsertAddRow", FreeCaller<Patch_InsertAddRow>());
  GlobalEventManager().addCommand("PatchDeleteFirstColumn", FreeCaller<Patch_DeleteFirstColumn>());
  GlobalEventManager().addCommand("PatchDeleteLastColumn", FreeCaller<Patch_DeleteLastColumn>());
  GlobalEventManager().addCommand("PatchDeleteFirstRow", FreeCaller<Patch_DeleteFirstRow>());
  GlobalEventManager().addCommand("PatchDeleteLastRow", FreeCaller<Patch_DeleteLastRow>());
  GlobalEventManager().addCommand("InvertCurve", FreeCaller<Patch_Invert>());
  GlobalEventManager().addCommand("RedisperseRows", FreeCaller<Patch_RedisperseRows>());
  GlobalEventManager().addCommand("RedisperseCols", FreeCaller<Patch_RedisperseCols>());
  GlobalEventManager().addCommand("MatrixTranspose", FreeCaller<Patch_Transpose>());
  GlobalEventManager().addCommand("CapCurrentCurve", FreeCaller<Patch_Cap>());
  GlobalEventManager().addCommand("CycleCapTexturePatch", FreeCaller<Patch_CycleProjection>());
  GlobalEventManager().addCommand("ThickenPatch", FreeCaller<patch::thickenSelectedPatches>());
}

void Patch_constructMenu(GtkMenu* menu)
{
  createMenuItemWithMnemonic(menu, "Cylinder", "PatchCylinder");
  {
    GtkMenu* menu_in_menu = create_sub_menu_with_mnemonic (menu, "More Cylinders");
    createMenuItemWithMnemonic(menu_in_menu, "Dense Cylinder", "PatchDenseCylinder");
    createMenuItemWithMnemonic(menu_in_menu, "Very Dense Cylinder", "PatchVeryDenseCylinder");
    createMenuItemWithMnemonic(menu_in_menu, "Square Cylinder", "PatchSquareCylinder");
  }
  menu_separator (menu);
  createMenuItemWithMnemonic(menu, "End cap", "PatchEndCap");
  createMenuItemWithMnemonic(menu, "Bevel", "PatchBevel");
  menu_separator (menu);
  createMenuItemWithMnemonic(menu, "Cone", "PatchCone");
  menu_separator (menu);
  createMenuItemWithMnemonic(menu, "Simple Patch Mesh...", "SimplePatchMesh");
  menu_separator (menu);
  {
    GtkMenu* menu_in_menu = create_sub_menu_with_mnemonic (menu, "Insert");
    createMenuItemWithMnemonic(menu_in_menu, "Insert (2) Columns", "PatchInsertInsertColumn");
    createMenuItemWithMnemonic(menu_in_menu, "Add (2) Columns", "PatchInsertAddColumn");
    menu_separator (menu_in_menu);
    createMenuItemWithMnemonic(menu_in_menu, "Insert (2) Rows", "PatchInsertInsertRow");
    createMenuItemWithMnemonic(menu_in_menu, "Add (2) Rows", "PatchInsertAddRow");
  }
  {
    GtkMenu* menu_in_menu = create_sub_menu_with_mnemonic (menu, "Delete");
    createMenuItemWithMnemonic(menu_in_menu, "First (2) Columns", "PatchDeleteFirstColumn");
    createMenuItemWithMnemonic(menu_in_menu, "Last (2) Columns", "PatchDeleteLastColumn");
    menu_separator (menu_in_menu);
    createMenuItemWithMnemonic(menu_in_menu, "First (2) Rows", "PatchDeleteFirstRow");
    createMenuItemWithMnemonic(menu_in_menu, "Last (2) Rows", "PatchDeleteLastRow");
  }
  menu_separator (menu);
  {
    GtkMenu* menu_in_menu = create_sub_menu_with_mnemonic (menu, "Matrix");
    createMenuItemWithMnemonic(menu_in_menu, "Invert", "InvertCurve");
    GtkMenu* menu_3 = create_sub_menu_with_mnemonic (menu_in_menu, "Re-disperse");
    createMenuItemWithMnemonic(menu_3, "Rows", "RedisperseRows");
    createMenuItemWithMnemonic(menu_3, "Columns", "RedisperseCols");
    createMenuItemWithMnemonic(menu_in_menu, "Transpose", "MatrixTranspose");
  }
  menu_separator (menu);
  createMenuItemWithMnemonic(menu, "Thicken Selected Patches", "ThickenPatch");
  createMenuItemWithMnemonic(menu, "Cap Selection", "CapCurrentCurve");
  createMenuItemWithMnemonic(menu, "Cycle Cap Texture", "CycleCapTexturePatch");
}


#include <gtk/gtkbox.h>
#include <gtk/gtktable.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtkradiobutton.h>
#include <gtk/gtkcombobox.h>
#include <gtk/gtklabel.h>
#include "gtkutil/dialog.h"
#include "gtkutil/widget.h"

void DoNewPatchDlg()
{
  ModalDialog dialog;
  GtkComboBox* width;
  GtkComboBox* height;

  GtkWindow* window = create_dialog_window(MainFrame_getWindow(), "Patch density", G_CALLBACK(dialog_delete_callback), &dialog);

  GtkAccelGroup* accel = gtk_accel_group_new();
  gtk_window_add_accel_group(window, accel);

  {
    GtkHBox* hbox = create_dialog_hbox(4, 4);
    gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(hbox));
    {
      GtkTable* table = create_dialog_table(2, 2, 4, 4);
      gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(table), TRUE, TRUE, 0);
      {
        GtkLabel* label = GTK_LABEL(gtk_label_new("Width:"));
        gtk_widget_show(GTK_WIDGET(label));
        gtk_table_attach(table, GTK_WIDGET(label), 0, 1, 0, 1,
                          (GtkAttachOptions) (GTK_FILL),
                          (GtkAttachOptions) (0), 0, 0);
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
      }
      {
        GtkLabel* label = GTK_LABEL(gtk_label_new("Height:"));
        gtk_widget_show(GTK_WIDGET(label));
        gtk_table_attach(table, GTK_WIDGET(label), 0, 1, 1, 2,
                          (GtkAttachOptions) (GTK_FILL),
                          (GtkAttachOptions) (0), 0, 0);
        gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
      }

      {
        GtkComboBox* combo = GTK_COMBO_BOX(gtk_combo_box_new_text());
        gtk_combo_box_append_text(combo, "3");
        gtk_combo_box_append_text(combo, "5");
        gtk_combo_box_append_text(combo, "7");
        gtk_combo_box_append_text(combo, "9");
        gtk_combo_box_append_text(combo, "11");
        gtk_combo_box_append_text(combo, "13");
        gtk_combo_box_append_text(combo, "15");
        gtk_widget_show(GTK_WIDGET(combo));
        gtk_table_attach(table, GTK_WIDGET(combo), 1, 2, 0, 1,
                          (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                          (GtkAttachOptions) (0), 0, 0);

        width = combo;
      }
      {
        GtkComboBox* combo = GTK_COMBO_BOX(gtk_combo_box_new_text());
        gtk_combo_box_append_text(combo, "3");
        gtk_combo_box_append_text(combo, "5");
        gtk_combo_box_append_text(combo, "7");
        gtk_combo_box_append_text(combo, "9");
        gtk_combo_box_append_text(combo, "11");
        gtk_combo_box_append_text(combo, "13");
        gtk_combo_box_append_text(combo, "15");
        gtk_widget_show(GTK_WIDGET(combo));
        gtk_table_attach(table, GTK_WIDGET(combo), 1, 2, 1, 2,
                          (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                          (GtkAttachOptions) (0), 0, 0);

        height = combo;
      }
    }

    {
      GtkVBox* vbox = create_dialog_vbox(4);
      gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(vbox), TRUE, TRUE, 0);
      {
        GtkButton* button = create_dialog_button("OK", G_CALLBACK(dialog_button_ok), &dialog);
        gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(button), FALSE, FALSE, 0);
        widget_make_default(GTK_WIDGET(button));
        gtk_widget_grab_focus(GTK_WIDGET(button));
        gtk_widget_add_accelerator(GTK_WIDGET(button), "clicked", accel, GDK_Return, (GdkModifierType)0, (GtkAccelFlags)0);
      }
      {
        GtkButton* button = create_dialog_button("Cancel", G_CALLBACK(dialog_button_cancel), &dialog);
        gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(button), FALSE, FALSE, 0);
        gtk_widget_add_accelerator(GTK_WIDGET(button), "clicked", accel, GDK_Escape, (GdkModifierType)0, (GtkAccelFlags)0);
      }
    }
  }

  // Initialize dialog
  gtk_combo_box_set_active(width, 0);
  gtk_combo_box_set_active(height, 0);

  if(modal_dialog_show(window, dialog) == eIDOK)
  {
    int w = gtk_combo_box_get_active(width) * 2 + 3;
    int h = gtk_combo_box_get_active(height) * 2 + 3;

    Scene_PatchConstructPrefab(GlobalSceneGraph(), PatchCreator_getBounds(), TextureBrowser_GetSelectedShader(GlobalTextureBrowser()), ePlane, GlobalXYWnd().getActiveViewType(), w, h);
  }

  gtk_widget_destroy(GTK_WIDGET(window));
}




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
          GtkImage* image = new_local_image("cap_bevel.bmp");
          gtk_widget_show(GTK_WIDGET(image));
          gtk_table_attach(table, GTK_WIDGET(image), 0, 1, 0, 1,
                            (GtkAttachOptions) (GTK_FILL),
                            (GtkAttachOptions) (0), 0, 0);
        }
        {
          GtkImage* image = new_local_image("cap_endcap.bmp");
          gtk_widget_show(GTK_WIDGET(image));
          gtk_table_attach(table, GTK_WIDGET(image), 0, 1, 1, 2,
                            (GtkAttachOptions) (GTK_FILL),
                            (GtkAttachOptions) (0), 0, 0);
        }
        {
          GtkImage* image = new_local_image("cap_ibevel.bmp");
          gtk_widget_show(GTK_WIDGET(image));
          gtk_table_attach(table, GTK_WIDGET(image), 0, 1, 2, 3,
                            (GtkAttachOptions) (GTK_FILL),
                            (GtkAttachOptions) (0), 0, 0);
        }
        {
          GtkImage* image = new_local_image("cap_iendcap.bmp");
          gtk_widget_show(GTK_WIDGET(image));
          gtk_table_attach(table, GTK_WIDGET(image), 0, 1, 3, 4,
                            (GtkAttachOptions) (GTK_FILL),
                            (GtkAttachOptions) (0), 0, 0);
        }
        {
          GtkImage* image = new_local_image("cap_cylinder.bmp");
          gtk_widget_show(GTK_WIDGET(image));
          gtk_table_attach(table, GTK_WIDGET(image), 0, 1, 4, 5,
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
