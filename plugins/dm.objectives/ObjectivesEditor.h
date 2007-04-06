#ifndef OBJECTIVESEDITOR_H_
#define OBJECTIVESEDITOR_H_

#include "Objective.h"
#include "ObjectiveEntity.h"

#include <gtk/gtkwidget.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtkcellrenderertoggle.h>
#include <gtk/gtktreeselection.h>
#include <gtk/gtkeditable.h>

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

	// List of actual objectives associated with the selected entity
	GtkListStore* _objectiveList;
	bool _objectiveListLocked; // disable updates during selection change
	
	// Table of dialog subwidgets
	std::map<int, GtkWidget*> _widgets;

	// Pointer to the worldspawn entity
	Entity* _worldSpawn;
	
	// Map of ObjectiveEntity objectives, indexed by the name of the world 
	// entity
	ObjectiveEntityMap _entities;
	
	// Iterators for current entity and current objective
	ObjectiveEntityMap::iterator _curEntity;
	GtkTreeIter _curObjective;

private:

	// Constructor creates widgets	
	ObjectivesEditor();
	
	// Widget construction helpers
	GtkWidget* createEntitiesPanel();
	GtkWidget* createObjectivesPanel();
	GtkWidget* createObjectiveEditPanel();
	GtkWidget* createFlagsTable();
	GtkWidget* createButtons();
	
	// GTK callbacks
	static void _onCancel(GtkWidget* w, ObjectivesEditor* self);
	static void _onOK(GtkWidget*, ObjectivesEditor* self);
	static void _onStartActiveCellToggled(
		GtkCellRendererToggle*, const gchar* path, ObjectivesEditor* self);
	static void _onEntitySelectionChanged(GtkTreeSelection*, ObjectivesEditor*);
	static void _onObjectiveSelectionChanged(GtkTreeSelection*, 
											 ObjectivesEditor*);
	static void _onAddEntity(GtkWidget*, ObjectivesEditor*);
	static void _onDeleteEntity(GtkWidget*, ObjectivesEditor*);
	static void _onAddObjective(GtkWidget*, ObjectivesEditor*);
	static void _onDeleteObjective(GtkWidget*, ObjectivesEditor*);
	static void _onClearObjectives(GtkWidget*, ObjectivesEditor*);
	static void _onFlagToggle(GtkWidget*, ObjectivesEditor*);
	static void _onInitialStateChanged(GtkWidget*, ObjectivesEditor*);
	static void _onDescriptionEdited(GtkEditable*, ObjectivesEditor*);
	static void _onEditComponents(GtkWidget*, ObjectivesEditor*);
	
	// Show dialog widgets
	void show();
	
	// Populate the dialog widgets with appropriate state from the map
	void populateWidgets();
	void populateActiveAtStart();
	
	// Refresh the objectives list from the currently-selected ObjectiveEntity
	void refreshObjectivesList();

	// Populate the edit panel from the selected objective
	void populateEditPanel();
	
	// Return the currently-selected objective
	Objective& getCurrentObjective();
	
public:
	
	/**
	 * Static method to display the Objectives Editor dialog.
	 */
	static void displayDialog();
	
};

}

#endif /*OBJECTIVESEDITOR_H_*/
