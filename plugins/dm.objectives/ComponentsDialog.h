#ifndef COMPONENTSDIALOG_H_
#define COMPONENTSDIALOG_H_

#include "ce/ComponentEditor.h"

#include <gtk/gtkwindow.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtktreeselection.h>

#include <map>
#include <vector>
#include <string>

#include <gtkutil/window/BlockingTransientWindow.h>

namespace objectives
{

/* FORWARD DECLS */
class Objective;

/**
 * Dialog for displaying and editing the components (conditions) attached to
 * a particular objective.
 */
class ComponentsDialog
: public gtkutil::BlockingTransientWindow
{
	// Widgets map
	std::map<int, GtkWidget*> _widgets;

	// The objective we are editing
	Objective& _objective;
	
	// List store for the components
	GtkListStore* _componentList;
	GtkTreeSelection* _componentSel;
	
	// Currently-active ComponentEditor (if any)
	ce::ComponentEditorPtr _componentEditor;
	
private:

	// Construction helpers
	GtkWidget* createListView();
	GtkWidget* createEditPanel();
	GtkWidget* createComponentEditorPanel();
	GtkWidget* createButtons();

	// Populate the list of components from the Objective's component map
	void populateComponents();
	
	// Populate the edit panel widgets with the specified component number
	void populateEditPanel(int index);
	
	// Get the index of the selected Component, or -1 if there is no selection
	int getSelectedIndex();
	
	/* GTK CALLBACKS */
	static void _onClose(GtkWidget*, ComponentsDialog*);
	static void _onDelete(GtkWidget*, ComponentsDialog*);
	static void _onSelectionChanged(GtkTreeSelection*, ComponentsDialog*);
	static void _onAddComponent(GtkWidget*, ComponentsDialog*);
	static void _onDeleteComponent(GtkWidget*, ComponentsDialog*);
	
	static void _onTypeChanged(GtkWidget*, ComponentsDialog*);
	
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
	
};

}

#endif /*COMPONENTSDIALOG_H_*/
