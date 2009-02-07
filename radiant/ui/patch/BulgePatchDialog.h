#ifndef BULGEPATCHDIALOG_H_
#define BULGEPATCHDIALOG_H_

#include <gtk/gtkwidget.h>

/** 
 * Jesps: Dialog to query the user for the maxValue  
 */
namespace ui {

class BulgePatchDialog {
	// The parent window plus entry box
	GtkWidget* _noiseEntry;

	// The dialog widget
	GtkWidget* _dialog;
public:
	// Constructor 
	BulgePatchDialog();
	
	// Shows the dialog
	static bool queryPatchNoise(int& noise);

private:
	// GTK callback to allow ESC/Enter reactions
	static gboolean onKeyPress(GtkWidget* widget, GdkEventKey* event, BulgePatchDialog* self);

}; // class BulgePatchDialog

} // namespace ui

#endif /*PATCHCREATEDIALOG_H_*/
