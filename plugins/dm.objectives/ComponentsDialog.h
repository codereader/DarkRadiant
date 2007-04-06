#ifndef COMPONENTSDIALOG_H_
#define COMPONENTSDIALOG_H_

#include <gtk/gtkwindow.h>
#include <gtk/gtkliststore.h>

namespace objectives
{

/* FORWARD DECLS */
class Objective;

/**
 * Dialog for displaying and editing the components (conditions) attached to
 * a particular objective.
 */
class ComponentsDialog
{
	// Main dialog widget
	GtkWidget* _widget;

	// The objective we are editing
	Objective& _objective;
	
	// List store for the components
	GtkListStore* _componentList;
	
private:

	// Construction helpers
	GtkWidget* createListView();
	GtkWidget* createButtons();

	/* GTK CALLBACKS */
	static void _onDelete(GtkWidget*, ComponentsDialog*);
	
public:
	
	/**
	 * Constructor creates widgets.
	 * 
	 * @param parent
	 * The parent window for which this dialog should be a transient.
	 * 
	 * @param objective
	 * The Objective object for which conditions should be displayed and edited.
	 */
	ComponentsDialog(GtkWindow* parent, Objective& objective);
	
	/**
	 * Display the dialog and enter a recursive main loop, blocking until all
	 * changes are made.
	 */
	void showAndBlock();
};

}

#endif /*COMPONENTSDIALOG_H_*/
