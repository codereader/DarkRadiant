#ifndef PATCHINSPECTOR_H_
#define PATCHINSPECTOR_H_

#include "iselection.h"
#include "gtkutil/WindowPosition.h"

typedef struct _GtkWidget GtkWidget;

namespace ui {

class PatchInspector :
	public SelectionSystem::Observer
{
	// The actual dialog widget
	GtkWidget* _dialog;
	
	// The window position tracker
	gtkutil::WindowPosition _windowPosition;

public:
	PatchInspector();
	
	/** greebo: Contains the static instance of this dialog.
	 * Constructs the instance and calls toggle() when invoked.
	 */
	static PatchInspector& Instance();

	/** greebo: This toggles the visibility of the patch inspector.
	 * The dialog is constructed only once and never destructed 
	 * during runtime.
	 */
	void toggle();

	/** greebo: SelectionSystem::Observer implementation. Gets called by
	 * the SelectionSystem upon selection change to allow updating of the
	 * patch property widgets.
	 */
	void selectionChanged(scene::Instance& instance);

	// Updates the widgets
	void update();

	/** greebo: Safely disconnects this dialog from all systems 
	 * 			(SelectionSystem, EventManager, ...)
	 * 			Also saves the window state to the registry.
	 */
	void shutdown();

private:

	// Creates and packs the widgets into the dialog (called by constructor)
	void populateWindow();

	// The callback for the delete event (toggles the visibility)
	static gboolean onDelete(GtkWidget* widget, GdkEvent* event, PatchInspector* self);

}; // class PatchInspector

} // namespace ui

#endif /*PATCHINSPECTOR_H_*/
