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

#include "debugging/debugging.h"
#include "warnings.h"

#include "ieventmanager.h"
#include "iscenegraph.h"
#include "iregistry.h"
#include "brush/TexDef.h"
#include "iundo.h"
#include "iselection.h"

#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkframe.h>
#include <gtk/gtklabel.h>
#include <gtk/gtktable.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkspinbutton.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkcheckbutton.h>	//Shamus: For Textool

#include "signal/isignal.h"
#include "generic/object.h"
#include "texturelib.h"
#include "shaderlib.h"
#include "stringio.h"

#include "gtkutil/idledraw.h"
#include "gtkutil/dialog.h"
#include "gtkutil/entry.h"
#include "gtkutil/nonmodal.h"
#include "gtkutil/pointer.h"
#include "gtkutil/glwidget.h"			//Shamus: For Textool
#include "gtkutil/button.h"
#include "map.h"
#include "select.h"
#include "patchmanip.h"
#include "brushmanip.h"
#include "patchdialog.h"
#include "preferences.h"
#include "brush/TextureProjection.h"
#include "mainframe.h"
#include "gtkdlgs.h"
#include "dialog.h"
#include "patch.h"
#include "stream/stringstream.h"
#include "igrid.h"

#include "ui/surfaceinspector/SurfaceInspector.h"
#include "selection/algorithm/Shader.h"
#include "textool/TexTool.h"
#include "brush/BrushInstance.h"
#include "brush/Face.h"

extern FaceInstanceSet g_SelectedFaceInstances;

inline GValue GValue_default()
{
  GValue value;
  value.g_type = 0;
  return value;
}

inline void spin_button_set_step(GtkSpinButton* spin, gfloat step)
{
#if 1
  gtk_spin_button_get_adjustment(spin)->step_increment = step;
#else
  GValue gvalue = GValue_default();
  g_value_init(&gvalue, G_TYPE_DOUBLE);
  g_value_set_double(&gvalue, step);
  g_object_set(G_OBJECT(gtk_spin_button_get_adjustment(spin)), "step-increment", &gvalue, NULL);
#endif
}

class Increment
{
  float& m_f;
public:
  GtkSpinButton* m_spin;
  GtkEntry* m_entry;
  Increment(float& f) : m_f(f), m_spin(0), m_entry(0)
  {
  }
  void cancel()
  {
    entry_set_float(m_entry, m_f);
  }
  typedef MemberCaller<Increment, &Increment::cancel> CancelCaller;
  void apply()
  {
    m_f = static_cast<float>(entry_get_float(m_entry));
    spin_button_set_step(m_spin, m_f);
  }
  typedef MemberCaller<Increment, &Increment::apply> ApplyCaller;
};

void SurfaceInspector_GridChange();

/*class SurfaceInspector : public Dialog
{
  GtkWindow* BuildDialog();

  NonModalEntry m_textureEntry;
  NonModalSpinner m_hshiftSpinner;
  NonModalEntry m_hshiftEntry;
  NonModalSpinner m_vshiftSpinner;
  NonModalEntry m_vshiftEntry;
  NonModalSpinner m_hscaleSpinner;
  NonModalEntry m_hscaleEntry;
  NonModalSpinner m_vscaleSpinner;
  NonModalEntry m_vscaleEntry;
  NonModalSpinner m_rotateSpinner;
  NonModalEntry m_rotateEntry;

  IdleDraw m_idleDraw;

  GtkCheckButton* m_surfaceFlags[32];
  GtkCheckButton* m_contentFlags[32];

  NonModalEntry m_valueEntry;
  GtkEntry* m_valueEntryWidget;
public:
  WindowPositionTracker m_positionTracker;
  WindowPositionTrackerImportStringCaller m_importPosition;
  WindowPositionTrackerExportStringCaller m_exportPosition;

  // Dialog Data
  float m_fitHorizontal;
  float m_fitVertical;

  Increment m_hshiftIncrement;
  Increment m_vshiftIncrement;
  Increment m_hscaleIncrement;
  Increment m_vscaleIncrement;
  Increment m_rotateIncrement;
  GtkEntry* m_texture;

  SurfaceInspector() :
    m_textureEntry(ApplyShaderCaller(*this), UpdateCaller(*this)),
    m_hshiftSpinner(ApplyTexdefCaller(*this), UpdateCaller(*this)),
    m_hshiftEntry(Increment::ApplyCaller(m_hshiftIncrement), Increment::CancelCaller(m_hshiftIncrement)),
    m_vshiftSpinner(ApplyTexdefCaller(*this), UpdateCaller(*this)),
    m_vshiftEntry(Increment::ApplyCaller(m_vshiftIncrement), Increment::CancelCaller(m_vshiftIncrement)),
    m_hscaleSpinner(ApplyTexdefCaller(*this), UpdateCaller(*this)),
    m_hscaleEntry(Increment::ApplyCaller(m_hscaleIncrement), Increment::CancelCaller(m_hscaleIncrement)),
    m_vscaleSpinner(ApplyTexdefCaller(*this), UpdateCaller(*this)),
    m_vscaleEntry(Increment::ApplyCaller(m_vscaleIncrement), Increment::CancelCaller(m_vscaleIncrement)),
    m_rotateSpinner(ApplyTexdefCaller(*this), UpdateCaller(*this)),
    m_rotateEntry(Increment::ApplyCaller(m_rotateIncrement), Increment::CancelCaller(m_rotateIncrement)),
    m_idleDraw(UpdateCaller(*this)),
    m_valueEntry(ApplyFlagsCaller(*this), UpdateCaller(*this)),
    m_importPosition(m_positionTracker),
    m_exportPosition(m_positionTracker),
    m_hshiftIncrement(g_si_globals.shift[0]),
    m_vshiftIncrement(g_si_globals.shift[1]),
    m_hscaleIncrement(g_si_globals.scale[0]),
    m_vscaleIncrement(g_si_globals.scale[1]),
    m_rotateIncrement(g_si_globals.rotate)
  {
    m_fitVertical = 1;
    m_fitHorizontal = 1;
    m_positionTracker.setPosition(c_default_window_pos);
  }

  void constructWindow(GtkWindow* main_window)
  {
    m_parent = main_window;
    Create();
    GlobalGrid().addGridChangeCallback(FreeCaller<SurfaceInspector_GridChange>());
  }
  void destroyWindow()
  {
    Destroy();
  }
  bool visible() const
  {
    return GTK_WIDGET_VISIBLE(const_cast<GtkWindow*>(GetWidget()));
  }
  void queueDraw()
  {
    if(visible())
    {
      m_idleDraw.queueDraw();
    }
  }

  void Update();
  typedef MemberCaller<SurfaceInspector, &SurfaceInspector::Update> UpdateCaller;
  void ApplyShader();
  typedef MemberCaller<SurfaceInspector, &SurfaceInspector::ApplyShader> ApplyShaderCaller;
  void ApplyTexdef();
  typedef MemberCaller<SurfaceInspector, &SurfaceInspector::ApplyTexdef> ApplyTexdefCaller;
  void ApplyFlags();
  typedef MemberCaller<SurfaceInspector, &SurfaceInspector::ApplyFlags> ApplyFlagsCaller;
};*/

void SurfaceInspector_toggleShown()
{
	/*// Check if the user wants to use the "old" surface inspector	
	if (GlobalRegistry().get(RKEY_LEGACY_SURFACE_INSPECTOR) == "1") {
  		if (getSurfaceInspector().visible()) {
    		getSurfaceInspector().HideDlg();
  		}
  		else {
			DoSurface();
		}
	}
	else {*/
		// Toggle the inspector window
		ui::SurfaceInspector::Instance().toggle();
	//}
}

bool SelectedFaces_empty() {
	return g_SelectedFaceInstances.empty();
}

void FlipTextureX() {
	Select_FlipTexture(0);
}

void FlipTextureY() {
	Select_FlipTexture(1);
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
  
  GlobalEventManager().addCommand("FlipTextureX", FreeCaller<FlipTextureX>());
  GlobalEventManager().addCommand("FlipTextureY", FreeCaller<FlipTextureY>());
  
  GlobalEventManager().addCommand("TexToolGridUp", FreeCaller<TexToolGridUp>());
  GlobalEventManager().addCommand("TexToolGridDown", FreeCaller<TexToolGridDown>());
  GlobalEventManager().addCommand("TexToolSnapToGrid", FreeCaller<TexToolSnapToGrid>());
  GlobalEventManager().addCommand("TexToolMergeItems", FreeCaller<TexToolMergeItems>());
  GlobalEventManager().addCommand("TexToolFlipS", FreeCaller<TexToolFlipS>());
  GlobalEventManager().addCommand("TexToolFlipT", FreeCaller<TexToolFlipT>());
  GlobalEventManager().addRegistryToggle("TexToolToggleGrid", "user/ui/textures/texTool/gridActive");
}

void SurfaceInspector_Construct()
{
  //g_SurfaceInspector = new SurfaceInspector;

  SurfaceInspector_registerCommands();

  /*GlobalPreferenceSystem().registerPreference("SurfaceWnd", getSurfaceInspector().m_importPosition, getSurfaceInspector().m_exportPosition);
  GlobalPreferenceSystem().registerPreference("SI_SurfaceTexdef_Scale1", FloatImportStringCaller(g_si_globals.scale[0]), FloatExportStringCaller(g_si_globals.scale[0]));      
  GlobalPreferenceSystem().registerPreference("SI_SurfaceTexdef_Scale2", FloatImportStringCaller(g_si_globals.scale[1]), FloatExportStringCaller(g_si_globals.scale[1]));
  GlobalPreferenceSystem().registerPreference("SI_SurfaceTexdef_Shift1", FloatImportStringCaller(g_si_globals.shift[0]), FloatExportStringCaller(g_si_globals.shift[0]));
  GlobalPreferenceSystem().registerPreference("SI_SurfaceTexdef_Shift2", FloatImportStringCaller(g_si_globals.shift[1]), FloatExportStringCaller(g_si_globals.shift[1]));
  GlobalPreferenceSystem().registerPreference("SI_SurfaceTexdef_Rotate", FloatImportStringCaller(g_si_globals.rotate), FloatExportStringCaller(g_si_globals.rotate));
  GlobalPreferenceSystem().registerPreference("SnapTToGrid", BoolImportStringCaller(g_si_globals.m_bSnapTToGrid), BoolExportStringCaller(g_si_globals.m_bSnapTToGrid));

  typedef FreeCaller1<const Selectable&, SurfaceInspector_SelectionChanged> SurfaceInspectorSelectionChangedCaller;
  GlobalSelectionSystem().addSelectionChangeCallback(SurfaceInspectorSelectionChangedCaller());
  typedef FreeCaller<SurfaceInspector_updateSelection> SurfaceInspectorUpdateSelectionCaller;
  Brush_addTextureChangedCallback(SurfaceInspectorUpdateSelectionCaller());
  Patch_addTextureChangedCallback(SurfaceInspectorUpdateSelectionCaller());

  SurfaceInspector_registerPreferencesPage();*/
}
