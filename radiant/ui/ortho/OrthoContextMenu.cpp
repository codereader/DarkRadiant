#include "OrthoContextMenu.h"
#include "IconMenuLabel.h"
#include "EntityClassChooser.h"

#include "entity.h" // Entity_createFromSelection()
#include "ientity.h" // Node_getEntity()
#include "map.h" // Scene_countSelectedBrushes()

#include "gtkutil/dialog.h"

#include "ui/modelselector/ModelSelector.h"

namespace ui
{

// Static class function to display the singleton instance.

void OrthoContextMenu::displayInstance(const Vector3& point) {
	static OrthoContextMenu _instance;
	_instance.show(point);
}

// Constructor. Create GTK widgets here.

OrthoContextMenu::OrthoContextMenu()
: _widget(gtk_menu_new())
{
	GtkWidget* addModel = IconMenuLabel(ADD_MODEL_ICON, ADD_MODEL_TEXT);
	GtkWidget* addLight = IconMenuLabel(ADD_LIGHT_ICON, ADD_LIGHT_TEXT);
	GtkWidget* addEntity = IconMenuLabel(ADD_ENTITY_ICON, ADD_ENTITY_TEXT);
	GtkWidget* addPrefab = IconMenuLabel(ADD_PREFAB_ICON, ADD_PREFAB_TEXT);
	_convertStatic = IconMenuLabel(CONVERT_TO_STATIC_ICON, CONVERT_TO_STATIC_TEXT);
	
	gtk_widget_set_sensitive(addPrefab, FALSE);
	
	g_signal_connect(G_OBJECT(addEntity), "activate", G_CALLBACK(callbackAddEntity), this);
	g_signal_connect(G_OBJECT(addLight), "activate", G_CALLBACK(callbackAddLight), this);
	g_signal_connect(G_OBJECT(addModel), "activate", G_CALLBACK(callbackAddModel), this);
	g_signal_connect(G_OBJECT(_convertStatic), "activate", G_CALLBACK(callbackConvertToStatic), this);

	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), addModel);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), addLight);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), addEntity);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), addPrefab);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), gtk_separator_menu_item_new());
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), _convertStatic);
		
	gtk_widget_show_all(_widget);
}

// Show the menu

void OrthoContextMenu::show(const Vector3& point) {
	_lastPoint = point;
	checkConvertStatic(); // enable or disable the convert-to-static command
	gtk_menu_popup(GTK_MENU(_widget), NULL, NULL, NULL, NULL, 1, GDK_CURRENT_TIME);
}

// Check if the convert to static command should be enabled

void OrthoContextMenu::checkConvertStatic() {

	// Command should be enabled if there is at least one selected
	// brush and no non-brush selections.
	int numSelections = GlobalSelectionSystem().countSelected();
	if (numSelections > 0 
		&& map::countSelectedBrushes() == numSelections) 
	{
		gtk_widget_set_sensitive(_convertStatic, TRUE);
	}
	else {
		gtk_widget_set_sensitive(_convertStatic, FALSE);
	}
}

/* GTK CALLBACKS */

void OrthoContextMenu::callbackAddEntity(GtkMenuItem* item, OrthoContextMenu* self) {
	EntityClassChooser::displayInstance(self->_lastPoint);
}

void OrthoContextMenu::callbackAddLight(GtkMenuItem* item, OrthoContextMenu* self) {
	Entity_createFromSelection(LIGHT_CLASSNAME, self->_lastPoint);
}

void OrthoContextMenu::callbackAddModel(GtkMenuItem* item, OrthoContextMenu* self) {
	
	// To create a model we need EITHER nothing selected OR exactly one brush selected.
	if (GlobalSelectionSystem().countSelected() == 0
		|| map::countSelectedBrushes() == 1) {
	
		// Display the model selector and block waiting for a selection (may be empty)
		std::string model = ui::ModelSelector::chooseModel();
		
		// If a model was selected, create the entity and set its model key
		if (!model.empty()) {
			NodeSmartReference node = Entity_createFromSelection(MODEL_CLASSNAME, self->_lastPoint);
			Node_getEntity(node)->setKeyValue("model", model);
		}
		
	}
	else {
		gtkutil::errorDialog("Either nothing or exactly one brush must be selected for model creation");
	}

}

void OrthoContextMenu::callbackConvertToStatic(GtkMenuItem* item, OrthoContextMenu* self) {
	// Create a func_static entity. Only brushes can be selected if this menu item is
	// enabled.
	Entity_createFromSelection(MODEL_CLASSNAME, self->_lastPoint);	
}

} // namespace ui
