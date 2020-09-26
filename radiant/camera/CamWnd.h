#pragma once

#include "iscenegraph.h"
#include "imousetool.h"
#include "icameraview.h"
#include "ieventmanager.h"
#include "irender.h"
#include "wxutil/GLWidget.h"
#include "wxutil/FreezePointer.h"
#include "wxutil/WindowPosition.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/event/KeyEventFilter.h"
#include "wxutil/MouseToolHandler.h"
#include "wxutil/DeferredMotionDelta.h"

#include <wx/wxprec.h>
#include <wx/glcanvas.h>
#include <wx/timer.h>
#include <wx/stopwatch.h>
#include "render/View.h"

#include "Camera.h"

#include "Rectangle.h"
#include <memory>
#include "util/Noncopyable.h"
#include <sigc++/connection.h>
#include "tools/CameraMouseToolEvent.h"

const int CAMWND_MINSIZE_X = 240;
const int CAMWND_MINSIZE_Y = 200;

class SelectionTest;

namespace ui
{

class CamWnd :
    public wxEvtHandler,
    public IFreeMoveView,
    public scene::Graph::Observer,
    public util::Noncopyable,
    public sigc::trackable,
    private wxutil::XmlResourceBasedWidget,
    protected wxutil::MouseToolHandler
{
private:
    // Overall panel including toolbar and GL widget
    wxPanel* _mainWxWidget;

    // The ID of this window
    int _id;

    static int _maxId;

    render::View _view;

    // The contained camera
    Camera _camera;

    static ShaderPtr _faceHighlightShader;
    static ShaderPtr _primitiveHighlightShader;

    wxutil::FreezePointer _freezePointer;

    // Is true during an active drawing process
    bool _drawing;

    // The GL widget
    wxutil::GLWidget* _wxGLWidget;

    std::size_t _mapValidHandle;

    wxTimer _timer;
    bool _timerLock; // to avoid double-timer-firings

    sigc::connection _glExtensionsInitialisedNotifier;

    wxutil::KeyEventFilterPtr _escapeListener;

    // Remembering the free movement type while holding down a key
    unsigned int _freeMoveFlags;
    wxTimer _freeMoveTimer;
    wxStopWatch _keyControlTimer;

    wxutil::DeferredMotionDelta _deferredMotionDelta;

    bool _strafe; // true when in strafemode toggled by the ctrl-key
    bool _strafeForward; // true when in strafemode by ctrl-key and shift is pressed for forward strafing

public:
    // Constructor and destructor
    CamWnd(wxWindow* parent);

    virtual ~CamWnd();

    // The unique ID of this camwindow
    int getId();

    // ICameraView implementation
    SelectionTestPtr createSelectionTestForPoint(const Vector2& point) override;
    const VolumeTest& getVolumeTest() const override;
    int getDeviceWidth() const override;
    int getDeviceHeight() const override;
    void queueDraw() override;
    void forceRedraw() override;

    void update();

    // The callback when the scene gets changed
    void onSceneGraphChange() override;

    static void captureStates();
    static void releaseStates();

    Camera& getCamera();

    const Vector3& getCameraOrigin() const;
    void setCameraOrigin(const Vector3& origin);

    const Vector3& getCameraAngles() const;
    void setCameraAngles(const Vector3& angles);

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

    void jumpToObject(SelectionTest& selectionTest);

    // Enables/disables the (ordinary) camera movement (non-freelook)
    void addHandlersMove();
    void removeHandlersMove();

    // Increases/decreases the far clip plane distance
    void farClipPlaneIn();
    void farClipPlaneOut();

    void startRenderTime();
    void stopRenderTime();

    void onForwardKey(KeyEventType eventType);
    void onBackwardKey(KeyEventType eventType);
    void onLeftKey(KeyEventType eventType);
    void onRightKey(KeyEventType eventType);
    void onUpKey(KeyEventType eventType);
    void onDownKey(KeyEventType eventType);

protected:
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

    void Cam_Draw();
    bool onRender();
    void drawTime();

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

    void handleFreeMovement(float timePassed);
    void setFreeMoveFlags(unsigned int mask);
    void clearFreeMoveFlags(unsigned int mask);
    // Gets called with the accumulated delta values, as buffered by wxutil::DeferredMotionDelta
    void onDeferredMotionDelta(int x, int y);
    void performFreeMove(int dx, int dy);
};

/**
 * Shared pointer typedef.
 */
typedef std::shared_ptr<CamWnd> CamWndPtr;
typedef std::weak_ptr<CamWnd> CamWndWeakPtr;

} // namespace
