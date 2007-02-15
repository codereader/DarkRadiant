#ifndef SURFACEINSPECTOR_H_
#define SURFACEINSPECTOR_H_

#include <gtk/gtkwidget.h>

namespace ui {

class SurfaceInspector
{
	// The actual dialog window
	GtkWidget* _dialog;

public:

	// Constructor
	SurfaceInspector();

	// Shows/hides this dialog
	void toggle();

	/** greebo: Contains the static instance of this dialog.
	 * Constructs the instance and calls toggle() when invoked.
	 */
	static void toggleInspector();
	
}; // class SurfaceInspector

} // namespace ui

#endif /*SURFACEINSPECTOR_H_*/
