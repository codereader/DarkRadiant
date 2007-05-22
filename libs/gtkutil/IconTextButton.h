#ifndef ICONTEXTBUTTON_H_
#define ICONTEXTBUTTON_H_

#include <gtk/gtklabel.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtktogglebutton.h>

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
	
	// Toggle button flag
	bool _isToggle;
	
public:

	/** Construct an IconTextButton with the given label text and local icon
	 * path.
	 * 
	 * @param name
	 * The text to display under the icon.
	 * 
	 * @param iconPath
	 * Name of local icon file.
	 * 
	 * @param isToggle
	 * true if the button should be a toggle button, false for a normal button.
	 */
	IconTextButton(const std::string& name, 
				   GdkPixbuf* icon,
				   bool isToggle)
	: _icon(icon),
	  _label(NULL),
	  _isToggle(isToggle)
	{ 
		if (name != "") {
			_label = gtk_label_new(name.c_str());
		}
	}
	
	/** Operator cast to GtkWidget* packs the widgets and returns a button with
	 * the contents.
	 */
	operator GtkWidget* () {
		// Create vbox containing image and label
		GtkWidget* vbx = gtk_vbox_new(FALSE, 3);
		gtk_box_pack_start(GTK_BOX(vbx), 
						   gtk_image_new_from_pixbuf(_icon),
						   TRUE, FALSE, 0);
		if (_label != NULL) {
			gtk_box_pack_end(GTK_BOX(vbx), _label, TRUE, FALSE, 0);
		}
		
		// Create a button and add the vbox
		GtkWidget* button;
		if (_isToggle)
			button = gtk_toggle_button_new();
		else
			button = gtk_button_new();
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
