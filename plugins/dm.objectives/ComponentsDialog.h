#ifndef COMPONENTSDIALOG_H_
#define COMPONENTSDIALOG_H_

#include <gtk/gtkwindow.h>

namespace objectives
{

/**
 * Dialog for displaying and editing the components (conditions) attached to
 * a particular objective.
 */
class ComponentsDialog
{
	// Main dialog widget
	GtkWidget* _widget;
	
private:

	/* GTK CALLBACKS */
	static void _onDelete(GtkWidget*, ComponentsDialog*);
	
public:
	
	/**
	 * Constructor creates widgets.
	 * 
	 * @param parent
	 * The parent window for which this dialog should be a transient.
	 */
	ComponentsDialog(GtkWindow* parent);
	
	/**
	 * Display the dialog and enter a recursive main loop, blocking until all
	 * changes are made.
	 */
	void showAndBlock();
};

}

#endif /*COMPONENTSDIALOG_H_*/
