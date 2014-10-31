#pragma once

#include "iscenegraph.h"
#include "icameraview.h"
#include "irender.h"
#include "wxutil/GLWidget.h"
#include "wxutil/DeferredMotion.h"
#include "wxutil/FreezePointer.h"
#include "wxutil/WindowPosition.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/event/KeyEventFilter.h"

#include <wx/wxprec.h>
#include <wx/glcanvas.h>
#include <wx/timer.h>
#include "render/View.h"
#include "map/DeferredDraw.h"

#include "RadiantCameraView.h"
#include "Camera.h"

#include "selection/Rectangle.h"
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <sigc++/connection.h>
#include "tools/CameraMouseToolEvent.h"

const int CAMWND_MINSIZE_X = 240;
const int CAMWND_MINSIZE_Y = 200;

class SelectionTest;

class CamWnd :
    public ICameraView,
	public scene::Graph::Observer,
	public boost::noncopyable,
    public sigc::trackable,
	private wxutil::XmlResourceBasedWidget,
	public wxEvtHandler
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

	DeferredDraw _deferredDraw;
	wxutil::DeferredMotion _deferredMouseMotion;

	sigc::connection _glExtensionsInitialisedNotifier;

    ui::MouseToolPtr _activeMouseTool;

    wxutil::KeyEventFilterPtr _escapeListener;

public:
	// Constructor and destructor
	CamWnd(wxWindow* parent);

	virtual ~CamWnd();

	// The unique ID of this camwindow
	int getId();

    // ICameraView implementation
    SelectionTestPtr createSelectionTestForPoint(const Vector2& point);
    const VolumeTest& getVolumeTest() const;
    int getDeviceWidth() const;
    int getDeviceHeight() const;
    void queueDraw();

	void draw();
	void update();

	// The callback when the scene gets changed
	void onSceneGraphChange();

	static void captureStates();
	static void releaseStates();

	Camera& getCamera();

	Vector3 getCameraOrigin() const;
	void setCameraOrigin(const Vector3& origin);

	Vector3 getCameraAngles() const;
	void setCameraAngles(const Vector3& angles);

    const Frustum& getViewFrustum() const;

	// greebo: This measures the rendering time during a 360° turn of the camera.
	void benchmark();

	// This tries to find brushes above/below the current camera position and moves the view upwards/downwards
	void changeFloor(const bool up);

	wxutil::GLWidget* getwxGLWidget() const { return _wxGLWidget; }
	wxWindow* getMainWidget() const;

	void enableFreeMove();
	void disableFreeMove();
	bool freeMoveEnabled() const;

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

	void performDeferredDraw();

    ui::CameraMouseToolEvent createMouseEvent(const Vector2& point, const Vector2& delta = Vector2(0, 0));

	void onGLResize(wxSizeEvent& ev);

	void onMouseScroll(wxMouseEvent& ev);

    void handleGLMouseButtonPress(wxMouseEvent& ev);
    void handleGLMouseButtonRelease(wxMouseEvent& ev);
    void handleGLMouseMove(int x, int y, unsigned int state);

	void onGLMouseButtonPress(wxMouseEvent& ev);
	void onGLMouseButtonRelease(wxMouseEvent& ev);
	void onGLMouseMove(int x, int y, unsigned int state);

	void onGLMouseButtonPressFreeMove(wxMouseEvent& ev);
	void onGLMouseButtonReleaseFreeMove(wxMouseEvent& ev);
	void onGLMouseMoveFreeMove(wxMouseEvent& ev);
	
	void onGLMouseMoveFreeMoveDelta(int x, int y, unsigned int state);
	void onGLFreeMoveCaptureLost();

	void onGLExtensionsInitialised();

	void _onFrame(wxTimerEvent& ev);

    void clearActiveMouseTool();
};

/**
 * Shared pointer typedef.
 */
typedef boost::shared_ptr<CamWnd> CamWndPtr;
typedef boost::weak_ptr<CamWnd> CamWndWeakPtr;
