#ifndef TEXTOOL_H_
#define TEXTOOL_H_

#include "gtk/gtkwidget.h"
#include "gtkutil/WindowPosition.h"
#include "math/Vector3.h"
#include "math/aabb.h"
#include "ishaders.h"
#include "iselection.h"
#include "iundo.h"

#include "TexToolItem.h"

class Winding;
class Patch;

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
	
	// The shader we're working with (shared ptr)
	IShaderPtr _shader;
	
	// A reference to the SelectionInfo structure (with the counters)
	const SelectionInfo& _selectionInfo;
	
	// The extents of the selected objects in texture space
	AABB _selAABB;
	
	// The extents of the visible rectangle in texture space
	AABB _texSpaceAABB;
	
	// The dimensions of the GL widget in pixels.
	Vector2 _windowDims;
	
	// The zoomfactor of this window (default = 1.1)
	float _zoomFactor;
	
	// The currently active objects in the textool window 
	selection::textool::TexToolItemVec _items;
	
	// The draggable selection rectangle
	selection::Rectangle _selectionRectangle;
	
	// The rectangle defining the manipulation's start and end point
	selection::Rectangle _manipulateRectangle;
	
	// The rectangle defining the moveOrigin operation's start and end point
	selection::Rectangle _moveOriginRectangle;
	
	// TRUE if we are in selection mode
	bool _dragRectangle;
	
	// TRUE if a manipulation is currently ongoing
	bool _manipulatorMode;
	
	// TRUE if the texspace center is being dragged around 
	bool _viewOriginMove;
	
	// The undocommand that gets created when a transformation begins
	// and destroyed upon end. The destruction triggers the UndoMementos being saved.  
	UndoableCommand* _undoCommand;
	
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
	 * @returns: the reference to the internal AABB with the z-component set to 0.
	 * 			 Can return an invalid AABB as well (if no selection was found). 
	 */
	AABB& getExtents();
	
	/** greebo: Returns the AABB of the currently visible texture space.
	 */
	AABB& getVisibleTexSpace();
	
	/** greebo: Recalculates and relocates the visible texture space.
	 * 			It's basically centered onto the current selection
	 * 			with the extents multiplied by the default zoomFactor.
	 */
	void recalculateVisibleTexSpace();
	
	/** greebo: Visualises the U/V coordinates by drawing the points
	 * into the "texture space".
	 */
	void drawUVCoords();
	
	/** greebo: Draws the grid into the visible texture space. 
	 */
	void drawGrid();
	
	/** greebo: Loads all the relevant data from the
	 * selectionsystem and prepares the member variables for drawing. 
	 */
	void update();
	
	/** greebo: Passes the given visitor to every Item in the hierarchy.
	 */
	void foreachItem(selection::textool::ItemVisitor& visitor);
	
	/** greebo: Returns a list of selectables for the given rectangle.
	 */
	selection::textool::TexToolItemVec getSelectables(const selection::Rectangle& rectangle);
	
	/** greebo: Returns a list of selectables for the given point.
	 * (A small rectangle is constructed to perform the selection test)
	 */
	selection::textool::TexToolItemVec getSelectables(const Vector2& coords);
	
	/** greebo: These two get called by the GTK callback and handle the event
	 * 
	 * @coords: this has already been converted into texture space. 
	 */
	void doMouseUp(const Vector2& coords, GdkEventButton* event);
	void doMouseDown(const Vector2& coords, GdkEventButton* event);
	void doMouseMove(const Vector2& coords, GdkEventMotion* event);

	/** greebo: Converts the mouse/window coordinates into texture coords.
	 */
	Vector2 getTextureCoords(const double& x, const double& y);

	// The callback for the delete event (toggles the visibility)
	static gboolean onDelete(GtkWidget* widget, GdkEvent* event, TexTool* self);
	static gboolean onExpose(GtkWidget* widget, GdkEventExpose* event, TexTool* self);
	static gboolean triggerRedraw(GtkWidget* widget, GdkEventFocus* event, TexTool* self);
	
	// The callbacks for capturing the mouse events
	static gboolean onMouseUp(GtkWidget* widget, GdkEventButton* event, TexTool* self);
	static gboolean onMouseDown(GtkWidget* widget, GdkEventButton* event, TexTool* self);
	static gboolean onMouseMotion(GtkWidget* widget, GdkEventMotion* event, TexTool* self);
	
	// The callback for mouse scroll events
	static gboolean onMouseScroll(GtkWidget* widget, GdkEventScroll* event, TexTool* self);
	
	// The static keyboard callback to catch the ESC key
	static gboolean onKeyPress(GtkWindow* window, GdkEventKey* event, TexTool* self);
}; // class TexTool

} // namespace ui

#endif /*TEXTOOL_H_*/
