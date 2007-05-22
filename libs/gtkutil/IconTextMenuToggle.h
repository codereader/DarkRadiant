#ifndef ICONTEXTMENUTOGGLE_H_
#define ICONTEXTMENUTOGGLE_H_

#include <gtk/gtklabel.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkcheckmenuitem.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkimage.h>
#include <string>

namespace gtkutil
{

/** Toggle version of IconTextMenuItem. This class creates a GtkCheckMenuItem
 * containing an icon and a label.
 */

class IconTextMenuToggle
{
private:

	GdkPixbuf* _icon;
	GtkWidget* _label;

public:
	
	// Constructor takes the icon pixbuf and the label text.
	IconTextMenuToggle(GdkPixbuf* icon, const std::string& text)
	: _icon(icon),
	  _label(gtk_label_new(text.c_str())) {}
	  
	// Operator cast to GtkWidget* packs the widgets into an hbox which
	// is then returned.
	operator GtkWidget* () {
		GtkWidget* hbx = gtk_hbox_new(FALSE, 4);
		gtk_box_pack_start(GTK_BOX(hbx), 
    					   gtk_image_new_from_pixbuf(_icon),
						   FALSE,
						   FALSE,
						   0);
		gtk_box_pack_start(GTK_BOX(hbx), _label, FALSE, FALSE, 0);

		GtkWidget* menuItem = gtk_check_menu_item_new();
		gtk_container_add(GTK_CONTAINER(menuItem), hbx);
		return menuItem;
	}
	
};

} // namespace gtkutil

#endif /*ICONTEXTMENUTOGGLE_H_*/
