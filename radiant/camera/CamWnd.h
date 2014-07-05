#pragma once

#include "iscenegraph.h"
#include "irender.h"
#include "gtkutil/GLWidget.h"
#include "gtkutil/DeferredMotion.h"
#include "gtkutil/FreezePointer.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/GladeWidgetHolder.h"
#include "gtkutil/XmlResourceBasedWidget.h"
#include "gtkutil/Timer.h"
#include "selection/RadiantWindowObserver.h"

#include <wx/wxprec.h>
#include <wx/glcanvas.h>
#include "render/View.h"
#include "map/DeferredDraw.h"

#include "RadiantCameraView.h"
#include "Camera.h"

#include "selection/Rectangle.h"
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

const int CAMWND_MINSIZE_X = 240;
const int CAMWND_MINSIZE_Y = 200;

class SelectionTest;

class CamWnd :
	public scene::Graph::Observer,
	public boost::noncopyable,
    public sigc::trackable,
    private gtkutil::GladeWidgetHolder,
	private wxutil::XmlResourceBasedWidget,
	public wxEvtHandler
{
private:
    // Outer GUI widget (containing toolbar and GL widget)
    Gtk::Container* _mainWidget;

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
	gtkutil::GLWidget* _camGLWidget;
	wxutil::GLWidget* _wxGLWidget;

	Glib::RefPtr<Gtk::Window> _parentWindow;

	std::size_t _mapValidHandle;

	selection::Rectangle _dragRectangle;

	gtkutil::Timer _timer;

	// Used in Windows only
	sigc::connection _windowStateConn;

	SelectionSystemWindowObserver* _windowObserver;

	DeferredDraw _deferredDraw;
	wxutil::DeferredMotion _deferredMouseMotion;

public:
	// Constructor and destructor
	CamWnd(wxWindow* parent);

	virtual ~CamWnd();

	// The unique ID of this camwindow
	int getId();

	void queueDraw();
	void draw();
	void update();

	// The callback when the scene gets changed
	void onSceneGraphChange();

	static void captureStates();
	static void releaseStates();

	Camera& getCamera();

	void updateSelectionBox(const selection::Rectangle& area);

	Vector3 getCameraOrigin() const;
	void setCameraOrigin(const Vector3& origin);

	Vector3 getCameraAngles() const;
	void setCameraAngles(const Vector3& angles);

	// greebo: This measures the rendering time during a 360° turn of the camera.
	void benchmark();

	// This tries to find brushes above/below the current camera position and moves the view upwards/downwards
	void changeFloor(const bool up);

	Gtk::Widget* getWidget() const;
	wxutil::GLWidget* getwxGLWidget() const { return _wxGLWidget; }
	wxWindow* getMainWidget() const;
	const Glib::RefPtr<Gtk::Window>& getParent() const;

	/**
	 * Set the immediate parent window of this CamWnd.
	 */
	void setContainer(const Glib::RefPtr<Gtk::Window>& newParent);

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

	void onGLResize(wxSizeEvent& ev);
	bool onExpose(GdkEventExpose* ev);

	void onMouseScroll(wxMouseEvent& ev);

	void onGLMouseButtonPress(wxMouseEvent& ev);
	void onGLMouseButtonRelease(wxMouseEvent& ev);
	void onGLMouseMove(int x, int y, unsigned int state);

	void onGLMouseButtonPressFreeMove(wxMouseEvent& ev);
	void onGLMouseButtonReleaseFreeMove(wxMouseEvent& ev);
	void onGLMouseMoveFreeMove(wxMouseEvent& ev);
	
	void onGLMouseMoveFreeMoveDelta(int x, int y, unsigned int state);
	void onGLFreeMoveCaptureLost();

	static gboolean _onFrame(gpointer data);

protected:
	// Used in Windows only to fix camera views going grey
	void connectWindowStateEvent(Gtk::Window& window);
	void disconnectWindowStateEvent();

	bool onWindowStateEvent(GdkEventWindowState* ev); // only used in Windows
};

/**
 * Shared pointer typedef.
 */
typedef boost::shared_ptr<CamWnd> CamWndPtr;
typedef boost::weak_ptr<CamWnd> CamWndWeakPtr;
