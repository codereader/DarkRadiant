#ifndef BULGEPATCHDIALOG_H_
#define BULGEPATCHDIALOG_H_

#include "gtk/gtkwidget.h"
#include "gtk/gtkwindow.h"

/** greebo: Dialog to query the user for the maxValue
 *  
 */
namespace ui {

class BulgePatchDialog
{
	GtkWindow* _parent;
	
	GtkWidget* _noiseEntry;

public:
	// The dialog widget (is public so that onKeyPress can access it)
	GtkWidget* _dialog;

	// Constructor 
	BulgePatchDialog();
	
	bool queryPatchNoise(int& noise);

private:
	static gboolean onKeyPress(GtkWidget* widget, GdkEventKey* event, BulgePatchDialog* self);

}; // class BulgePatchDialog

} // namespace ui

#endif /*PATCHCREATEDIALOG_H_*/
