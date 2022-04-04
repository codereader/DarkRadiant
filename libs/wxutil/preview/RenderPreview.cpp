#include "RenderPreview.h"

#include "ifilter.h"
#include "i18n.h"
#include "icameraview.h"
#include "iscenegraphfactory.h"
#include "irendersystemfactory.h"

#include "math/AABB.h"
#include "util/ScopedBoolLock.h"
#include "registry/registry.h"
#include "render/CamRenderer.h"
#include "render/CameraView.h"
#include "render/SceneRenderWalker.h"
#include "wxutil/menu/FilterPopupMenu.h"

#include "../GLWidget.h"
#include <wx/sizer.h>
#include <wx/menu.h>
#include <wx/dcclient.h>
#include <wx/textctrl.h>
#include "../Bitmap.h"

#include <fmt/format.h>
#include <functional>

namespace wxutil
{

namespace
{
    const GLfloat PREVIEW_FOV = 60;

    // Widget names
    const std::string BOTTOM_BOX("bottomBox");
    const std::string PAUSE_BUTTON("pauseButton");
    const std::string STOP_BUTTON("stopButton");

	const std::string RKEY_RENDERPREVIEW_SHOWGRID("user/ui/renderPreview/showGrid");
	const std::string RKEY_RENDERPREVIEW_FONTSIZE("user/ui/renderPreview/fontSize");
	const std::string RKEY_RENDERPREVIEW_FONTSTYLE("user/ui/renderPreview/fontStyle");
}

RenderPreview::RenderPreview(wxWindow* parent, bool enableAnimation) :
    _mainPanel(loadNamedPanel(parent, "RenderPreviewPanel")),
	_glWidget(new wxutil::GLWidget(_mainPanel, std::bind(&RenderPreview::drawPreview, this), "RenderPreview")),
    _initialised(false),
	_renderGrid(registry::getValue<bool>(RKEY_RENDERPREVIEW_SHOWGRID)),
    _renderSystem(GlobalRenderSystemFactory().createRenderSystem()),
    _viewOrigin(0, 0, 0),
    _viewAngles(0, 0, 0),
    _modelView(Matrix4::getIdentity()),
    _modelRotation(Matrix4::getIdentity()),
    _lastX(0),
    _lastY(0),
    _renderingInProgress(false),
    _timer(this),
    _previewWidth(0),
    _previewHeight(0),
	_filterTool(nullptr)
{
	Bind(wxEVT_TIMER, &RenderPreview::_onFrame, this);

    // Insert GL widget
	_mainPanel->GetSizer()->Prepend(_glWidget, 1, wxEXPAND);

	_glWidget->Connect(wxEVT_SIZE, wxSizeEventHandler(RenderPreview::onSizeAllocate), NULL, this);
	_glWidget->Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(RenderPreview::onGLScroll), NULL, this);
    _glWidget->Connect(wxEVT_MOTION, wxMouseEventHandler(RenderPreview::onGLMotion), NULL, this);
	_glWidget->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(RenderPreview::onGLMouseClick), NULL, this);
    _glWidget->Connect(wxEVT_LEFT_DCLICK, wxMouseEventHandler(RenderPreview::onGLMouseClick), NULL, this);
    _glWidget->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(RenderPreview::onGLMouseClick), NULL, this);
    _glWidget->Connect(wxEVT_RIGHT_DCLICK, wxMouseEventHandler(RenderPreview::onGLMouseClick), NULL, this);
    _glWidget->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(RenderPreview::onGLKeyPress), NULL, this);

    setupToolbars(enableAnimation);

    // Clicks are eaten when the FreezePointer is active, request to receive them
    _freezePointer.connectMouseEvents(
        std::bind(&RenderPreview::onGLMouseClick, this, std::placeholders::_1),
        std::bind(&RenderPreview::onGLMouseRelease, this, std::placeholders::_1));
}

void RenderPreview::setupToolbars(bool enableAnimation)
{
    wxToolBar* toolbar = findNamedObject<wxToolBar>(_mainPanel, "RenderPreviewAnimToolbar");

    _toolbarSizer = toolbar->GetContainingSizer();

    // Set up the toolbar
    if (enableAnimation)
    {
        connectToolbarSignals();
    }
    else
    {
        toolbar->Hide();
    }

    // Connect filters menu to toolbar
    wxToolBar* filterToolbar = findNamedObject<wxToolBar>(_mainPanel, "RenderPreviewFilterToolbar");

    wxToolBarToolBase* filterTool = filterToolbar->AddTool(wxID_ANY, _("Filters"),
                                                           wxutil::GetLocalBitmap("iconFilter16.png"),
                                                           _("Filters"), wxITEM_DROPDOWN);

	// By setting a drodown menu the toolitem will take ownership and delete the menu on destruction
    filterToolbar->SetDropdownMenu(filterTool->GetId(), new wxutil::FilterPopupMenu());

    filterToolbar->Realize();

    // Get notified of filter changes
    GlobalFilterSystem().filterConfigChangedSignal().connect(
        sigc::mem_fun(this, &RenderPreview::onFilterConfigChanged)
    );

    wxToolBar* renderToolbar = findNamedObject<wxToolBar>(_mainPanel, "RenderPreviewRenderModeToolbar");

    renderToolbar->Bind(wxEVT_TOOL, &RenderPreview::onRenderModeChanged, this,
        getToolBarToolByLabel(renderToolbar, "texturedModeButton")->GetId());
    renderToolbar->Bind(wxEVT_TOOL, &RenderPreview::onRenderModeChanged, this,
        getToolBarToolByLabel(renderToolbar, "lightingModeButton")->GetId());

    updateActiveRenderModeButton();

	wxToolBar* utilToolbar = findNamedObject<wxToolBar>(_mainPanel, "RenderPreviewUtilToolbar");

	utilToolbar->Bind(wxEVT_TOOL, &RenderPreview::onGridButtonClick, this,
        getToolBarToolByLabel(utilToolbar, "gridButton")->GetId());

	utilToolbar->ToggleTool(getToolBarToolByLabel(utilToolbar, "gridButton")->GetId(), _renderGrid);
}

void RenderPreview::connectToolbarSignals()
{
	wxToolBar* toolbar = findNamedObject<wxToolBar>(_mainPanel, "RenderPreviewAnimToolbar");

	toolbar->Bind(wxEVT_TOOL, &RenderPreview::onStartPlaybackClick, this, getToolBarToolByLabel(toolbar, "startTimeButton")->GetId());
	toolbar->Bind(wxEVT_TOOL, &RenderPreview::onPausePlaybackClick, this, getToolBarToolByLabel(toolbar, "pauseTimeButton")->GetId());
	toolbar->Bind(wxEVT_TOOL, &RenderPreview::onStopPlaybackClick, this, getToolBarToolByLabel(toolbar, "stopTimeButton")->GetId());

	toolbar->Bind(wxEVT_TOOL, &RenderPreview::onStepBackClick, this, getToolBarToolByLabel(toolbar, "prevButton")->GetId());
	toolbar->Bind(wxEVT_TOOL, &RenderPreview::onStepForwardClick, this, getToolBarToolByLabel(toolbar, "nextButton")->GetId());

    // Connect the frame selector
    auto frameSelector = getToolBarControlByName(toolbar, "FrameSelector")->GetControl();
    frameSelector->SetWindowStyleFlag(wxTE_PROCESS_ENTER);
    frameSelector->Bind(wxEVT_SPINCTRL, &RenderPreview::onFrameSelected, this);
    frameSelector->Bind(wxEVT_TEXT_ENTER, &RenderPreview::onFrameConfirmed, this);
}

RenderPreview::~RenderPreview()
{
    _scene.reset();
    _renderSystem.reset();
	_timer.Stop();
}

void RenderPreview::updateActiveRenderModeButton()
{
    wxToolBar* toolbar = static_cast<wxToolBar*>(_mainPanel->FindWindow("RenderPreviewRenderModeToolbar"));

    if (getLightingModeEnabled())
    {
        toolbar->ToggleTool(getToolBarToolByLabel(toolbar, "lightingModeButton")->GetId(), true);
    }
    else
    {
        toolbar->ToggleTool(getToolBarToolByLabel(toolbar, "texturedModeButton")->GetId(), true);
    }
}

void RenderPreview::onFilterConfigChanged()
{
    if (!getScene()->root()) return;

    GlobalFilterSystem().updateSubgraph(getScene()->root());
    queueDraw();
}

void RenderPreview::addToolbar(wxToolBar* toolbar)
{
	_toolbarSizer->Add(toolbar, 0, wxEXPAND);
}

void RenderPreview::queueDraw()
{
    if (!_renderingInProgress)
    {
        _glWidget->Refresh();
    }
}

void RenderPreview::setSize(int width, int height)
{
	_glWidget->SetClientSize(width, height);
}

void RenderPreview::initialisePreview()
{
    _initialised = true;

    // Set up the lights
    glEnable(GL_LIGHTING);

    glEnable(GL_LIGHT0);
    GLfloat l0Amb[] = { 0.3f, 0.3f, 0.3f, 1.0f };
    GLfloat l0Dif[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat l0Pos[] = { 1.0f, 1.0f, 1.0f, 0.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, l0Amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, l0Dif);
    glLightfv(GL_LIGHT0, GL_POSITION, l0Pos);

    glEnable(GL_LIGHT1);
    GLfloat l1Dif[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat l1Pos[] = { 0.0, 0.0, 1.0, 0.0 };
    glLightfv(GL_LIGHT1, GL_DIFFUSE, l1Dif);
    glLightfv(GL_LIGHT1, GL_POSITION, l1Pos);

    if (_renderSystem->shaderProgramsAvailable())
    {
        setLightingModeEnabled(false);
    }

    // In case it didn't happen until now, make sure the rendersystem is realised
    _renderSystem->realise();

    updateModelViewMatrix();
}

void RenderPreview::setViewOrigin(const Vector3& origin)
{
    _viewOrigin = origin;

    updateModelViewMatrix();
}

void RenderPreview::setViewAngles(const Vector3& angles)
{
    _viewAngles = angles;

    updateModelViewMatrix();
}

bool RenderPreview::getLightingModeEnabled()
{
    return _renderSystem->getCurrentShaderProgram() == RenderSystem::SHADER_PROGRAM_INTERACTION;
}

void RenderPreview::setLightingModeEnabled(bool enabled)
{
    if (enabled && !getLightingModeEnabled())
    {
        _renderSystem->setShaderProgram(RenderSystem::SHADER_PROGRAM_INTERACTION);
        queueDraw();
    }
    else if (!enabled && getLightingModeEnabled())
    {
        _renderSystem->setShaderProgram(RenderSystem::SHADER_PROGRAM_NONE);
        queueDraw();
    }

    // Synchronise the button state, if necessary
    auto* toolbar = static_cast<wxToolBar*>(_mainPanel->FindWindow("RenderPreviewRenderModeToolbar"));
    auto* textureButton = getToolBarToolByLabel(toolbar, "texturedModeButton");
    auto* lightingButton = getToolBarToolByLabel(toolbar, "lightingModeButton");

    if (!enabled && !textureButton->IsToggled())
    {
        toolbar->ToggleTool(textureButton->GetId(), true);
    }
    else if (enabled && !lightingButton->IsToggled())
    {
        toolbar->ToggleTool(lightingButton->GetId(), true);
    }
}

const scene::GraphPtr& RenderPreview::getScene()
{
    if (!_scene)
    {
        _scene = GlobalSceneGraphFactory().createSceneGraph();

        setupSceneGraph();

        associateRenderSystem();
    }

    return _scene;
}

void RenderPreview::setupSceneGraph()
{
    // Set our render time to 0
    _renderSystem->setTime(0);
}

void RenderPreview::associateRenderSystem()
{
    if (_scene && _scene->root())
    {
        _scene->root()->setRenderSystem(_renderSystem);
    }
}

const Matrix4& RenderPreview::getModelViewMatrix()
{
    return _modelView;
}

Matrix4 RenderPreview::calculateModelViewMatrix()
{
    static const Matrix4 RADIANT2OPENGL = Matrix4::byColumns(
        0, -1, 0, 0,
        0,  0, 1, 0,
       -1,  0, 0, 0,
        0,  0, 0, 1
    );

    // greebo: This is modeled after the camera's modelview code
    // Some notes: first the matrix stack is built in the reverse order,
    // then a final call to inverse() is made to bring them in the correct order

    // In the final form the matrix stack does this:
    // 0. We start with the view at the 0,0,0 origin, facing down the negative z axis
    // 1. Move the view point away from the model (translate by -viewOrigin)
    // 2. Rotate the view using Euler angles (rotating about two specific axes)
    // 3. Apply the radiant2openGL matrix which basically rotates the model 90 degrees
    //    around two axes. This is needed to resemble the view of a human (who - with
    //    pitch, yaw and roll angles equal to zero - looks parallel to the xy plane,
    //    and not down the negative z axis like openGL would.

    // Translate the model by viewOrigin (with the later inverse() call this will
    // be the first applied translation by -viewOrigin)
    Matrix4 modelview = Matrix4::getTranslation(_viewOrigin);

    // Rotate the view like a human would turn their head. Due to the radiant2openGL transform
    // the axes are used differently. Pitch is rotating around y instead of x, for example.
    Vector3 radiant_eulerXYZ(0, _viewAngles[camera::CAMERA_PITCH], -_viewAngles[camera::CAMERA_YAW]);
    modelview.rotateByEulerXYZDegrees(radiant_eulerXYZ);

    // As last step apply the radiant2openGL transform which rotates first around z, then around y
    modelview.multiplyBy(RADIANT2OPENGL);

    // To get the translation transform applied first, inverse the whole stack
    modelview.invert();

    return modelview;
}

void RenderPreview::updateModelViewMatrix()
{
    _modelView = calculateModelViewMatrix();
}

void RenderPreview::startPlayback()
{
	if (_timer.IsRunning())
    {
        // Timer is already running, just reset the preview time
        _renderSystem->setTime(0);
    }
    else
    {
        // Timer is not enabled, we're paused or stopped
        _timer.Start(MSEC_PER_FRAME);
    }

	wxToolBar* toolbar = findNamedObject<wxToolBar>(_mainPanel, "RenderPreviewAnimToolbar");

	toolbar->EnableTool(getToolBarToolByLabel(toolbar, "pauseTimeButton")->GetId(), true);
	toolbar->EnableTool(getToolBarToolByLabel(toolbar, "stopTimeButton")->GetId(), true);

    updateFrameSelector();
}

void RenderPreview::stopPlayback()
{
    _renderSystem->setTime(0);
    _timer.Stop();

	wxToolBar* toolbar = findNamedObject<wxToolBar>(_mainPanel, "RenderPreviewAnimToolbar");

	toolbar->EnableTool(getToolBarToolByLabel(toolbar, "pauseTimeButton")->GetId(), false);
	toolbar->EnableTool(getToolBarToolByLabel(toolbar, "stopTimeButton")->GetId(), false);

    updateFrameSelector();

    queueDraw();
}

bool RenderPreview::onPreRender()
{
    return true;
}

RenderStateFlags RenderPreview::getRenderFlagsFill()
{
    return  RENDER_MASKCOLOUR |
            RENDER_ALPHATEST |
            RENDER_BLEND |
            RENDER_CULLFACE |
            RENDER_OFFSETLINE |
            RENDER_VERTEX_COLOUR |
            RENDER_FILL |
            RENDER_LIGHTING |
            RENDER_TEXTURE_2D |
            RENDER_SMOOTH |
            RENDER_SCALED |
            RENDER_FILL |
            RENDER_TEXTURE_CUBEMAP |
            RENDER_BUMP |
            RENDER_PROGRAM;
}

RenderStateFlags RenderPreview::getRenderFlagsWireframe()
{
    return RENDER_MASKCOLOUR |
           RENDER_ALPHATEST |
           RENDER_BLEND |
           RENDER_CULLFACE |
           RENDER_OFFSETLINE |
           RENDER_VERTEX_COLOUR |
           RENDER_LIGHTING |
           RENDER_SMOOTH |
           RENDER_SCALED |
           RENDER_TEXTURE_CUBEMAP |
           RENDER_BUMP |
           RENDER_PROGRAM;
}

bool RenderPreview::drawPreview()
{
    if (_renderingInProgress) return false; // avoid double-entering this method

    if (!_initialised)
    {
        initialisePreview();

        // Since we shouldn't call the virtual canDrawGrid() in the constructor
        // adjust the tool bar here.
        if (!canDrawGrid())
        {
            auto* utilToolbar = findNamedObject<wxToolBar>(_mainPanel, "RenderPreviewUtilToolbar");
            utilToolbar->DeleteTool(getToolBarToolByLabel(utilToolbar, "gridButton")->GetId());
        }
    }

    util::ScopedBoolLock lock(_renderingInProgress);

    _renderSystem->startFrame();

    if (!_glFont)
    {
        auto fontSize = registry::getValue<int>(RKEY_RENDERPREVIEW_FONTSIZE);
        auto fontStyle = registry::getValue<std::string>(RKEY_RENDERPREVIEW_FONTSTYLE) == "Sans" ?
            IGLFont::Style::Sans : IGLFont::Style::Mono;
        _glFont = GlobalOpenGL().getFont(fontStyle, fontSize);
    }

    glViewport(0, 0, _previewWidth, _previewHeight);

    // Set up the render and clear the drawing area in any case
    glDepthMask(GL_TRUE);

    if (getLightingModeEnabled())
    {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    }
    else
    {
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Pre-Render event
	if (!onPreRender())
	{
		// a return value of false means to cancel rendering
		drawInfoText();
		return true; // swap buffers
	}

    // Set up the camera
    Matrix4 projection = camera::calculateProjectionMatrix(0.1f, 10000, PREVIEW_FOV, _previewWidth, _previewHeight);

    // Keep the modelview matrix in the volumetest class up to date
    _view.construct(projection, getModelViewMatrix(), _previewWidth, _previewHeight);
    _view.setViewer(_viewOrigin);

	// Set the projection and modelview matrices
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixd(projection);

	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixd(_view.GetModelview());

	// Front-end render phase, collect OpenGLRenderable objects from the scene
    render::CamRenderer renderer(_view, _shaders);
    render::SceneRenderWalker sceneWalker(renderer, _view);
    getScene()->foreachVisibleNodeInVolume(_view, sceneWalker);

    RenderStateFlags flags = getRenderFlagsFill();

    // Start back end render phase
    if (getLightingModeEnabled())
    {
        _renderSystem->renderLitScene(flags, _view);
    }
    else
    {
        _renderSystem->renderFullBrightScene(RenderViewType::Camera, flags, _view);
    }

    // Grid will be drawn afterwards, with enabled depth test
    if (_renderGrid && canDrawGrid())
    {
        drawGrid();
    }

    // Give subclasses an opportunity to render their own on-screen stuff
    onPostRender();

    // Draw the render time
    drawInfoText();

    _renderSystem->endFrame();

    return true;
}

void RenderPreview::renderWireFrame()
{
    RenderStateFlags flags = getRenderFlagsWireframe();

    // Set up the camera
    Matrix4 projection = camera::calculateProjectionMatrix(0.1f, 10000, PREVIEW_FOV, _previewWidth, _previewHeight);

    // Front-end render phase, collect OpenGLRenderable objects from the scene
    render::CamRenderer renderer(_view, _shaders);
    render::SceneRenderWalker sceneWalker(renderer, _view);
    getScene()->foreachVisibleNodeInVolume(_view, sceneWalker);

    // Launch the back end rendering
    _renderSystem->renderFullBrightScene(RenderViewType::Camera, flags, _view);
}

void RenderPreview::onGLMouseClick(wxMouseEvent& ev)
{
    _lastX = ev.GetX();
    _lastY = ev.GetY();

    if (ev.RightDown())
    {
        if (_freezePointer.isCapturing(_glWidget))
        {
            _freezePointer.endCapture();
            return;
        }

        // Key event processing requires focus
        _glWidget->SetFocus();

        _freezePointer.startCapture(_glWidget,
            [&](int x, int y, int mouseState) { onGLMotionDelta(x, y, mouseState); },
            [&]() {}); // capture is released by FreezePointer
    }
}

void RenderPreview::onGLMouseRelease(wxMouseEvent& ev)
{
}

void RenderPreview::onGLMotion(wxMouseEvent& ev)
{
    if (ev.LeftIsDown()) // dragging with mouse button
    {
        // Calculate the mouse delta as a vector in the XY plane, and store the
        // current position for the next event.
        Vector3 deltaPos(ev.GetX() - _lastX, _lastY - ev.GetY(), 0);

        _lastX = ev.GetX();
        _lastY = ev.GetY();

        // Get the rotation axes in model space
        Vector3 localXRotAxis = _modelView.getInverse().transformDirection(Vector3(1, 0, 0));
        Vector3 localZRotAxis = Vector3(0, 0, 1);

        if (deltaPos.y() != 0)
        {
            _modelRotation.premultiplyBy(Matrix4::getRotation(localXRotAxis, degrees_to_radians(deltaPos.y())));
        }

        if (deltaPos.x() != 0)
        {
            _modelRotation.premultiplyBy(Matrix4::getRotation(localZRotAxis, degrees_to_radians(-deltaPos.x())));
        }

        // Notify the subclasses to do something with this matrix
        onModelRotationChanged();

        queueDraw();
    }
}

void RenderPreview::onGLMotionDelta(int x, int y, unsigned int mouseState)
{
    const float dtime = 0.1f;
    const float angleSpeed = 3; // camerasettings::anglespeed

    _viewAngles[camera::CAMERA_PITCH] += y * dtime * angleSpeed;
    _viewAngles[camera::CAMERA_YAW] += x * dtime * angleSpeed;

    if (_viewAngles[camera::CAMERA_PITCH] > 90)
        _viewAngles[camera::CAMERA_PITCH] = 90;
    else if (_viewAngles[camera::CAMERA_PITCH] < -90)
        _viewAngles[camera::CAMERA_PITCH] = -90;

    if (_viewAngles[camera::CAMERA_YAW] >= 360)
        _viewAngles[camera::CAMERA_YAW] -= 360;
    else if (_viewAngles[camera::CAMERA_YAW] <= 0)
        _viewAngles[camera::CAMERA_YAW] += 360;

    updateModelViewMatrix();

    queueDraw();
}

AABB RenderPreview::getSceneBounds()
{
    return AABB(Vector3(0,0,0), Vector3(64,64,64));
}

Vector3 RenderPreview::getGridOrigin()
{
    return getSceneBounds().getOrigin();
}

void RenderPreview::resetModelRotation()
{
    _modelRotation = Matrix4::getIdentity();

    onModelRotationChanged();
}

void RenderPreview::onGLScroll(wxMouseEvent& ev)
{
    // Scroll increment is a fraction of the AABB radius
    float inc = static_cast<float>(getSceneBounds().getRadius()) * 0.3f;

    Vector3 forward(_modelView[2], _modelView[6], _modelView[10]);

	if (ev.GetWheelRotation() > 0)
    {
        _viewOrigin -= forward * inc;
    }
    else if (ev.GetWheelRotation() < 0)
    {
        _viewOrigin += forward * inc;
    }

    updateModelViewMatrix();

    queueDraw();
}

void RenderPreview::onRenderModeChanged(wxCommandEvent& ev)
{
    if (ev.GetInt() == 0) // un-toggled
    {
        return; // Don't react on UnToggle events
    }

    wxToolBar* toolbar = static_cast<wxToolBar*>(_mainPanel->FindWindow("RenderPreviewRenderModeToolbar"));

    // This function will be called twice, once for the inactivating button and
    // once for the activating button
    if (getToolBarToolByLabel(toolbar, "texturedModeButton")->GetId() == ev.GetId())
    {
        setLightingModeEnabled(false);
    }
    else if (getToolBarToolByLabel(toolbar, "lightingModeButton")->GetId() == ev.GetId())
    {
        setLightingModeEnabled(true);
    }
}

void RenderPreview::onGridButtonClick(wxCommandEvent& ev)
{
	_renderGrid = (ev.GetInt() != 0);

	registry::setValue<bool>(RKEY_RENDERPREVIEW_SHOWGRID, _renderGrid);

	queueDraw();
}

void RenderPreview::onStartPlaybackClick(wxCommandEvent& ev)
{
	startPlayback();
}

void RenderPreview::onStopPlaybackClick(wxCommandEvent& ev)
{
	stopPlayback();
}

void RenderPreview::onPausePlaybackClick(wxCommandEvent& ev)
{
	// Disable the button
	wxToolBar* toolbar = findNamedObject<wxToolBar>(_mainPanel, "RenderPreviewAnimToolbar");
	toolbar->EnableTool(getToolBarToolByLabel(toolbar, "pauseTimeButton")->GetId(), false);

	if (_timer.IsRunning())
    {
        _timer.Stop();
    }
    else
    {
        _timer.Start(MSEC_PER_FRAME); // re-enable playback
    }
}

void RenderPreview::onStepForwardClick(wxCommandEvent& ev)
{
    // Disable the button
	wxToolBar* toolbar = findNamedObject<wxToolBar>(_mainPanel, "RenderPreviewAnimToolbar");
	toolbar->EnableTool(getToolBarToolByLabel(toolbar, "pauseTimeButton")->GetId(), false);

	if (_timer.IsRunning())
    {
        _timer.Stop();
    }

    _renderSystem->setTime(_renderSystem->getTime() + MSEC_PER_FRAME);
    updateFrameSelector();
    queueDraw();
}

void RenderPreview::onStepBackClick(wxCommandEvent& ev)
{
    // Disable the button
    wxToolBar* toolbar = findNamedObject<wxToolBar>(_mainPanel, "RenderPreviewAnimToolbar");
	toolbar->EnableTool(getToolBarToolByLabel(toolbar, "pauseTimeButton")->GetId(), false);

	if (_timer.IsRunning())
    {
        _timer.Stop();
    }

    if (_renderSystem->getTime() > 0)
    {
        _renderSystem->setTime(_renderSystem->getTime() - MSEC_PER_FRAME);
        updateFrameSelector();
    }

    queueDraw();
}

void RenderPreview::onFrameSelected(wxSpinEvent& ev)
{
    jumpToSelectedFrame(static_cast<wxSpinCtrl*>(ev.GetEventObject()));
}

void RenderPreview::onFrameConfirmed(wxCommandEvent& ev)
{
    jumpToSelectedFrame(static_cast<wxSpinCtrl*>(ev.GetEventObject()));
}

void RenderPreview::jumpToSelectedFrame(wxSpinCtrl* spinCtrl)
{
    if (_timer.IsRunning())
    {
        _timer.Stop();
    }

    _renderSystem->setTime(spinCtrl->GetValue() * MSEC_PER_FRAME);
    queueDraw();
}

void RenderPreview::updateFrameSelector()
{
    auto toolbar = findNamedObject<wxToolBar>(_mainPanel, "RenderPreviewAnimToolbar");
    auto frameSelector = static_cast<wxSpinCtrl*>(getToolBarControlByName(toolbar, "FrameSelector")->GetControl());
    frameSelector->SetValue(_renderSystem->getTime() / MSEC_PER_FRAME);
}

void RenderPreview::onSizeAllocate(wxSizeEvent& ev)
{
	_previewWidth = ev.GetSize().GetWidth();
    _previewHeight = ev.GetSize().GetHeight();
}

bool RenderPreview::canDrawGrid()
{
    return true;
}

void RenderPreview::drawGrid()
{
	static float GRID_MAX_DIM = 512.0f;
	static float GRID_STEP = 16.0f;

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_1D);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
	glDisable(GL_BLEND);

	glLineWidth(1);
	glColor3f(0.7f, 0.7f, 0.7f);

    glPushMatrix();

    auto gridOrigin = getGridOrigin();
    glTranslated(gridOrigin.x(), gridOrigin.y(), gridOrigin.z());

	glBegin(GL_LINES);

	for (float x = -GRID_MAX_DIM; x < GRID_MAX_DIM; x += GRID_STEP)
	{
		Vector3 start(x, -GRID_MAX_DIM, 0);
		Vector3 end(x, GRID_MAX_DIM, 0);

		Vector3 start2(GRID_MAX_DIM, x, 0);
		Vector3 end2(-GRID_MAX_DIM, x, 0);

		glVertex2dv(start);
		glVertex2dv(end);

		glVertex2dv(start2);
		glVertex2dv(end2);
	}

	glEnd();

    glPopMatrix();
}

void RenderPreview::drawInfoText()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, static_cast<float>(_previewWidth), 0, static_cast<float>(_previewHeight), -100, 100);

    glScalef(1, -1, 1);
    glTranslatef(0, -static_cast<float>(_previewHeight), 0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (GLEW_VERSION_1_3)
    {
        glClientActiveTexture(GL_TEXTURE0);
        glActiveTexture(GL_TEXTURE0);
    }

    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_DEPTH_TEST);

    glColor3f( 1.f, 1.f, 1.f );
    glLineWidth(1);

    glRasterPos3f(1.0f, static_cast<float>(_previewHeight) - 1.0f, 0.0f);

    _glFont->drawString(getInfoText());
}

std::string RenderPreview::getInfoText()
{
    return fmt::format("{0:.3f} sec.", (_renderSystem->getTime() * 0.001f));
}

void RenderPreview::onGLKeyPress(wxKeyEvent& ev)
{
    if (!_freezePointer.isCapturing(_glWidget))
    {
        return;
    }

    float inc = static_cast<float>(getSceneBounds().getRadius()) * 0.12f;

    if (ev.ShiftDown())
    {
        inc *= 4.0f;
    }

    Vector3 forward(_modelView[2], _modelView[6], _modelView[10]);
    Vector3 right(_modelView[0], _modelView[4], _modelView[8]);

    switch (ev.GetKeyCode())
    {
    case WXK_UP:
        _viewOrigin -= forward * inc;
        break;
    case WXK_DOWN:
        _viewOrigin += forward * inc;
        break;
    case WXK_RIGHT:
        _viewOrigin += right * inc;
        break;
    case WXK_LEFT:
        _viewOrigin -= right * inc;
        break;
    default:
        ev.Skip();
        return;
    }

    updateModelViewMatrix();
    queueDraw();
}

void RenderPreview::_onFrame(wxTimerEvent& ev)
{
    if (!_renderingInProgress)
    {
        _renderSystem->setTime(_renderSystem->getTime() + MSEC_PER_FRAME);
        updateFrameSelector();
        queueDraw();
    }
}

} // namespace
