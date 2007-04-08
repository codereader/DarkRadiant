#include "OrthoContextMenu.h"
#include "EntityClassChooser.h"
#include "gtkutil/IconTextMenuItem.h"

#include "selectionlib.h"
#include "scenelib.h"
#include "entity.h" // Entity_createFromSelection()
#include "ientity.h" // Node_getEntity()
#include "map.h" // Scene_countSelectedBrushes()
#include "mainframe.h"

#include "gtkutil/dialog.h"

#include "selection/algorithm/Group.h"
#include "ui/modelselector/ModelSelector.h"

namespace ui
{
	
	namespace {
	
		/* CONSTANTS */
		
		const char* LIGHT_CLASSNAME = "light";
		const char* MODEL_CLASSNAME = "func_static";
		const char* SPEAKER_CLASSNAME = "speaker";
	
		const char* ADD_MODEL_TEXT = "Create model...";
		const char* ADD_MODEL_ICON = "cmenu_add_model.png";
		const char* ADD_LIGHT_TEXT = "Create light...";
		const char* ADD_LIGHT_ICON = "cmenu_add_light.png";
		const char* ADD_ENTITY_TEXT = "Create entity...";
		const char* ADD_ENTITY_ICON = "cmenu_add_entity.png";
		const char* ADD_PREFAB_TEXT = "Insert prefab...";
		const char* ADD_PREFAB_ICON = "cmenu_add_prefab.png";
		const char* ADD_SPEAKER_TEXT = "Create speaker...";
		const char* ADD_SPEAKER_ICON = "icon_sound.png";
		
		const char* CONVERT_TO_STATIC_TEXT = "Convert to func_static";
		const char* CONVERT_TO_STATIC_ICON = "cmenu_convert_static.png";
		
		const char* REVERT_TO_WORLDSPAWN_TEXT = "Revert to worldspawn";
		const char* REVERT_TO_WORLDSPAWN_ICON = "cmenu_revert_worldspawn.png";
	}

// Static class function to display the singleton instance.

void OrthoContextMenu::displayInstance(const Vector3& point) {
	static OrthoContextMenu _instance;
	_instance.show(point);
}

// Constructor. Create GTK widgets here.

OrthoContextMenu::OrthoContextMenu()
: _widget(gtk_menu_new())
{
	GtkWidget* addModel = gtkutil::IconTextMenuItem(ADD_MODEL_ICON, 
													ADD_MODEL_TEXT);
	GtkWidget* addLight = gtkutil::IconTextMenuItem(ADD_LIGHT_ICON, 
													ADD_LIGHT_TEXT);
	GtkWidget* addEntity = gtkutil::IconTextMenuItem(ADD_ENTITY_ICON, 
													 ADD_ENTITY_TEXT);
	GtkWidget* addPrefab = gtkutil::IconTextMenuItem(ADD_PREFAB_ICON, 
													 ADD_PREFAB_TEXT);
	GtkWidget* addSpkr = gtkutil::IconTextMenuItem(ADD_SPEAKER_ICON, 
												   ADD_SPEAKER_TEXT);
	_convertStatic = gtkutil::IconTextMenuItem(CONVERT_TO_STATIC_ICON, 
											   CONVERT_TO_STATIC_TEXT);
	_revertWorldspawn = gtkutil::IconTextMenuItem(REVERT_TO_WORLDSPAWN_ICON, 
												  REVERT_TO_WORLDSPAWN_TEXT);
	
	g_signal_connect(G_OBJECT(addEntity), "activate", G_CALLBACK(callbackAddEntity), this);
	g_signal_connect(G_OBJECT(addLight), "activate", G_CALLBACK(callbackAddLight), this);
	g_signal_connect(G_OBJECT(addModel), "activate", G_CALLBACK(callbackAddModel), this);
	g_signal_connect(G_OBJECT(addPrefab), "activate", G_CALLBACK(callbackAddPrefab), this);
	g_signal_connect(
		G_OBJECT(addSpkr), "activate", G_CALLBACK(_onAddSpeaker), this);
	g_signal_connect(G_OBJECT(_convertStatic), "activate", G_CALLBACK(callbackConvertToStatic), this);
	g_signal_connect(G_OBJECT(_revertWorldspawn), "activate", G_CALLBACK(callbackRevertToWorldspawn), this);

	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), addEntity);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), addModel);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), addLight);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), addSpkr);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), addPrefab);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), gtk_separator_menu_item_new());
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), _convertStatic);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), _revertWorldspawn);
		
	gtk_widget_show_all(_widget);
}

// Show the menu

void OrthoContextMenu::show(const Vector3& point) {
	_lastPoint = point;
	checkConvertStatic(); // enable or disable the convert-to-static command
	checkRevertToWorldspawn();
	gtk_menu_popup(GTK_MENU(_widget), NULL, NULL, NULL, NULL, 1, GDK_CURRENT_TIME);
}

// Check if the convert to static command should be enabled

void OrthoContextMenu::checkConvertStatic() {

	// Command should be enabled if there is at least one selected
	// primitive and no non-primitive selections.
	int numSelections = GlobalSelectionSystem().countSelected();
	if (numSelections > 0 
		&& map::countSelectedPrimitives() == numSelections) 
	{
		gtk_widget_set_sensitive(_convertStatic, TRUE);
	}
	else {
		gtk_widget_set_sensitive(_convertStatic, FALSE);
	}
}

void OrthoContextMenu::checkRevertToWorldspawn() {
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	bool sensitive = false;
	
	// Exactly one entity has to be selected
	if (info.totalCount == 1 && info.entityCount == 1) {
		scene::Instance& instance = GlobalSelectionSystem().ultimateSelected();
		
		sensitive = node_is_group(instance.path().top());
	}
	
	if (sensitive) {
		gtk_widget_set_sensitive(_revertWorldspawn, true);
		gtk_widget_show_all(_revertWorldspawn);
		gtk_widget_hide_all(_convertStatic);
	}
	else {
		gtk_widget_set_sensitive(_revertWorldspawn, false);
		gtk_widget_hide_all(_revertWorldspawn);
		gtk_widget_show_all(_convertStatic);
	}
}

/* GTK CALLBACKS */

void OrthoContextMenu::callbackAddEntity(GtkMenuItem* item, OrthoContextMenu* self) {
	EntityClassChooser::displayInstance(self->_lastPoint);
}

void OrthoContextMenu::callbackAddLight(GtkMenuItem* item, OrthoContextMenu* self) {
	Entity_createFromSelection(LIGHT_CLASSNAME, self->_lastPoint);
}

void OrthoContextMenu::callbackAddPrefab(GtkMenuItem* item, OrthoContextMenu* self) {
	// Pass the call to the map algorithm and give the lastPoint coordinate as argument
	map::loadPrefabAt(self->_lastPoint);
}

void OrthoContextMenu::_onAddSpeaker(GtkMenuItem* item, OrthoContextMenu* self)
{
	Entity_createFromSelection(SPEAKER_CLASSNAME, self->_lastPoint);	
}

void OrthoContextMenu::callbackAddModel(GtkMenuItem* item, OrthoContextMenu* self) {
	
	// To create a model we need EITHER nothing selected OR exactly one brush selected.
	if (GlobalSelectionSystem().countSelected() == 0
		|| map::countSelectedBrushes() == 1) {
	
		// Display the model selector and block waiting for a selection (may be empty)
		ModelAndSkin ms = ui::ModelSelector::chooseModel();
		
		// If a model was selected, create the entity and set its model key
		if (!ms.model.empty()) {
			NodeSmartReference node = Entity_createFromSelection(MODEL_CLASSNAME, self->_lastPoint);
			Node_getEntity(node)->setKeyValue("model", ms.model);
			Node_getEntity(node)->setKeyValue("skin", ms.skin);
		}
		
	}
	else {
		gtkutil::errorDialog("Either nothing or exactly one brush must be selected for model creation",
							 MainFrame_getWindow());
	}

}

void OrthoContextMenu::callbackConvertToStatic(GtkMenuItem* item, OrthoContextMenu* self) {
	// Create a func_static entity. Only brushes can be selected if this menu item is
	// enabled.
	Entity_createFromSelection(MODEL_CLASSNAME, self->_lastPoint);	
}

void OrthoContextMenu::callbackRevertToWorldspawn(GtkMenuItem* item, OrthoContextMenu* self) {
	// Pass the call to the according method
	selection::algorithm::revertGroupToWorldSpawn();	
}

} // namespace ui
