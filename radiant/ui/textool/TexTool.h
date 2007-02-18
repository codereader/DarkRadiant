#ifndef TEXTOOL_H_
#define TEXTOOL_H_

#include "gtk/gtkwidget.h"
#include "gtkutil/WindowPosition.h"

namespace ui {

class TexTool
{
	// The textool gtkwindow
	GtkWidget* _window;

	// The window position tracker
	gtkutil::WindowPosition _windowPosition;

public:
	TexTool();
	
	/** greebo: Toggles the visibility of this TexTool instance.
	 * The actual static instance is owned by the Instane() method.
	 */
	void toggle();
	
	/** greebo: Some sort of "soft" destructor that de-registers
	 * this class from the SelectionSystem, saves the window state, etc.
	 */
	void shutdown();
	
	/** greebo: This is the static accessor method containing
	 * the static instance of the TexTool class. Use this to access
	 * the public member methods like toggle() and shutdown().
	 */
	static TexTool& Instance();
	
private:
	// The callback for the delete event (toggles the visibility)
	static gboolean onDelete(GtkWidget* widget, GdkEvent* event, TexTool* self);

}; // class TexTool

} // namespace ui

#endif /*TEXTOOL_H_*/
