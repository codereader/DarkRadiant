#pragma once

#include <memory>
#include <sigc++/connection.h>

#include "iscenegraph.h"
#include "imousetool.h"
#include "icameraview.h"
#include "igl.h"
#include "ui/ieventmanager.h"

#include "wxutil/DockablePanel.h"
#include "wxutil/GLWidget.h"
#include "wxutil/FreezePointer.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/event/KeyEventFilter.h"
#include "wxutil/MouseToolHandler.h"
#include "wxutil/DeferredMotionDelta.h"

#include <wx/wxprec.h>
#include <wx/timer.h>
#include <wx/stopwatch.h>

#include "render/CamRenderer.h"
#include "render/RenderStatistics.h"
#include "render/View.h"
#include "util/Noncopyable.h"
#include "Rectangle.h"
#include "tools/CameraMouseToolEvent.h"
#include "messages/TextureChanged.h"

constexpr int CAMWND_MINSIZE_X = 240;
constexpr int CAMWND_MINSIZE_Y = 200;

constexpr double SPEED_MOVE = 32.0;
constexpr double SPEED_TURN = 22.5;

class SelectionTest;

namespace render { class CamRenderer; };

namespace ui
{
class CameraWndManager;

/// Main 3D view widget
class CamWnd :
    public wxutil::DockablePanel,
    public camera::IFreeMoveView,
    public scene::Graph::Observer,
    public util::Noncopyable,
    public sigc::trackable,
    private wxutil::XmlResourceBasedWidget,
    protected wxutil::MouseToolHandler
{
private:
    CameraWndManager& _owner;

    // Overall panel including toolbar and GL widget
    wxPanel* _mainWxWidget;

    // The ID of this window
    int _id;

    static int _maxId;

    render::View _view;

    // The contained camera
    camera::ICameraView::Ptr _camera;

    std::unique_ptr<render::CamRenderer> _renderer;
    static render::CamRenderer::HighlightShaders _shaders;

    wxutil::FreezePointer _freezePointer;

    // Is true during an active drawing process
    bool _drawing;

    // Update of this window in the next idle event loop
    bool _updateRequested;

    // The GL widget
    wxutil::GLWidget* _wxGLWidget;

    // Camera toolbar and associated button IDs
    wxToolBar* _camToolbar = nullptr;
    struct {
        // Lighting mode buttons
        int wireFrame = wxID_NONE;
        int flatShade = wxID_NONE;
        int textured = wxID_NONE;
        int lighting = wxID_NONE;
        int lightingShadow = wxID_NONE;

        // Far clip buttons
        int farClipIn = wxID_NONE;
        int farClipOut = wxID_NONE;
        int farClipToggle = wxID_NONE;

        // Animation control buttons
        int startTime = wxID_NONE;
        int stopTime = wxID_NONE;
    } _btnIDs;

    std::size_t _mapValidHandle;

    // Timer for animation
    wxTimer _timer;
    bool _timerLock; // to avoid double-timer-firings

    sigc::connection _glExtensionsInitialisedNotifier;

    wxutil::KeyEventFilterPtr _escapeListener;

    // Render statistics for display in the window (frame render time etc)
    render::RenderStatistics _renderStats;

    // Remembering the free movement type while holding down a key
    bool _freeMoveEnabled;
    unsigned int _freeMoveFlags;
    wxTimer _freeMoveTimer;
    wxStopWatch _keyControlTimer;

    wxutil::DeferredMotionDelta _deferredMotionDelta;

    bool _strafe; // true when in strafemode toggled by the ctrl-key
    bool _strafeForward; // true when in strafemode by ctrl-key and shift is pressed for forward strafing

    IGLFont::Ptr _glFont;

    sigc::connection _shadowMappingKeyChangedHandler;
    std::size_t _textureChangedHandler;

public:
    CamWnd(wxWindow* parent, CameraWndManager& owner);
    ~CamWnd() override;

    // The unique ID of this camwindow
    int getId();

    /// Return the camera toolbar
    wxToolBar* getToolbar() { return _camToolbar; }

    SelectionTestPtr createSelectionTestForPoint(const Vector2& point) override;
    const VolumeTest& getVolumeTest() const override;
    bool supportsDragSelections() override;
    int getDeviceWidth() const override;
    int getDeviceHeight() const override;
    void queueDraw() override;
    void forceRedraw() override;

    // Note: this only updates the GL viewport size, it doesn't resize the GL widget itself
    void setDeviceDimensions(int width, int height);

    void update();

    // The callback when the scene gets changed
    void onSceneGraphChange() override;

    static void captureStates();
    static void releaseStates();

    camera::ICameraView& getCamera();

    /// \see ICameraView::getCameraAngles
    const Vector3& getCameraAngles() const;

    /// \see ICameraView::setCameraAngles
    void setCameraAngles(const Vector3& angles);

    /// \see ICameraView::getCameraOrigin
    const Vector3& getCameraOrigin() const;

    /// \see ICameraView::setCameraOrigin
    void setCameraOrigin(const Vector3& origin);

    const Frustum& getViewFrustum() const;

    // greebo: This measures the rendering time during a 360° turn of the camera.
    void benchmark();

    // This tries to find brushes above/below the current camera position and moves the view upwards/downwards
    void changeFloor(const bool up);

    wxutil::GLWidget* getwxGLWidget() const { return _wxGLWidget; }
    wxWindow* getMainWidget() const;

    void enableFreeMove() override;
    void disableFreeMove() override;
    bool freeMoveEnabled() const override;

    // Enables/disables the (ordinary) camera movement (non-freelook)
    void addHandlersMove();
    void removeHandlersMove();

    // Increases/decreases the far clip plane distance
    float getFarClipPlaneDistance() const;
    void setFarClipPlaneDistance(float distance);
    bool getFarClipPlaneEnabled() const;
    void setFarClipPlaneEnabled(bool enabled);

    void startRenderTime();
    void stopRenderTime();

    void onForwardKey(KeyEventType eventType);
    void onBackwardKey(KeyEventType eventType);
    void onLeftKey(KeyEventType eventType);
    void onRightKey(KeyEventType eventType);
    void onUpKey(KeyEventType eventType);
    void onDownKey(KeyEventType eventType);

    void pitchUpDiscrete();
    void pitchDownDiscrete();
    void rotateRightDiscrete();
    void rotateLeftDiscrete();
    void moveRightDiscrete(double units);
    void moveLeftDiscrete(double units);
    void moveDownDiscrete(double units);
    void moveUpDiscrete(double units);
    void moveBackDiscrete(double units);
    void moveForwardDiscrete(double units);

protected:
    void onPanelActivated() override;
    void onPanelDeactivated() override;

    void handleFreeMoveKeyEvent(KeyEventType eventType, unsigned int movementFlags);
    void handleKeyEvent(KeyEventType eventType, unsigned int freeMoveFlags, const std::function<void()>& discreteMovement);

    // Required overrides being a MouseToolHandler
    virtual MouseTool::Result processMouseDownEvent(const MouseToolPtr& tool, const Vector2& point) override;
    virtual MouseTool::Result processMouseUpEvent(const MouseToolPtr& tool, const Vector2& point) override;
    virtual MouseTool::Result processMouseMoveEvent(const MouseToolPtr& tool, int x, int y) override;
    virtual void startCapture(const MouseToolPtr& tool) override;
    virtual void endCapture() override;
    virtual IInteractiveView& getInteractiveView() override;

private:
    void connectEventHandlers();
    void disconnectEventHandlers();

    void constructGUIComponents();
    void constructToolbar();
    void setFarClipButtonSensitivity();
    void onRenderModeButtonsChanged(wxCommandEvent& ev);
    void updateActiveRenderModeButton();
    void onFarClipPlaneOutClick(wxCommandEvent& ev);
    void onFarClipPlaneInClick(wxCommandEvent& ev);
    void onStartTimeButtonClick(wxCommandEvent& ev);
    void onStopTimeButtonClick(wxCommandEvent& ev);
    void updateToolbarVisibility();

    void ensureFont();
    void Cam_Draw();
    bool onRender();
    void drawTime();
    void drawGrid();
    void requestRedraw(bool force);
    
    CameraMouseToolEvent createMouseEvent(const Vector2& point, const Vector2& delta = Vector2(0, 0));

    void onGLResize(wxSizeEvent& ev);

    void onMouseScroll(wxMouseEvent& ev);

    void onGLMouseButtonPress(wxMouseEvent& ev);
    void onGLMouseButtonRelease(wxMouseEvent& ev);
    void onGLMouseMove(wxMouseEvent& ev);

    // Mouse motion callback used in freelook mode only, processes deltas
    void handleGLMouseMoveFreeMoveDelta(int x, int y, unsigned int state);
    
    void onGLExtensionsInitialised();

    void onFrame(wxTimerEvent& ev);
    void onFreeMoveTimer(wxTimerEvent& ev);
    void onIdle(wxIdleEvent& ev);

    void handleFreeMovement(float timePassed);
    void setFreeMoveFlags(unsigned int mask);
    void clearFreeMoveFlags(unsigned int mask);
    // Gets called with the accumulated delta values, as buffered by wxutil::DeferredMotionDelta
    void onDeferredMotionDelta(int x, int y);
    void performFreeMove(int dx, int dy);

    void handleTextureChanged(radiant::TextureChangedMessage& msg);
};

/**
 * Shared pointer typedef.
 */
typedef std::shared_ptr<CamWnd> CamWndPtr;
typedef std::weak_ptr<CamWnd> CamWndWeakPtr;

} // namespace
