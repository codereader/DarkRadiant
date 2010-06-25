#include "SelectionSetToolmenu.h"

#include "i18n.h"
#include <gtk/gtktoolitem.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkcomboboxentry.h>

#include "gtkutil/LeftAlignedLabel.h"

namespace selection
{

SelectionSetToolmenu::SelectionSetToolmenu() :
	_toolItem(gtk_tool_item_new()),
	_entry(gtk_combo_box_entry_new_text())
{
	// Hbox containing all our items
	GtkWidget* hbox = gtk_hbox_new(FALSE, 3);
	gtk_container_add(GTK_CONTAINER(_toolItem), hbox);

	// Pack Label
	gtk_box_pack_start(GTK_BOX(hbox), 
		gtkutil::LeftAlignedLabel(_("Selection Set: ")), FALSE, FALSE, 0);

	// Pack Combo Box
	gtk_box_pack_start(GTK_BOX(hbox), _entry, TRUE, TRUE, 0);
}

GtkToolItem* SelectionSetToolmenu::getToolItem()
{
	gtk_widget_show_all(GTK_WIDGET(_toolItem));
	return _toolItem;
}

void SelectionSetToolmenu::onSelectionSetEdited(GtkCellEditable* cellEditable, 
												SelectionSetToolmenu* self)
{
	// TODO: Create new selection set if possible
}

} // namespace
