#ifndef SREDITOR_H_
#define SREDITOR_H_

#include "iselection.h"
#include "ientity.h"
#include "gtkutil/WindowPosition.h"

namespace ui {

class StimResponseEditor :
	public SelectionSystem::Observer
{
	GtkWidget* _dialog;

	// The position/size memoriser
	gtkutil::WindowPosition _windowPosition;
	
	// The entity we're editing
	Entity* _entity;

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

}; // class StimResponseEditor

} // namespace ui

#endif /*SREDITOR_H_*/
