#pragma once

#include "itexturetoolmodel.h"

#include "math/AABB.h"
#include "ishaders.h"
#include "imanipulator.h"
#include "iorthoview.h"
#include "imousetool.h"
#include <sigc++/connection.h>
#include <sigc++/trackable.h>
#include "wxutil/MouseToolHandler.h"
#include "wxutil/FreezePointer.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/event/SingleIdleCallback.h"
#include "tools/TextureToolMouseEvent.h"
#include "render/TextureToolView.h"
#include "messages/ManipulatorModeToggleRequest.h"
#include "messages/ComponentSelectionModeToggleRequest.h"
#include "messages/TextureChanged.h"
#include "messages/GridSnapRequest.h"

#include <wx/panel.h>

#include "messages/TextureToolRequest.h"
#include "wxutil/DockablePanel.h"

class Winding;
class Patch;

namespace wxutil { class GLWidget; }

namespace ui
{

class TexTool :
	public wxutil::DockablePanel,
    public IOrthoViewBase,
	public sigc::trackable,
    protected wxutil::MouseToolHandler,
    protected wxutil::XmlResourceBasedWidget,
    public wxutil::SingleIdleCallback
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
    bool _manipulatorPanelNeedsUpdate;
    bool _activeMaterialNeedsUpdate;

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
	std::size_t _texToolRequestHandler;

    bool _determineThemeFromImage;

private:
	void connectListeners();
	void disconnectListeners();

	void setGridActive(bool active);

	// Creates, packs and connects the child widgets
	void populateWindow();
	wxWindow* createManipulationPanel();

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
    void updateActiveMaterial();

	bool onGLDraw();
	void onGLResize(wxSizeEvent& ev);

	// The callbacks for capturing the mouse events
	void onMouseUp(wxMouseEvent& ev);
	void onMouseDown(wxMouseEvent& ev);
	void onMouseMotion(wxMouseEvent& ev);
	void onMouseScroll(wxMouseEvent& ev);

    void onShiftSelected(const std::string& direction);
    void onScaleSelected(const std::string& direction);
    void onRotateSelected(const std::string& direction);

	// UndoSystem event handler
	void onUndoRedoOperation();

    void handleManipulatorModeToggleRequest(selection::ManipulatorModeToggleRequest& request);
    void handleComponentSelectionModeToggleRequest(selection::ComponentSelectionModeToggleRequest& request);
    void handleGridSnapRequest(selection::GridSnapRequest& request);
    void handleTextureChanged(radiant::TextureChangedMessage& message);
    void handleTextureToolRequest(TextureToolRequest& request);

    // Returns true if the texture tool window or the GL widget has focus
    bool textureToolHasFocus();

    void updateThemeButtons();
    void determineThemeBasedOnPixelData(const std::vector<unsigned char>& pixels);

public:
	TexTool(wxWindow* parent);
	~TexTool() override;

    int getWidth() const override;
    int getHeight() const override;

    void zoomIn() override;
    void zoomOut() override;

    SelectionTestPtr createSelectionTestForPoint(const Vector2& point) override;
    int getDeviceWidth() const override;
    int getDeviceHeight() const override;
    const VolumeTest& getVolumeTest() const override;
    bool supportsDragSelections() override;

    // Request a deferred update of the UI elements
    void queueDraw() override;
    void forceRedraw() override;

    void scrollByPixels(int x, int y) override;

	/** greebo: Updates the GL window
	 */
	void draw();

	/** greebo: Registers the commands in the EventManager
	 */
	static void registerCommands();

protected:
	// Idle callback, used for deferred updates
	void onIdle() override;

    void onPanelActivated() override;
    void onPanelDeactivated() override;

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
    void onSelectionChanged();

    TextureToolMouseEvent createMouseEvent(const Vector2& point, const Vector2& delta = Vector2(0, 0));

    void handleGLCapturedMouseMotion(const MouseToolPtr& tool, int x, int y, unsigned int mouseState);

    void updateManipulationPanel();
};

} // namespace ui
