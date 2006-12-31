/*
Copyright (C) 1999-2006 Id Software, Inc. and contributors.
For a list of contributors, see the accompanying CONTRIBUTORS file.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if !defined(INCLUDED_XYWINDOW_H)
#define INCLUDED_XYWINDOW_H

#include "math/matrix.h"
#include "signal/signal.h"

#include "gtkutil/cursor.h"
#include "gtkutil/window.h"
#include "gtkutil/xorrectangle.h"
#include "view.h"
#include "map.h"

#include "timer.h"

#include "EventLib.h"
#include "camera/CameraObserver.h"
#include "camera/CamWnd.h"
#include "selection/RadiantWindowObserver.h"

// Constants

const int XYWND_MINSIZE_X = 200;
const int XYWND_MINSIZE_Y = 200;

#include "qerplugin.h"

class Shader;
namespace scene
{
  class Node;
}
typedef struct _GtkWindow GtkWindow;
typedef struct _GtkMenu GtkMenu;

inline const char* ViewType_getTitle(EViewType viewtype)
{
  if(viewtype == XY)
  {
    return "XY Top";
  }
  if(viewtype == XZ)
  {
    return "XZ Front";
  }
  if(viewtype == YZ)
  {
    return "YZ Side";
  }
  return "";
}

class XYWnd :
	public CameraObserver 
{
	GtkWidget* m_gl_widget;
	guint m_sizeHandler;
	guint m_exposeHandler;

	DeferredDraw m_deferredDraw;
	DeferredMotion m_deferred_motion;

	// The maximum/minimum values of a coordinate
	float _minWorldCoord;
	float _maxWorldCoord;

	// The timer used for chase mouse xyview movements
	Timer _chaseMouseTimer;

	FreezePointer _freezePointer;

	bool _moveStarted;
	bool _zoomStarted;
	
	guint _chaseMouseHandler;

	float	m_fScale;
	Vector3 m_vOrigin;
	
	View m_view;
	static Shader* m_state_selected;
	
	int m_ptCursorX, m_ptCursorY;
	
	unsigned int m_buttonstate;
	
	int m_nNewBrushPressx;
	int m_nNewBrushPressy;
	scene::Node* m_NewBrushDrag;
	bool m_bNewBrushDrag;
	
	Vector3 m_mousePosition;
	
	EViewType m_viewType;

	SelectionSystemWindowObserver* m_window_observer;
	XORRectangle m_XORRectangle;
	WindowPositionTracker m_positionTracker;

	int m_entityCreate_x, m_entityCreate_y;
	bool m_entityCreate;
	
  	// Save the current event state
  	GdkEventButton* _event;
  	
  	guint m_move_focusOut;
	guint m_zoom_focusOut;
	
	bool _isActive;
	
	int m_chasemouse_current_x, m_chasemouse_current_y;
	int m_chasemouse_delta_x, m_chasemouse_delta_y;
	
	Matrix4 m_projection;
	Matrix4 m_modelview;
	
	int _width;
	int _height;
	
	int _dragZoom;
	
public:
	Signal0 onDestroyed;
	Signal3<int, int, GdkEventButton*> onMouseDown;

	GtkWindow* m_parent;
	
	// Constructor
	XYWnd();
	
	// Destructor
	~XYWnd();
	
	void queueDraw();	
	GtkWidget* getWidget();

	static void captureStates();
	static void releaseStates();
	
	void positionView(const Vector3& position);
	const Vector3& getOrigin();
	void setOrigin(const Vector3& origin);
	void scroll(int x, int y);
	
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
	
	void updateXORRectangle(Rectangle area);
	
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
	bool chaseMouseMotion(int pointx, int pointy, const unsigned int& state);
	
	void updateModelview();
	void updateProjection();
  	
	void EntityCreate_MouseDown(int x, int y);
	void EntityCreate_MouseMove(int x, int y);
	void EntityCreate_MouseUp(int x, int y);
	
	void onEntityCreate(const std::string& item);
	
	void setViewType(EViewType n);
	EViewType getViewType() const;
	
	void setScale(float f);
	float getScale() const;
	
	int getWidth() const;
	int getHeight() const;

	int& dragZoom();

  	// Save the current GDK event state
  	void setEvent(GdkEventButton* event);
  
	// The method handling the different mouseUp situations according to <event>
	void mouseUp(int x, int y, GdkEventButton* event);

	// The method responsible for mouseMove situations according to <event>
	void mouseMoved(int x, int y, const unsigned int& state);

	// The method handling the different mouseDown situations
	void mouseDown(int x, int y, GdkEventButton* event);
	typedef Member3<XYWnd, int, int, GdkEventButton*, void, &XYWnd::mouseDown> MouseDownCaller;
	
	// greebo: CameraObserver implementation; gets called when the camera is moved
	void cameraMoved();

private:
	void onContextMenu();
	void drawSizeInfo(int nDim1, int nDim2, Vector3& vMinBounds, Vector3& vMaxBounds);

	// GTK Callbacks, these have to be static
	static gboolean	callbackButtonPress(GtkWidget* widget, GdkEventButton* event, XYWnd* self);
	static gboolean	callbackButtonRelease(GtkWidget* widget, GdkEventButton* event, XYWnd* self);
	static void 	callbackMouseMotion(gdouble x, gdouble y, guint state, void* data);
	static gboolean	callbackMouseWheelScroll(GtkWidget* widget, GdkEventScroll* event, XYWnd* self);
	static gboolean	callbackSizeAllocate(GtkWidget* widget, GtkAllocation* allocation, XYWnd* self);	
	static gboolean	callbackExpose(GtkWidget* widget, GdkEventExpose* event, XYWnd* self);
	static void 	callbackMoveDelta(int x, int y, unsigned int state, void* data);
	static gboolean callbackMoveFocusOut(GtkWidget* widget, GdkEventFocus* event, XYWnd* self);
	static gboolean	callbackChaseMouse(gpointer data);
	static gboolean callbackZoomFocusOut(GtkWidget* widget, GdkEventFocus* event, XYWnd* self);
	static void		callbackZoomDelta(int x, int y, unsigned int state, void* data);

}; // class XYWnd

inline void XYWnd_Update(XYWnd& xywnd) {
  xywnd.queueDraw();
}

struct xywindow_globals_t
{
  bool m_bNoStipple;

  xywindow_globals_t() :
    m_bNoStipple(false)
  {}
};

extern xywindow_globals_t g_xywindow_globals;

EViewType GlobalXYWnd_getCurrentViewType();

typedef struct _GtkWindow GtkWindow;
void XY_Top_Shown_Construct(GtkWindow* parent);
void YZ_Side_Shown_Construct(GtkWindow* parent);
void XZ_Front_Shown_Construct(GtkWindow* parent);

void XYWindow_Construct();
void XYWindow_Destroy();

void XYShow_registerCommands();
void XYWnd_registerShortcuts();

#endif
