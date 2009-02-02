#ifndef PATCHTHICKENDIALOG_
#define PATCHTHICKENDIALOG_

#include "gtk/gtkwidget.h"
#include "gtk/gtkwindow.h"

/** greebo: Dialog to query the user for the desired patch thickness and
 *  if seams are to be created.
 * 
 */
namespace ui {

class PatchThickenDialog
{
	GtkWindow* _parent;
	
	GtkWidget* _thicknessEntry;
	GtkWidget* _seamsCheckBox;
	
	GtkWidget* _dialog;
	
	GtkWidget* _radNormals;
	GtkWidget* _radX;
	GtkWidget* _radY;
	GtkWidget* _radZ;

public:
	// Constructor, instantiate this class by specifying the parent window 
	PatchThickenDialog();
	
	/** greebo: Launches the dialog to query the user for the 
	 * desired patch thickness and "addSeams" bool
	 * 
	 * @returns: TRUE, if the user pressed ok, FALSE if cancelled 
	 */
	bool queryPatchThickness(float& thickness, bool& createSeams, int& axis);

}; // class PatchThickenDialog

} // namespace ui

#endif /*PATCHTHICKENDIALOG_*/
