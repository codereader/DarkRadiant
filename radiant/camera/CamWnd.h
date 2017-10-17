#pragma once

#include "iscenegraph.h"
#include "imousetool.h"
#include "icameraview.h"
#include "irender.h"
#include "wxutil/GLWidget.h"
#include "wxutil/FreezePointer.h"
#include "wxutil/WindowPosition.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/event/KeyEventFilter.h"
#include "wxutil/MouseToolHandler.h"

#include <wx/wxprec.h>
#include <wx/glcanvas.h>
#include <wx/timer.h>
#include "render/View.h"

#include "RadiantCameraView.h"
#include "Camera.h"

#include "selection/Rectangle.h"
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
    public ICameraView,
	public scene::Graph::Observer,
	public util::Noncopyable,
    public sigc::trackable,
	private wxutil::XmlResourceBasedWidget,
	public wxEvtHandler,
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

	RadiantCameraView _cameraView;

	static ShaderPtr _faceHighlightShader;
	static ShaderPtr _primitiveHighlightShader;

	wxutil::FreezePointer _freezePointer;

	// Is true during an active drawing process
	bool _drawing;

	bool _freeMoveEnabled;

    // The GL widget
	wxutil::GLWidget* _wxGLWidget;

	std::size_t _mapValidHandle;

	wxTimer _timer;
    bool _timerLock; // to avoid double-timer-firings

	sigc::connection _glExtensionsInitialisedNotifier;

    wxutil::KeyEventFilterPtr _escapeListener;

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

	void draw();
	void update();

	// The callback when the scene gets changed
	void onSceneGraphChange() override;

	static void captureStates();
	static void releaseStates();

	Camera& getCamera();

	Vector3 getCameraOrigin() const override;
	void setCameraOrigin(const Vector3& origin) override;

	Vector3 getRightVector() const override;
	Vector3 getUpVector() const override;
	Vector3 getForwardVector() const override;

	Vector3 getCameraAngles() const;
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

	void enableDiscreteMoveEvents();
	void enableFreeMoveEvents();
	void disableDiscreteMoveEvents();
	void disableFreeMoveEvents();

	// Increases/decreases the far clip plane distance
	void farClipPlaneIn();
	void farClipPlaneOut();

	void startRenderTime();
	void stopRenderTime();

protected:
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

	void Cam_Draw();
	void onRender();
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
};

/**
 * Shared pointer typedef.
 */
typedef std::shared_ptr<CamWnd> CamWndPtr;
typedef std::weak_ptr<CamWnd> CamWndWeakPtr;

} // namespace
