#pragma once

#include <gtkmm/frame.h>

#include "math/Matrix4.h"
#include "../Timer.h"
#include "../GladeWidgetHolder.h"

#include "ifiltermenu.h"
#include "iscenegraph.h"
#include "irender.h"

#include "render/ShaderStateRenderer.h"
#include "render/SceneRenderWalker.h"
#include "render/NopVolumeTest.h"

namespace Gtk
{
    class ToolButton;
    class ToggleToolButton;
    class Toolbar;
    class HBox;
}

namespace gtkutil
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
    public Gtk::Frame,
    private GladeWidgetHolder
{
    // The scene we're rendering
    scene::GraphPtr _scene;

    // GL widget
    gtkutil::GLWidget* _glWidget;

private:
    void connectToolbarSignals();
    bool drawPreview();
    bool onGLMotion(GdkEventMotion*);
    bool onGLScroll(GdkEventScroll*);
    void onPause();
    void onStepForward();
    void onStepBack();
    void onSizeAllocate(Gtk::Allocation& allocation);
    void filtersChanged();

    void drawTime();

    // Called each frame by gtkutil::Timer
    static gboolean _onFrame(gpointer data);

protected:

    // The backend rendersystem instance
    RenderSystemPtr _renderSystem;

    // The front-end renderer, collecting the OpenGLRenderables
    render::ShaderStateRenderer _renderer;
    render::NopVolumeTest _volumeTest;

    // The scene adaptor passing nodes into our front-end renderer
    render::SceneRenderWalker _sceneWalker;

    // Current distance between camera and preview
    GLfloat _camDist;

    // Current rotation matrix
    Matrix4 _rotation;

    // Mutex flag to avoid draw call bunching
    bool _renderingInProgress;

    gtkutil::Timer _timer;

    int _previewWidth;
    int _previewHeight;

    // The filters menu
    ui::IFilterMenuPtr _filtersMenu;

protected:
    const scene::GraphPtr& getScene();

    /// Add another one to the toolbar hbox
    void addToolbar(Gtk::Toolbar& toolbar);

    /// Schedule a GL widget redraw operation
    void queueDraw();

    // Subclasses should at least add a single node as scene root, such that
    // the rendersystem can be associated. This is called after initialisePreview()
    virtual void setupSceneGraph();

    virtual Matrix4 getProjectionMatrix(float near_z, float far_z, float fieldOfView, int width, int height);
    virtual Matrix4 getModelViewMatrix();

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

    /**
     * \brief
     * Construct a RenderPreview
     *
     * \param enableAnimation
     * If true, display animation controls in toolbar, otherwise hide the
     * animation controls.
     */
    RenderPreview(bool enableAnimation = true);

    virtual ~RenderPreview();

public:
    void setSize(int width, int height);

    /**
     * Initialise the GL preview. This clears the window and sets up the
     * initial matrices and lights.
     */
    void initialisePreview();

    /// Get the RenderSystem used by the preview
    RenderSystemPtr getRenderSystem()
    {
        return _renderSystem;
    }
};
typedef boost::shared_ptr<RenderPreview> RenderPreviewPtr;

} // namespace
