#pragma once

#include "iclipper.h"
#include "iscenegraph.h"

#include "math/Vector3.h"
#include "math/Matrix4.h"
#include "math/Vector4.h"
#include "gtkutil/FreezePointer.h"
#include "gtkutil/DeferredMotion.h"
#include "gtkutil/WindowPosition.h"
#include "xmlutil/Node.h"
#include "timer.h"

#include <wx/cursor.h>

#include "map/DeferredDraw.h"
#include "camera/CameraObserver.h"
#include "camera/CamWnd.h"
#include "selection/RadiantWindowObserver.h"

	namespace {
		const int XYWND_MINSIZE_X = 100;
		const int XYWND_MINSIZE_Y = 100;
	}

class XYWnd :
	public CameraObserver,
	public scene::Graph::Observer,
	public wxEvtHandler
{
protected:
	// Unique ID of this XYWnd
	int _id;

	wxutil::GLWidget* _wxGLWidget;

	DeferredDraw m_deferredDraw;
	wxutil::DeferredMotion _deferredMouseMotion; // for wxgl

	// The maximum/minimum values of a coordinate
	double _minWorldCoord;
	double _maxWorldCoord;

	// The timer used for chase mouse xyview movements
	Timer _chaseMouseTimer;

	wxutil::FreezePointer _freezePointer;

	wxCursor _defaultCursor;
	wxCursor _crossHairCursor;

	bool _moveStarted;
	bool _zoomStarted;

	bool _chasingMouse;

	double	m_fScale;
	Vector3 m_vOrigin;

	render::View m_view;

	// Shader to use for selected items
	static ShaderPtr _selectedShader;

	int m_ptCursorX, m_ptCursorY;

	unsigned int _wxMouseButtonState;

	int m_nNewBrushPressx;
	int m_nNewBrushPressy;
	scene::INodePtr m_NewBrushDrag;
	bool m_bNewBrushDrag;

	Vector3 m_mousePosition;

	EViewType m_viewType;

	SelectionSystemWindowObserver* m_window_observer;
	selection::Rectangle _dragRectangle;

	wxutil::WindowPosition _windowPosition;

	int m_entityCreate_x, m_entityCreate_y;
	bool m_entityCreate;

  	// Save the current button state
  	unsigned int _eventState;

	bool _isActive;

	int _chasemouseCurrentX;
	int _chasemouseCurrentY;
	int _chasemouseDeltaX;
	int _chasemouseDeltaY;

	Matrix4 m_projection;
	Matrix4 m_modelview;

	int _width;
	int _height;

	int _dragZoom;

	Glib::RefPtr<Gtk::Window> _parent;

	// The handle returned from the Map valid callback signal
	std::size_t _validCallbackHandle;

public:
	// Constructor, this allocates the GL widget
	XYWnd(int uniqueId, wxWindow* parent);

	// Destructor
	virtual ~XYWnd();

	int getId() const;

	void queueDraw();
	wxutil::GLWidget* getGLWidget() const { return _wxGLWidget; }

	void setParent(const Glib::RefPtr<Gtk::Window>& parent);
	const Glib::RefPtr<Gtk::Window>& getParent() const;

	// Capture and release the selected shader
	static void captureStates();
	static void releaseStates();

	// Returns the long type string ("XY Top", "YZ Side", "XZ Front") for use in window titles
	static const std::string getViewTypeTitle(EViewType viewtype);

	// Returns the short type string (XY, XZ, YZ)
	static const std::string getViewTypeStr(EViewType viewtype);

	void positionView(const Vector3& position);
	const Vector3& getOrigin();
	void setOrigin(const Vector3& origin);
	void scroll(int x, int y);
	Vector4 getWindowCoordinates();

	void positionCamera(int x, int y, CamWnd& camwnd);
	void orientCamera(int x, int y, CamWnd& camwnd);

	void draw();
	void drawCameraIcon(const Vector3& origin, const Vector3& angles);
	void drawBlockGrid();
	void drawGrid();

	void NewBrushDrag_Begin(int x, int y);
	void NewBrushDrag(int x, int y);
	void NewBrushDrag_End(int x, int y);

	void convertXYToWorld(int x, int y, Vector3& point);
	void snapToGrid(Vector3& point);

	void mouseToPoint(int x, int y, Vector3& point);

	void updateSelectionBox(const selection::Rectangle& area);

	void beginMove();
	void endMove();

	void beginZoom();
	void endZoom();

	void zoomIn();
	void zoomOut();

	void setActive(bool b);
	bool isActive() const;

	void Clipper_OnLButtonDown(int x, int y);
	void Clipper_OnLButtonUp(int x, int y);
	void Clipper_OnMouseMoved(int x, int y);
	void Clipper_Crosshair_OnMouseMoved(int x, int y);
	void DropClipPoint(int pointx, int pointy);

	void chaseMouse();
	
	void updateModelview();
	void updateProjection();

	void EntityCreate_MouseDown(int x, int y);
	void EntityCreate_MouseMove(int x, int y);
	void EntityCreate_MouseUp(int x, int y);

	virtual void setViewType(EViewType n);
	EViewType getViewType() const;

	void setScale(float f);
	float getScale() const;

	int getWidth() const;
	int getHeight() const;

	// greebo: CameraObserver implementation; gets called when the camera is moved
	void cameraMoved();

	// greebo: This gets called upon scene change
	void onSceneGraphChange();

protected:
	// Disconnects all widgets and unsubscribes as observer
	void destroyXYView();

private:
	unsigned int GetButtonStateForMouseEvent(wxMouseEvent& ev);

	void onContextMenu();
	void drawSizeInfo(int nDim1, int nDim2, const Vector3& vMinBounds, const Vector3& vMaxBounds);

	// callbacks
	bool chaseMouseMotion(int pointx, int pointy, unsigned int state);
	void onIdle(wxIdleEvent& ev);
	
	// The method responsible for mouseMove situations according to <event>
	void handleGLMouseUp(wxMouseEvent& ev);
	void handleGLMouseMove(int x, int y, unsigned int state);
	void handleGLMouseDown(wxMouseEvent& ev);

	// Is called by the DeferredDraw helper
	void performDeferredDraw();

	// wxGLWidget-attached render method
	void onRender();
	void onGLResize(wxSizeEvent& ev);
	void onGLWindowScroll(wxMouseEvent& ev);
	void onGLMouseButtonPress(wxMouseEvent& ev);
	void onGLMouseButtonRelease(wxMouseEvent& ev);
	void onGLMouseMove(int x, int y, unsigned int state);
	void onGLMouseMoveDelta(int x, int y, unsigned int state);
	void onGLMouseCaptureLost();

	void onGLZoomMouseCaptureLost();
	void onGLZoomDelta(int x, int y, unsigned int state);

}; // class XYWnd

/**
 * Shared pointer typedefs.
 */
typedef boost::shared_ptr<XYWnd> XYWndPtr;
typedef boost::weak_ptr<XYWnd> XYWndWeakPtr;
