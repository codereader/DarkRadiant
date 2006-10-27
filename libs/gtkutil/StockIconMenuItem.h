#ifndef STOCKICONMENUITEM_H_
#define STOCKICONMENUITEM_H_

#include <gtk/gtklabel.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkimage.h>
#include <string>

namespace gtkutil
{

/** Utility class representing a menu item with an icon and text. The 
 * menu item consists of a stock icon from GTK, and a text label.
 */

class StockIconMenuItem
{
private:

	GtkWidget* _icon;
	GtkWidget* _label;

public:
	
	// Constructor takes the icon name and the label text.
	StockIconMenuItem(const gchar* stockName, const std::string& text)
	: _icon(gtk_image_new_from_stock(stockName, GTK_ICON_SIZE_MENU)),
	  _label(gtk_label_new(text.c_str())) {}
	  
	// Operator cast to GtkWidget* packs the widgets into an hbox which
	// is then returned.
	operator GtkWidget* () {
		GtkWidget* hbx = gtk_hbox_new(FALSE, 4);
		gtk_box_pack_start(GTK_BOX(hbx), 
    					   _icon,
						   FALSE,
						   FALSE,
						   0);
		gtk_box_pack_start(GTK_BOX(hbx), _label, FALSE, FALSE, 0);

		GtkWidget* menuItem = gtk_menu_item_new();
		gtk_container_add(GTK_CONTAINER(menuItem), hbx);
		return menuItem;
	}
	
};

} // namespace gtkutil

#endif /*STOCKICONMENUITEM_H_*/
