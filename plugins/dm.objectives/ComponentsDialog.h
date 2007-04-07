#ifndef COMPONENTSDIALOG_H_
#define COMPONENTSDIALOG_H_

#include <gtk/gtkwindow.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtktreeselection.h>

#include <map>
#include <vector>
#include <string>

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

	// Widgets map
	std::map<int, GtkWidget*> _widgets;

	// The objective we are editing
	Objective& _objective;
	
	// List store for the components
	GtkListStore* _componentList;
	GtkTreeSelection* _componentSel;
	
private:

	// Construction helpers
	GtkWidget* createListView();
	GtkWidget* createEditPanel();
	GtkWidget* createButtons();

	// Static list of component type strings, in order
	typedef std::vector<std::string> StringList;
	static const StringList& getTypeStrings();

	// Populate the edit panel widgets with the specified component number
	void populateEditPanel(int index);
	
	/* GTK CALLBACKS */
	static void _onClose(GtkWidget*, ComponentsDialog*);
	static void _onDelete(GtkWidget*, ComponentsDialog*);
	static void _onSelectionChanged(GtkTreeSelection*, ComponentsDialog*);
	
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
