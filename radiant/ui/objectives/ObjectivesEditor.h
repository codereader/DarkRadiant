#ifndef OBJECTIVESEDITOR_H_
#define OBJECTIVESEDITOR_H_

#include <gtk/gtkwidget.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtkcellrenderertoggle.h>
#include <gtk/gtktreeselection.h>

namespace ui
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
	
	// Show dialog widgets
	void show();
	
	// Populate the dialog widgets with appropriate state from the map
	void populateWidgets();
	
public:
	
	/**
	 * Static method to display the Objectives Editor dialog.
	 */
	static void displayDialog();
	
};

}

#endif /*OBJECTIVESEDITOR_H_*/
