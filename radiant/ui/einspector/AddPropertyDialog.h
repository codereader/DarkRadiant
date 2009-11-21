#ifndef ADDPROPERTYDIALOG_H_
#define ADDPROPERTYDIALOG_H_

#include <gtk/gtkwidget.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtktreeselection.h>

#include <string>
#include "ientity.h"

namespace ui
{

/** Modal dialog to display a list of known properties and allow the user
 * to choose one. The dialog is displayed via a single static method which
 * creates the dialog, blocks in a recursive main loop until the choice is
 * made, and then returns the string property that was selected.
 */

class AddPropertyDialog
{
public:
	typedef std::vector<std::string> PropertyList;

private:
	// Main dialog widget
	GtkWidget* _widget;
	
	// Tree view, selection and model
	GtkTreeStore* _treeStore;
	GtkWidget* _treeView;
	GtkTreeSelection* _selection;
	
	// Text view for displaying usage info
	GtkWidget* _usageTextView;
	
	// The selected properties
	PropertyList _selectedProperties;
	
	// Target entity to query for existing spawnargs
	Entity* _entity;
	
private:

	// Create GUI components
	GtkWidget* createTreeView();
	GtkWidget* createButtonsPanel();
	GtkWidget* createUsagePanel();
	
	// Populate tree view with properties
	void populateTreeView();

	void updateUsagePanel();
	
	/* GTK CALLBACKS */
	
	static void _onDelete(GtkWidget*, GdkEvent*, AddPropertyDialog*);
	static void _onOK(GtkWidget*, AddPropertyDialog*);
	static void _onCancel(GtkWidget*, AddPropertyDialog*);
	static void _onSelectionChanged(GtkWidget*, AddPropertyDialog*);
		
	/* Private constructor creates the dialog widgets. Accepts an Entity
	 * to use for populating class-specific keys.
	 */
	AddPropertyDialog(Entity* entity);
	
public:

	/** 
	 * Static method to display an AddPropertyDialog and return the chosen 
	 * property.
	 * 
	 * @param entity
	 * The Entity to be queried for spawnargs.
	 * 
	 * @returns
	 * String list of the chosen properties (e.g. "light_radius").
	 */
	static PropertyList chooseProperty(Entity* entity);

};

}

#endif /*ADDPROPERTYDIALOG_H_*/
