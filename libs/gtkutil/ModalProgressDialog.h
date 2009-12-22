#ifndef MODALINFODIALOG_H_
#define MODALINFODIALOG_H_

#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>

#include <string>
#include <stdexcept>
#include <boost/shared_ptr.hpp>

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
	
	// Flag to indicate the operation has aborted
	bool _aborted;
	
private:

	// GTK Callback to catch delete-event, to prevent destruction of the
	// window
	static gboolean _onDelete(GtkWidget* widget, gpointer data) {
		return TRUE; // stop event	
	}
	
	// Cancel button callback
	static void _onCancel(GtkWidget*, ModalProgressDialog*);
	
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
	
	/**
	 * Exception thrown when cancel button is pressed.
	 */
	struct OperationAbortedException
	: public std::runtime_error
	{
		OperationAbortedException(const std::string& what)
		: std::runtime_error(what) {}		
	};
	
	/** 
	 * Set the text to display in the label, and pulse the progress bar. If the
	 * user has clicked the Cancel button since the last update, this method
	 * will throw an exception to indicate an aborted operation.
	 */
	void setText(const std::string& text);

	/** 
	 * Set the text to display in the label, and the completed fraction of the progress bar. 
	 * If the user has clicked the Cancel button since the last update, this method
	 * will throw an exception to indicate an aborted operation.
	 */
	void setTextAndFraction(const std::string& text, double fraction);
};
typedef boost::shared_ptr<ModalProgressDialog> ModalProgressDialogPtr;

}

#endif /*MODALINFODIALOG_H_*/
