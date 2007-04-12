#ifndef PATCHCREATEDIALOG_H_
#define PATCHCREATEDIALOG_H_

#include "gtk/gtkwidget.h"
#include "gtk/gtkwindow.h"

/** greebo: Dialog to query the user for the desired patch dimensions and
 *  whether the selected brushs are to be removed after creation.
 *  
 */
namespace ui {

class PatchCreateDialog
{
	GtkWindow* _parent;
	
	GtkWidget* _comboWidth;
	GtkWidget* _comboHeight;
	GtkWidget* _removeBrushCheckbox;

public:
	// The dialog widget (is public so that onKeyPress can access it)
	GtkWidget* _dialog;

	// Constructor 
	PatchCreateDialog();
	
	/** greebo: Launches the dialog to query the user for the 
	 * desired patch thickness and "addSeams" bool
	 * 
	 * @selBrushCount: The number of selected brushes (used to grey out the
	 * 				   "Remove selected Brushes" checkbox if this is not 1).
	 * 
	 * @returns: TRUE, if the user pressed ok, FALSE if cancelled 
	 * 
	 * @width, height: these contain the selected patch dimensions after return.
	 * @removeBrushes: contains TRUE if the user wishes to remove the selected brush
	 * 				   after the patch is created.
	 */
	bool queryPatchDimensions(int& width, int& height, 
							  const int& selBrushCount, bool& removeBrush);

private:
	static gboolean onKeyPress(GtkWidget* widget, GdkEventKey* event, PatchCreateDialog* self);

}; // class PatchThickenDialog

} // namespace ui

#endif /*PATCHCREATEDIALOG_H_*/
