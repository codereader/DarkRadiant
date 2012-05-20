#ifndef TEXTOOL_H_
#define TEXTOOL_H_

#include "icommandsystem.h"
#include "gtkutil/GLWidget.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/event/SingleIdleCallback.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "math/Vector3.h"
#include "math/AABB.h"
#include "ishaders.h"
#include "iradiant.h"
#include "iselection.h"
#include "iregistry.h"

#include "TexToolItem.h"

class Winding;
class Patch;

namespace ui
{

	namespace
	{
		const std::string RKEY_TEXTOOL_ROOT = "user/ui/textures/texTool/";
		const std::string RKEY_FACE_VERTEX_SCALE_PIVOT_IS_CENTROID = RKEY_TEXTOOL_ROOT + "faceVertexScalePivotIsCentroid";
	}

class TexTool;
typedef boost::shared_ptr<TexTool> TexToolPtr;

class TexTool
: public gtkutil::PersistentTransientWindow,
  public SelectionSystem::Observer,
  public gtkutil::SingleIdleCallback
{
	// The window position tracker
	gtkutil::WindowPosition _windowPosition;

	// GL widget
	gtkutil::GLWidget* _glWidget;

	// The shader we're working with (shared ptr)
	MaterialPtr _shader;

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
	textool::TexToolItemVec _items;

	// The draggable selection rectangle
	textool::Rectangle _selectionRectangle;

	// The rectangle defining the manipulation's start and end point
	textool::Rectangle _manipulateRectangle;

	// The rectangle defining the moveOrigin operation's start and end point
	textool::Rectangle _moveOriginRectangle;

	// TRUE if we are in selection mode
	bool _dragRectangle;

	// TRUE if a manipulation is currently ongoing
	bool _manipulatorMode;

	// TRUE if the texspace center is being dragged around
	bool _viewOriginMove;

	// The current grid size
	float _grid;

	// TRUE if the grid is active
	bool _gridActive;

private:
	// This is where the static shared_ptr of the singleton instance is held.
	static TexToolPtr& InstancePtr();

	/* TransientWindow callbacks */
	virtual void _preHide();
	virtual void _preShow();
	virtual void _postShow();

	void setGridActive(bool active);

	/** greebo: Selects all items that are related / connected
	 * 			to the currently selected ones. E.g. if one patch
	 * 			vertex is selected, all the vertices of the patch
	 * 			get selected as well.
	 */
	void selectRelatedItems();

	/** greebo: Helper methods that start/end an undoable operation.
	 */
	void beginOperation();
	void endOperation(const std::string& commandName);

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
	void foreachItem(textool::ItemVisitor& visitor);

	/** greebo: Returns the number of selected TexToolItems.
	 */
	int countSelected();

	/** greebo: Sets all selectables to <selected>
	 * 			If selected == false and no items are selected,
	 * 			the call is propagated to the main window to
	 * 			trigger a scene deselection.
	 */
	bool setAllSelected(bool selected);

	/** greebo: Returns a list of selectables for the given rectangle.
	 */
	textool::TexToolItemVec getSelectables(const textool::Rectangle& rectangle);

	/** greebo: Returns a list of selectables for the given point.
	 * (A small rectangle is constructed to perform the selection test)
	 */
	textool::TexToolItemVec getSelectables(const Vector2& coords);

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

	bool onExpose(GdkEventExpose* ev);
	bool triggerRedraw(GdkEventFocus* ev);
	void onSizeAllocate(Gtk::Allocation& allocation);

	// The callbacks for capturing the mouse events
	bool onMouseUp(GdkEventButton* ev);
	bool onMouseDown(GdkEventButton* ev);
	bool onMouseMotion(GdkEventMotion* ev);
	bool onMouseScroll(GdkEventScroll* ev);

	// The keyboard callback to catch the ESC key
	bool onKeyPress(GdkEventKey* ev);

public:
	TexTool();

	/** greebo: Some sort of "soft" destructor that de-registers
	 * this class from the SelectionSystem, saves the window state, etc.
	 */
	void onRadiantShutdown();

	/** greebo: This is the static accessor method containing
	 * the static instance of the TexTool class. Use this to access
	 * the public member methods like toggle() and shutdown().
	 */
	static TexTool& Instance();

	/** greebo: SelectionSystem::Observer implementation. Gets called by
	 * the SelectionSystem upon selection change to allow updating.
	 */
	void selectionChanged(const scene::INodePtr& node, bool isComponent);

	/** greebo: Updates the GL window
	 */
	void draw();

	// Request a deferred update of the UI elements (is performed when GTK is idle)
	void queueUpdate();

	/** greebo: Increases/Decreases the grid size.
	 */
	void gridUp();
	void gridDown();

	// Idle callback, used for deferred updates
	void onGtkIdle();

	/** greebo: Snaps the current TexTool selection to the active grid.
	 */
	void snapToGrid();

	/** greebo: Merges the selected items in terms of their UV coordinates.
	 * 			Mainly useful for stitching patch control vertices.
	 */
	void mergeSelectedItems();

	/** greebo: This flips the selection in texture space. This is the equivalent
	 * 			of the according buttons in the Surface Inspector, but it only affects
	 * 			the items selected in the TexTool.
	 *
	 * @axis: 0 = s-Axis flip, 1 = t-axis flip
	 */
	void flipSelected(int axis);

	/** greebo: Static command targets
	 */
	static void toggle(const cmd::ArgumentList& args);
	static void texToolGridUp(const cmd::ArgumentList& args);
	static void texToolGridDown(const cmd::ArgumentList& args);
	static void texToolSnapToGrid(const cmd::ArgumentList& args);
	static void texToolMergeItems(const cmd::ArgumentList& args);
	static void texToolFlipS(const cmd::ArgumentList& args);
	static void texToolFlipT(const cmd::ArgumentList& args);
	static void selectRelated(const cmd::ArgumentList& args);

	/** greebo: Registers the commands in the EventManager
	 */
	static void registerCommands();

}; // class TexTool

} // namespace ui

#endif /*TEXTOOL_H_*/
