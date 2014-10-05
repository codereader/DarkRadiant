#pragma once

#include "iscenegraph.h"
#include "iorthoview.h"

#include "math/Vector3.h"
#include "math/Matrix4.h"
#include "math/Vector4.h"
#include "wxutil/FreezePointer.h"
#include "wxutil/DeferredMotion.h"

#include <wx/cursor.h>
#include <wx/stopwatch.h>

#include "map/DeferredDraw.h"
#include "camera/CameraObserver.h"
#include "selection/RadiantWindowObserver.h"
#include "imousetool.h"

class XYWnd :
    public IOrthoView,
	public CameraObserver,
	public scene::Graph::Observer,
	public wxEvtHandler
{
protected:
	// Unique ID of this XYWnd
	int _id;

	wxutil::GLWidget* _wxGLWidget;

	DeferredDraw _deferredDraw;
	wxutil::DeferredMotion _deferredMouseMotion; // for wxgl

	// The maximum/minimum values of a coordinate
	double _minWorldCoord;
	double _maxWorldCoord;

	// The timer used for chase mouse xyview movements
	wxStopWatch _chaseMouseTimer;

	wxutil::FreezePointer _freezePointer;

	wxCursor _defaultCursor;
	wxCursor _crossHairCursor;

	bool _chasingMouse;

	double	_scale;
	Vector3 _origin;

	render::View _view;

	// Shader to use for selected items
	static ShaderPtr _selectedShader;

	Vector3 _mousePosition;

	EViewType _viewType;

	SelectionSystemWindowObserver* _windowObserver;
	selection::Rectangle _dragRectangle;

    int _contextMenu_x;
    int _contextMenu_y;
	bool _contextMenu;

  	// Save the current button state
  	unsigned int _eventState;

	bool _isActive;

	int _chasemouseCurrentX;
	int _chasemouseCurrentY;
	int _chasemouseDeltaX;
	int _chasemouseDeltaY;

	Matrix4 _projection;
	Matrix4 _modelView;

	int _width;
	int _height;

    ui::MouseToolPtr _activeMouseTool;

public:
	// Constructor, this allocates the GL widget
	XYWnd(int uniqueId, wxWindow* parent);

	// Destructor
	virtual ~XYWnd();

	int getId() const;

	void queueDraw();
	wxutil::GLWidget* getGLWidget() const { return _wxGLWidget; }

    SelectionTestPtr createSelectionTestForPoint(const Vector2& point);

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

	void draw();
	void drawCameraIcon(const Vector3& origin, const Vector3& angles);
	void drawBlockGrid();
	void drawGrid();

    Vector3 convertXYToWorld(int x, int y);
	void snapToGrid(Vector3& point);

	void mouseToPoint(int x, int y, Vector3& point);

	void updateSelectionBox(const selection::Rectangle& area);

	void beginMove();
	void endMove();

	void zoomIn();
	void zoomOut();

	void setActive(bool b);
	bool isActive() const;

    void setCursorType(IOrthoView::CursorType type);

	void chaseMouse();
	
	void updateModelview();
	void updateProjection();

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
    void clearActiveMouseTool();

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
};

/**
 * Shared pointer typedefs.
 */
typedef boost::shared_ptr<XYWnd> XYWndPtr;
typedef boost::weak_ptr<XYWnd> XYWndWeakPtr;
