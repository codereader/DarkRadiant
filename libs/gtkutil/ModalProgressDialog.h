#ifndef MODALINFODIALOG_H_
#define MODALINFODIALOG_H_

#include <gtk/gtkwindow.h>

#include <string>

namespace gtkutil
{

/** A simple dialog containing a progress bar and label which can be updated by the 
 * owning process to indicate the progress of a task, such as the loading of
 * textures.
 */

class ModalProgressDialog
{
	// Main dialog widget
	GtkWidget* _widget;
	
	// Label with info text
	GtkWidget* _label;
	
	// Progress bar
	GtkWidget* _progressBar;
	
private:

	// GTK Callback to catch delete-event, to prevent destruction of the
	// window
	static gboolean _onDelete(GtkWidget* widget, gpointer data) {
		return TRUE; // stop event	
	}
	
	// Process the GTK events to ensure the progress bar/text is updated
	// on screen
	void handleEvents();
	
public:

	/** Constructor accepts window to be modal for and the dialog
	 *  title.
	 */
	ModalProgressDialog(GtkWindow* parent, const std::string& title);

	/** Destructor. Destroys window and contained widgets.
	 */
	~ModalProgressDialog() {
		gtk_widget_destroy(_widget);
	}
	
	/** Set the text to display in the label.
	 */
	void setText(const std::string& text);
};

}

#endif /*MODALINFODIALOG_H_*/
