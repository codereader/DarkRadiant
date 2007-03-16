#ifndef SREDITOR_H_
#define SREDITOR_H_

#include "iselection.h"
#include "ientity.h"
#include "gtkutil/WindowPosition.h"

#include "StimTypes.h"
#include "SREntity.h"

typedef struct _GtkTreeView GtkTreeView;

namespace ui {

class StimResponseEditor :
	public SelectionSystem::Observer
{
	GtkWidget* _dialog;
	
	GtkWidget* _dialogVBox;
	
	// The combobox using a liststore model filled with stims
	GtkWidget* _stimTypeList; 
	
	// The treeview with the entity's stims/responses
	GtkWidget* _entitySRView;
	
	struct SRPropertyWidgets {
		GtkWidget* vbox;
		GtkWidget* stimButton;
		GtkWidget* respButton;
	} _srWidgets;
	
	// The list of the entity's stims/responses
	SREntityPtr _srEntity;
	
	// The position/size memoriser
	gtkutil::WindowPosition _windowPosition;
	
	// The entity we're editing
	Entity* _entity;
	
	// The helper class managing the stims
	StimTypes _stimTypes;

public:
	StimResponseEditor();
	
	/** greebo: Contains the static instance of this dialog.
	 */
	static StimResponseEditor& Instance();
	
	// Command target to toggle the dialog
	static void toggle();
	
	/** greebo: Some sort of "soft" destructor that de-registers
	 * this class from the SelectionSystem, saves the window state, etc.
	 */
	void shutdown();
	
	/** greebo: Gets called by the SelectionSystem
	 */
	void selectionChanged(scene::Instance& instance);

private:
	/** greebo: Updates the SR widget group according to the list selection.
	 */
	void updateSRWidgets();

	/** greebo: Creates the S/R widget group and returns its vbox
	 */
	GtkWidget* createSRWidgets();

	/** greebo: This fills the window with widgets
	 */
	void populateWindow();
	
	/** greebo: This updates the widget sensitivity and loads
	 * 			the data into them.
	 */
	void update();

	/** greebo: Checks the selection for a single entity.
	 */
	void rescanSelection();

	/** greebo: This toggles the visibility of the surface dialog.
	 * The dialog is constructed only once and never destructed 
	 * during runtime.
	 */
	void toggleWindow();
	
	// The callback for the delete event (toggles the visibility)
	static gboolean onDelete(GtkWidget* widget, GdkEvent* event, StimResponseEditor* self);

	// Callback for Stim/Response selection changes
	static void onSelectionChange(GtkTreeView* treeView, StimResponseEditor* self);

}; // class StimResponseEditor

} // namespace ui

#endif /*SREDITOR_H_*/
