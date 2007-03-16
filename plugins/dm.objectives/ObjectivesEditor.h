#ifndef OBJECTIVESEDITOR_H_
#define OBJECTIVESEDITOR_H_

#include <gtk/gtkwidget.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtkcellrenderertoggle.h>
#include <gtk/gtktreeselection.h>

#include <map>
#include <string>

/* FORWARD DECLS */
class Entity;

namespace objectives
{

/**
 * Dialog for adding and manipulating mission objectives in Dark Mod missions.
 */
class ObjectivesEditor
{
	// Dialog window
	GtkWidget* _widget;

	// List of target_addobjectives entities
	GtkListStore* _objectiveEntityList;

	// Tree of actual objectives associated with the selected entity
	GtkTreeStore* _objTreeStore;
	
	// Table of dialog subwidgets
	std::map<std::string, GtkWidget*> _widgets;

	// Pointer to the worldspawn entity
	Entity* _worldSpawn;

private:

	// Constructor creates widgets	
	ObjectivesEditor();
	
	// Widget construction helpers
	GtkWidget* createEntitiesPanel();
	GtkWidget* createObjectivesPanel();
	GtkWidget* createButtons();
	
	// GTK callbacks
	static void _onCancel(GtkWidget* w, ObjectivesEditor* self);
	static void _onStartActiveCellToggled(
		GtkCellRendererToggle*, const gchar* path, ObjectivesEditor* self);
	static void _onEntitySelectionChanged(GtkTreeSelection*, ObjectivesEditor*);
	static void _onAddEntity(GtkWidget*, ObjectivesEditor*);
	static void _onDeleteEntity(GtkWidget*, ObjectivesEditor*);
	
	// Show dialog widgets
	void show();
	
	// Populate the dialog widgets with appropriate state from the map
	void populateWidgets();
	void populateActiveAtStart();
	
	// Populate the objective tree with values from the selected entity
	void populateObjectiveTree(Entity* entity);
	
public:
	
	/**
	 * Static method to display the Objectives Editor dialog.
	 */
	static void displayDialog();
	
};

}

#endif /*OBJECTIVESEDITOR_H_*/
