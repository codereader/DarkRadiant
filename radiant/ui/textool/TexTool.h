#ifndef TEXTOOL_H_
#define TEXTOOL_H_

#include "gtk/gtkwidget.h"
#include "gtkutil/WindowPosition.h"
#include "math/Vector3.h"
#include "ishaders.h"
#include "iselection.h"

class Winding;
class Patch;
class AABB;

namespace ui {

class TexTool :
	public SelectionSystem::Observer
{
	// The textool gtkwindow
	GtkWidget* _window;

	// The window position tracker
	gtkutil::WindowPosition _windowPosition;

	// GL widget
	GtkWidget* _glWidget;
	
	// The two vectors defining the visible area
	Vector3 _extents[2];
	
	// The shader we're working with (shared ptr)
	IShaderPtr _shader;
	
	Winding* _winding;
	Patch* _patch;
	
	// A reference to the SelectionInfo structure (with the counters)
	const SelectionInfo& _selectionInfo;
	
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
	
	/** greebo: SelectionSystem::Observer implementation. Gets called by
	 * the SelectionSystem upon selection change to allow updating.
	 */
	void selectionChanged();
	
	/** greebo: Updates the GL window
	 */
	void draw();
	
private:
	// Creates, packs and connects the child widgets
	void populateWindow();
	
	/** greebo: Calculates the extents the selection has in
	 * texture space.
	 * 
	 * @returns: the AABB with the z-component set to 0.
	 * 			 Can return an invalid AABB as well (if no selection was found). 
	 */
	AABB getExtents();
	
	/** greebo: Visualises the U/V coordinates by drawing the points
	 * into the "texture space".
	 */
	void drawUVCoords();
	
	/** greebo: Loads all the relevant data from the
	 * selectionsystem and prepares the member variables for drawing. 
	 */
	void update();

	// The callback for the delete event (toggles the visibility)
	static gboolean onDelete(GtkWidget* widget, GdkEvent* event, TexTool* self);
	static gboolean onExpose(GtkWidget* widget, GdkEventExpose* event, TexTool* self);
	static gboolean triggerRedraw(GtkWidget* widget, GdkEventFocus* event, TexTool* self);
	
}; // class TexTool

} // namespace ui

#endif /*TEXTOOL_H_*/
