#include "OrthoContextMenu.h"
#include "IconMenuLabel.h"
#include "EntityClassChooser.h"

#include "entity.h" // Entity_createFromSelection()

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
	
	gtk_widget_set_sensitive(addPrefab, FALSE);
	
	g_signal_connect(G_OBJECT(addEntity), "activate", G_CALLBACK(callbackAddEntity), this);
	g_signal_connect(G_OBJECT(addLight), "activate", G_CALLBACK(callbackAddLight), this);
	g_signal_connect(G_OBJECT(addModel), "activate", G_CALLBACK(callbackAddModel), this);

	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), addModel);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), addLight);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), addEntity);
	gtk_menu_shell_append(GTK_MENU_SHELL(_widget), addPrefab);
		
	gtk_widget_show_all(_widget);
}

// Show the menu

void OrthoContextMenu::show(const Vector3& point) {
	_lastPoint = point;
	gtk_menu_popup(GTK_MENU(_widget), NULL, NULL, NULL, NULL, 1, GDK_CURRENT_TIME);
}

/* GTK CALLBACKS */

void OrthoContextMenu::callbackAddEntity(GtkMenuItem* item, OrthoContextMenu* self) {
	EntityClassChooser::displayInstance(self->_lastPoint);
}

void OrthoContextMenu::callbackAddLight(GtkMenuItem* item, OrthoContextMenu* self) {
	Entity_createFromSelection(LIGHT_CLASSNAME, self->_lastPoint);
}

void OrthoContextMenu::callbackAddModel(GtkMenuItem* item, OrthoContextMenu* self) {
	Entity_createFromSelection(MODEL_CLASSNAME, self->_lastPoint);
}

} // namespace ui
