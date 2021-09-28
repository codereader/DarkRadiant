#pragma once

#include "icommandsystem.h"
#include "itexturetoolmodel.h"

#include "wxutil/window/TransientWindow.h"
#include "math/Vector3.h"
#include "math/AABB.h"
#include "ishaders.h"
#include "imanipulator.h"
#include "iradiant.h"
#include "iorthoview.h"
#include "imousetool.h"
#include "iselection.h"
#include "iregistry.h"
#include <sigc++/connection.h>
#include <sigc++/trackable.h>
#include "wxutil/MouseToolHandler.h"
#include "wxutil/FreezePointer.h"
#include "tools/TextureToolMouseEvent.h"
#include "render/TextureToolView.h"
#include "messages/ManipulatorModeToggleRequest.h"
#include "messages/ComponentSelectionModeToggleRequest.h"
#include "messages/TextureChanged.h"
#include "messages/GridSnapRequest.h"

class Winding;
class Patch;

namespace wxutil { class GLWidget; }

namespace ui
{

class TexTool;
typedef std::shared_ptr<TexTool> TexToolPtr;

class TexTool : 
	public wxutil::TransientWindow,
    public IOrthoViewBase,
	public sigc::trackable,
    protected wxutil::MouseToolHandler
{
private:
	// GL widget
	wxutil::GLWidget* _glWidget;

    render::TextureToolView _view;

    wxutil::FreezePointer _freezePointer;

	// The shader we're working with
	MaterialPtr _shader;
	std::string _material;

	// The extents of the visible rectangle in texture space
	AABB _texSpaceAABB;

	// The dimensions of the GL widget in pixels.
	Vector2 _windowDims;

	// TRUE if the grid is active
	bool _gridActive;

	// For idle callbacks
	bool _selectionRescanNeeded;

	sigc::connection _sceneSelectionChanged;
	sigc::connection _undoHandler;
	sigc::connection _redoHandler;
	sigc::connection _manipulatorChanged;
	sigc::connection _selectionModeChanged;
	sigc::connection _selectionChanged;
	sigc::connection _gridChanged;
	std::size_t _manipulatorModeToggleRequestHandler;
	std::size_t _componentSelectionModeToggleRequestHandler;
	std::size_t _textureMessageHandler;
	std::size_t _gridSnapHandler;

    bool _determineThemeFromImage;

private:
	// This is where the static shared_ptr of the singleton instance is held.
	static TexToolPtr& InstancePtr();

	/* TransientWindow callbacks */
	virtual void _preHide();
	virtual void _preShow();

	void setGridActive(bool active);

	// Creates, packs and connects the child widgets
	void populateWindow();

	/** greebo: Calculates the extents of the entire scene selection in texture space.
	 *
	 * @returns: the bounds with the z-component set to 0.
	 * Returns an invalid AABB if no selection was found.
	 */
	AABB getUvBoundsFromSceneSelection();

	/** greebo: Returns the AABB of the currently visible texture space.
	 */
	const AABB& getVisibleTexSpace();

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

	bool onGLDraw();
	void onGLResize(wxSizeEvent& ev);

	// The callbacks for capturing the mouse events
	void onMouseUp(wxMouseEvent& ev);
	void onMouseDown(wxMouseEvent& ev);
	void onMouseMotion(wxMouseEvent& ev);
	void onMouseScroll(wxMouseEvent& ev);

	// UndoSystem event handler
	void onUndoRedoOperation();

    /**
     * greebo: Shutdown listeners de-registering from
     * the SelectionSystem, saving the window state, etc.
     */
    void onMainFrameShuttingDown();

    void handleManipulatorModeToggleRequest(selection::ManipulatorModeToggleRequest& request);
    void handleComponentSelectionModeToggleRequest(selection::ComponentSelectionModeToggleRequest& request);
    void handleGridSnapRequest(selection::GridSnapRequest& request);

    // Returns true if the texture tool window or the GL widget has focus
    bool textureToolHasFocus();

    void updateThemeButtons();
    void determineThemeBasedOnPixelData(const std::vector<unsigned char>& pixels);

public:
	TexTool();

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

	// Idle callback, used for deferred updates
	void onIdle(wxIdleEvent& ev);

	/** greebo: Static command targets
	 */
	static void toggle(const cmd::ArgumentList& args);

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
    void onManipulatorModeChanged(selection::IManipulator::Type type);
    void onSelectionModeChanged(textool::SelectionMode mode);

    TextureToolMouseEvent createMouseEvent(const Vector2& point, const Vector2& delta = Vector2(0, 0));

    void handleGLCapturedMouseMotion(const MouseToolPtr& tool, int x, int y, unsigned int mouseState);
};

} // namespace ui
