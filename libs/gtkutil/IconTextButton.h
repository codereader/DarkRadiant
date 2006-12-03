#ifndef ICONTEXTBUTTON_H_
#define ICONTEXTBUTTON_H_

#include "image.h"

#include <gtk/gtklabel.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkbutton.h>

namespace gtkutil
{

/** Button with an icon above and a title underneath.
 */

class IconTextButton
{
	// Icon pixbuf
	GdkPixbuf* _icon;
	
	// Label widget
	GtkWidget* _label;
	
public:

	/** Construct an IconTextButton with the given label text and local icon
	 * path.
	 */
	IconTextButton(const std::string& name, const std::string& iconPath)
	: _icon(gtkutil::getLocalPixbuf(iconPath)),
	  _label(gtk_label_new(name.c_str())) 
	{ }
	
	/** Operator cast to GtkWidget* packs the widgets and returns a button with
	 * the contents.
	 */
	operator GtkWidget* () {
		// Create vbox containing image and label
		GtkWidget* vbx = gtk_vbox_new(FALSE, 3);
		gtk_box_pack_start(GTK_BOX(vbx), 
						   gtk_image_new_from_pixbuf(_icon),
						   TRUE, FALSE, 0);
		gtk_box_pack_end(GTK_BOX(vbx), _label, TRUE, FALSE, 0);
		
		// Create a button and add the vbox
		GtkWidget* button = gtk_button_new();
		gtk_container_add(GTK_CONTAINER(button), vbx);

		// Set the button to standard size
		int w = 3 * gdk_pixbuf_get_width(_icon);
		gtk_widget_set_size_request(button, w, -1);

		// Return the button
		return button;
	}
};

}

#endif /*ICONTEXTBUTTON_H_*/
