#pragma once

#include "icommandsystem.h"

#include "wxutil/window/TransientWindow.h"
#include "math/Vector3.h"
#include "math/AABB.h"
#include "ishaders.h"
#include "itexturetoolview.h"
#include "iradiant.h"
#include "imousetool.h"
#include "iselection.h"
#include "iregistry.h"
#include <sigc++/connection.h>
#include <sigc++/trackable.h>
#include "wxutil/MouseToolHandler.h"
#include "wxutil/FreezePointer.h"
#include "tools/TextureToolMouseEvent.h"
#include "render/View.h"

#include "TexToolItem.h"

class Winding;
class Patch;

namespace wxutil { class GLWidget; }

namespace ui
{

namespace
{
	const std::string RKEY_TEXTOOL_ROOT = "user/ui/textures/texTool/";
	const std::string RKEY_FACE_VERTEX_SCALE_PIVOT_IS_CENTROID = RKEY_TEXTOOL_ROOT + "faceVertexScalePivotIsCentroid";
}

class TexTool;
typedef std::shared_ptr<TexTool> TexToolPtr;

class TexTool : 
	public wxutil::TransientWindow,
    public ITextureToolView,
	public sigc::trackable,
    protected wxutil::MouseToolHandler
{
private:
	// GL widget
	wxutil::GLWidget* _glWidget;

    render::View _view;

    wxutil::FreezePointer _freezePointer;

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

	// The currently active objects in the textool window
	textool::TexToolItemVec _items;

	// The draggable selection rectangle
	textool::Rectangle _selectionRectangle;

	// The rectangle defining the manipulation's start and end point
	textool::Rectangle _manipulateRectangle;

	// TRUE if we are in selection mode
	bool _dragRectangle;

	// TRUE if a manipulation is currently ongoing
	bool _manipulatorMode;

	// The current grid size
	float _grid;

	// TRUE if the grid is active
	bool _gridActive;

	// For idle callbacks
	bool _updateNeeded;

	sigc::connection _selectionChanged;
	sigc::connection _undoHandler;
	sigc::connection _redoHandler;

private:
	// This is where the static shared_ptr of the singleton instance is held.
	static TexToolPtr& InstancePtr();

	/* TransientWindow callbacks */
	virtual void _preHide();
	virtual void _preShow();

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

	/** greebo: These two get called by the callback and handle the event
	 *
	 * @coords: this has already been converted into texture space.
	 */
	void doMouseUp(const Vector2& coords, wxMouseEvent& event);
	void doMouseDown(const Vector2& coords, wxMouseEvent& event);
	void doMouseMove(const Vector2& coords, wxMouseEvent& event);

	/** greebo: Converts the mouse/window coordinates into texture coords.
	 */
	Vector2 getTextureCoords(const double& x, const double& y);

	bool onGLDraw();
	void onGLResize(wxSizeEvent& ev);

	// The callbacks for capturing the mouse events
	void onMouseUp(wxMouseEvent& ev);
	void onMouseDown(wxMouseEvent& ev);
	void onMouseMotion(wxMouseEvent& ev);
	void onMouseScroll(wxMouseEvent& ev);

	// The keyboard callback to catch the ESC key
	void onKeyPress(wxKeyEvent& ev);

	// UndoSystem event handler
	void onUndoRedoOperation();

public:
	TexTool();

	/** 
	 * greebo: Shutdown listeners de-registering from 
	 * the SelectionSystem, saving the window state, etc.
	 */
	void onMainFrameShuttingDown();

	/** greebo: This is the static accessor method containing
	 * the static instance of the TexTool class. Use this to access
	 * the public member methods like toggle() and shutdown().
	 */
	static TexTool& Instance();

    int getWidth() const override;
    int getHeight() const override;

    void zoomIn() override;
    void zoomOut() override;

    SelectionTestPtr createSelectionTestForPoint(const Vector2& point) override;
    int getDeviceWidth() const override;
    int getDeviceHeight() const override;
    const VolumeTest& getVolumeTest() const override;

    // Request a deferred update of the UI elements
    void queueDraw() override;
    void forceRedraw() override;

    void scrollByPixels(int x, int y) override;

	/** greebo: Updates the GL window
	 */
	void draw();

	/** greebo: Increases/Decreases the grid size.
	 */
	void gridUp();
	void gridDown();

	// Idle callback, used for deferred updates
	void onIdle(wxIdleEvent& ev);

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

protected:
    MouseTool::Result processMouseDownEvent(const MouseToolPtr& tool, const Vector2& point) override;
    MouseTool::Result processMouseUpEvent(const MouseToolPtr& tool, const Vector2& point) override;
    MouseTool::Result processMouseMoveEvent(const MouseToolPtr& tool, int x, int y) override;

    void startCapture(const MouseToolPtr& tool) override;
    void endCapture() override;

    IInteractiveView& getInteractiveView() override;

private:
    void updateProjection();
    double getTextureAspectRatio();

    TextureToolMouseEvent createMouseEvent(const Vector2& point, const Vector2& delta = Vector2(0, 0));

    void handleGLCapturedMouseMotion(const MouseToolPtr& tool, int x, int y, unsigned int mouseState);
};

} // namespace ui
