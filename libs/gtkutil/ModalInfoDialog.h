#ifndef MODALINFODIALOG_H_
#define MODALINFODIALOG_H_

#include <gtk/gtkwindow.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkvbox.h>

#include <string>

namespace gtkutil
{

/** A simple dialog containing a Label which can be updated by the owning
 * process to indicate the progress of a task, such as the loading of
 * textures.
 */

class ModalInfoDialog
{
	// Main dialog widget
	GtkWidget* _widget;
	
	// Label with info text
	GtkWidget* _label;
	
private:

	// GTK Callback to catch delete-event, to prevent destruction of the
	// window
	static gboolean _onDelete(GtkWidget* widget, gpointer data) {
		return TRUE; // stop event	
	}
	
public:

	/** Constructor accepts window to be modal for and the dialog
	 *  title.
	 */
	ModalInfoDialog(GtkWindow* parent, const std::string& title)
	: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
	  _label(gtk_label_new("")) 
	{
	  	// Window properties
		gtk_window_set_transient_for(GTK_WINDOW(_widget), parent);
		gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
		gtk_window_set_title(GTK_WINDOW(_widget), title.c_str());
    	gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
		gtk_window_set_default_size(GTK_WINDOW(_widget), 360, 80);
    
    	g_signal_connect(G_OBJECT(_widget), "delete-event", G_CALLBACK(_onDelete), NULL);
    
    	// Pack the label into the window
    	GtkWidget* vbx = gtk_vbox_new(FALSE, 0);
    	gtk_box_pack_start(GTK_BOX(vbx), _label, TRUE, FALSE, 0);
    	gtk_container_add(GTK_CONTAINER(_widget), vbx);
    	gtk_container_set_border_width(GTK_CONTAINER(_widget), 6);
    	
    	// Show the window
    	gtk_widget_show_all(_widget);
	}
	
	/** Destructor. Destroys window and contained widgets.
	 */
	~ModalInfoDialog() {
		gtk_widget_destroy(_widget);
	}
	
	/** Set the text to display in the label.
	 */
	void setText(const std::string& text) {
		gtk_label_set_markup(GTK_LABEL(_label), text.c_str());
	}
};

}

#endif /*MODALINFODIALOG_H_*/
