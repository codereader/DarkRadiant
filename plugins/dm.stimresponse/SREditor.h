#ifndef SREDITOR_H_
#define SREDITOR_H_

#include "gtkutil/WindowPosition.h"

namespace ui {

class StimResponseEditor
{
	GtkWidget* _dialog;

	// The position/size memoriser
	gtkutil::WindowPosition _windowPosition;

public:
	StimResponseEditor();
	
	static StimResponseEditor& Instance();
	
	// Command target to toggle the dialog
	static void toggle();
	
	/** greebo: Some sort of "soft" destructor that de-registers
	 * this class from the SelectionSystem, saves the window state, etc.
	 */
	void shutdown();

private:
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
