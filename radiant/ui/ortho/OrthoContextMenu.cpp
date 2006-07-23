#include "OrthoContextMenu.h"
#include "IconMenuLabel.h"
#include "EntityClassChooser.h"

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
	GtkWidget* addModel = IconMenuLabel("cmenu_add_model.png", "Add model...");
	GtkWidget* addLight = IconMenuLabel("cmenu_add_light.png", "Add light...");
	GtkWidget* addEntity = IconMenuLabel("cmenu_add_entity.png", "Add entity...");
	GtkWidget* addPrefab = IconMenuLabel("cmenu_add_prefab.png", "Add prefab...");
	
	g_signal_connect(G_OBJECT(addEntity), "activate", G_CALLBACK(callbackAddEntity), this);

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

} // namespace ui
