#ifndef ICONMENULABEL_H_
#define ICONMENULABEL_H_

#include <gtk/gtklabel.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtkimage.h>
#include <string>

#include <gtkmm/label.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/box.h>
#include <gtkmm/image.h>

namespace gtkutil
{

/** Utility class representing a menu item with an icon and text. The local
 * icon image name and the label text are passed to the constructor, which
 * creates the required GtkWidgets and returns them as part of a static
 * cast to GtkWidget*. This class does NOT manage the resource allocations,
 * the widgets will be created with the assumption that they will be packed
 * into a GtkMenu which will be responsible for destroying the widgets if
 * required.
 */

class IconTextMenuItem
{
private:

	GdkPixbuf* _icon;
	GtkWidget* _label;

public:
	
	// Constructor takes the icon name and the label text.
	IconTextMenuItem(GdkPixbuf* icon, const std::string& text)
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

		GtkWidget* menuItem = gtk_menu_item_new();
		gtk_container_add(GTK_CONTAINER(menuItem), hbx);
		return menuItem;
	}
	
};

class IconTextMenuItemmm :
	public Gtk::MenuItem
{
public:
	// Constructor takes the icon name and the label text.
	IconTextMenuItemmm(const Glib::RefPtr<Gdk::Pixbuf>& icon, const std::string& text)
	{
		Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 4));

		hbox->pack_start(*Gtk::manage(new Gtk::Image(icon)), false, false, 0);
		hbox->pack_start(*Gtk::manage(new Gtk::Label(text)), false, false, 0);

		add(*hbox);
	}
};

} // namespace gtkutil

#endif /*ICONMENULABEL_H_*/
