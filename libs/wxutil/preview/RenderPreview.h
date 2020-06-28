#pragma once

#include <wx/panel.h>
#include <wx/timer.h>
#include <sigc++/trackable.h>

#include "math/Matrix4.h"
#include "../XmlResourceBasedWidget.h"

#include "ifiltermenu.h"
#include "iscenegraph.h"
#include "irender.h"

#include "../FreezePointer.h"
#include "render/SimpleFrontendRenderer.h"
#include "render/SceneRenderWalker.h"
#include "render/NopVolumeTest.h"

class wxToolBarToolBase;

namespace wxutil
{

class GLWidget;

/**
 * greebo: This class acts as base for widgets featuring 
 * a real time openGL render preview. It offers
 * its own local SceneGraph, backend and frontend renderer
 * plus all the logic for camera handling and filtering.
 *
 * Override the protected methods to have the scene set up 
 * in special ways or add custom toolbar items.
 *
 * After construction the local scene graph will be empty.
 */
class RenderPreview :
    public wxEvtHandler,
    public sigc::trackable,
    private XmlResourceBasedWidget
{
private:
    void connectToolbarSignals();
    bool drawPreview();
    void onGLScroll(wxMouseEvent& ev);
	void onGLMouseClick(wxMouseEvent& ev);
    void onGLMouseRelease(wxMouseEvent& ev);
    void onGLMotion(wxMouseEvent& ev);
    void onGLMotionDelta(int x, int y, unsigned int mouseState);
    void onGLKeyPress(wxKeyEvent& ev);

	void onStartPlaybackClick(wxCommandEvent& ev);
	void onStopPlaybackClick(wxCommandEvent& ev);
	void onPausePlaybackClick(wxCommandEvent& ev);
    void onStepForwardClick(wxCommandEvent& ev);
    void onStepBackClick(wxCommandEvent& ev);

    void onSizeAllocate(wxSizeEvent& ev);
    void onFilterConfigChanged();
    void onRenderModeChanged(wxCommandEvent& ev);
	void onGridButtonClick(wxCommandEvent& ev);

    void drawTime();
	void drawGrid();

    // Called each frame by wxTimer
    void _onFrame(wxTimerEvent& ev);

    void updateModelViewMatrix();
    void updateActiveRenderModeButton();

    void setupToolbars(bool enableAnimation);

protected:
	wxPanel* _mainPanel;

private:
    // The scene we're rendering
    scene::GraphPtr _scene;

    // GL widget
    GLWidget* _glWidget;

    bool _initialised;

    FreezePointer _freezePointer;

	bool _renderGrid;

protected:
    // The backend rendersystem instance
    RenderSystemPtr _renderSystem;

    // The front-end renderer, collecting the OpenGLRenderables
    render::SimpleFrontendRenderer _renderer;
    render::NopVolumeTest _volumeTest;

    // The scene adaptor passing nodes into our front-end renderer
    render::SceneRenderWalker _sceneWalker;

    // Current viewer position and view angles
    Vector3 _viewOrigin;
    Vector3 _viewAngles;

    // Current modelview matrix
    Matrix4 _modelView;

    // The local model orientation
    Matrix4 _modelRotation;

    int _lastX;
    int _lastY;

    // Mutex flag to avoid draw call bunching
    bool _renderingInProgress;

    wxTimer _timer;

    int _previewWidth;
    int _previewHeight;

	wxSizer* _toolbarSizer;

    // The filters menu
    ui::IFilterMenuPtr _filtersMenu;
	wxToolBarToolBase* _filterTool;

protected:
    const scene::GraphPtr& getScene();

    /// Add another one to the toolbar hbox
    void addToolbar(wxToolBar* toolbar);

    // Subclasses should at least add a single node as scene root, such that
    // the rendersystem can be associated. This is called after initialisePreview()
    virtual void setupSceneGraph();

    virtual Matrix4 getProjectionMatrix(float near_z, float far_z, float fieldOfView, int width, int height);
    virtual const Matrix4& getModelViewMatrix();

    virtual Matrix4 calculateModelViewMatrix();

    void resetModelRotation();

    // When the user requests a model rotation change (by dragging)
    virtual void onModelRotationChanged() {}

    virtual void startPlayback();
    virtual void stopPlayback();

    // Override this to deliver accurate scene bounds, used for mousewheel-zooming
    virtual AABB getSceneBounds();

    // Called right before rendering, returning false will cancel the render algorithm
    virtual bool onPreRender();

    // Called after the render phase, can be used to draw custom stuff on the GL widget
    virtual void onPostRender() {}

    // Use this to render a wireframe view of the scene
    void renderWireFrame();

    // Override these to define the flags to render a fill/wireframe scene
    virtual RenderStateFlags getRenderFlagsFill();
    virtual RenderStateFlags getRenderFlagsWireframe();

    void associateRenderSystem();

    // Can be overridden by subclasses to update their scene/models
    virtual void onRenderModeChanged() {}

    /**
     * \brief
     * Construct a RenderPreview
     *
     * \param enableAnimation
     * If true, display animation controls in toolbar, otherwise hide the
     * animation controls.
     */
    RenderPreview(wxWindow* parent, bool enableAnimation = true);

    virtual ~RenderPreview();

public:
	wxPanel* getWidget() const
	{
		return _mainPanel;
	}

    void setSize(int width, int height);

    /**
     * Initialise the GL preview. This clears the window and sets up the
     * initial matrices and lights.
     */
    void initialisePreview();

    /// Get the RenderSystem used by the preview
    const RenderSystemPtr& getRenderSystem()
    {
        return _renderSystem;
    }

    // Defines the position of the camera
    void setViewOrigin(const Vector3& origin);

    // Defines the view angles (euler angles in degrees)
    void setViewAngles(const Vector3& angles);

    // Check whether lighting mode is enabled
    bool getLightingModeEnabled();

    // Enable/disable lighting mode
    void setLightingModeEnabled(bool enabled);

	/// Schedule a GL widget redraw operation
    void queueDraw();
};
typedef std::shared_ptr<RenderPreview> RenderPreviewPtr;

} // namespace
