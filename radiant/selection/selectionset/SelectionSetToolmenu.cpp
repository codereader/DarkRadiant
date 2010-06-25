#include "SelectionSetToolmenu.h"

#include <gtk/gtktoolitem.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkcomboboxentry.h>

namespace selection
{

SelectionSetToolmenu::SelectionSetToolmenu() :
	_toolItem(gtk_tool_item_new()),
	_entry(gtk_combo_box_entry_new())
{
	// Hbox containing all our items
	GtkWidget* hbox = gtk_hbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(_toolItem), hbox);

	gtk_box_pack_start(GTK_BOX(hbox), _entry, TRUE, TRUE, 0);
}

GtkToolItem* SelectionSetToolmenu::getToolItem()
{
	gtk_widget_show_all(GTK_WIDGET(_toolItem));
	return _toolItem;
}

} // namespace
