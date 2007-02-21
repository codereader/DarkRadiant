#ifndef OBJECTIVESEDITOR_H_
#define OBJECTIVESEDITOR_H_

#include <gtk/gtkwidget.h>

namespace ui
{

/**
 * Dialog for adding and manipulating mission objectives in Dark Mod missions.
 */
class ObjectivesEditor
{
	// Dialog window
	GtkWidget* _widget;

private:

	// Constructor creates widgets	
	ObjectivesEditor();
	
	// Show dialog widgets
	void show();
	
public:
	
	/**
	 * Static method to display the Objectives Editor dialog.
	 */
	static void displayDialog();
	
};

}

#endif /*OBJECTIVESEDITOR_H_*/
