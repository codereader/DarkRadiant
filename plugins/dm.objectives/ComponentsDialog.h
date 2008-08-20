#ifndef COMPONENTSDIALOG_H_
#define COMPONENTSDIALOG_H_

#include "ce/ComponentEditor.h"
#include "DifficultyPanel.h"
#include "Objective.h"

#include <gtk/gtkwindow.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtktogglebutton.h>

#include <map>
#include <vector>
#include <string>

#include "gtkutil/Timer.h"
#include "gtkutil/window/BlockingTransientWindow.h"

namespace objectives
{

/**
 * Dialog for displaying and editing the properties and components (conditions)
 * attached to a particular objective.
 */
class ComponentsDialog : 
	public gtkutil::BlockingTransientWindow
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
	
	// The widgets needed for editing the difficulty levels
	DifficultyPanel _diffPanel;

	// Working set of components we're editing (get written to the objective
	// as soon as the "Save" button is pressed.
	Objective::ComponentMap _components;

	// TRUE while the widgets are populated to disable GTK callbacks
	bool _updateMutex;

	// A timer to periodically update the component list
	gtkutil::Timer _timer;
	
private:

	// Construction helpers
	GtkWidget* createObjectiveEditPanel();
	GtkWidget* createObjectiveFlagsTable();
	GtkWidget* createListView();
	GtkWidget* createEditPanel();
	GtkWidget* createComponentEditorPanel();
	GtkWidget* createButtons();

	// Populate the list of components from the Objective's component map
	void populateComponents();

	// Updates the list store contents without removing any components
	void updateComponents();
	
	// Populate the edit panel widgets with the specified component number
	void populateEditPanel(int index);

	// Populate the objective properties
	void populateObjectiveEditPanel();
	
	// Get the index of the selected Component, or -1 if there is no selection
	int getSelectedIndex();

    // Change the active ComponentEditor
    void changeComponentEditor(Component& compToEdit);

    // If there is a ComponentEditor, write its contents to the Component
    void checkWriteComponent();

	// Writes the data from the widgets to the data structures
	void save();
	
	/* GTK CALLBACKS */
	static void _onSave(GtkWidget*, ComponentsDialog*);
	static void _onCancel(GtkWidget*, ComponentsDialog*);
	static void _onDelete(GtkWidget*, ComponentsDialog*);
	static void _onSelectionChanged(GtkTreeSelection*, ComponentsDialog*);

	static void _onCompToggleChanged(GtkToggleButton*, ComponentsDialog*);

	static void _onAddComponent(GtkWidget*, ComponentsDialog*);
	static void _onDeleteComponent(GtkWidget*, ComponentsDialog*);
	
	static void _onTypeChanged(GtkWidget*, ComponentsDialog*);
    static void _onApplyComponentChanges(GtkWidget*, ComponentsDialog*);

	static gboolean _onIntervalReached(gpointer data);
	
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

	// Destructor performs cleanup
	~ComponentsDialog();
	
};

}

#endif /*COMPONENTSDIALOG_H_*/
