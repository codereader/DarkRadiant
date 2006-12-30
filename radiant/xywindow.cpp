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

//
// XY Window
//
// Leonardo Zide (leo@lokigames.com)
//

#include "xywindow.h"

#include "debugging/debugging.h"

#include "iclipper.h"
#include "ientity.h"
#include "ieclass.h"
#include "igl.h"
#include "brush/TexDef.h"
#include "ibrush.h"
#include "iundo.h"
#include "iregistry.h"

#include <gtk/gtklabel.h>
#include <gtk/gtkmenuitem.h>

#include "generic/callback.h"
#include "string/string.h"
#include "stream/stringstream.h"

#include "scenelib.h"
#include "renderer.h"
#include "moduleobserver.h"

#include "gtkutil/menu.h"
#include "gtkutil/container.h"
#include "gtkutil/widget.h"
#include "gtkutil/glwidget.h"
#include "gtkmisc.h"
#include "select.h"
#include "csg.h"
#include "brushmanip.h"
#include "entity.h"
#include "camera/GlobalCamera.h"
#include "texwindow.h"
#include "mainframe.h"
#include "preferences.h"
#include "commands.h"
#include "grid.h"
#include "windowobservers.h"
#include "plugin.h"
#include "ui/colourscheme/ColourScheme.h"
#include "ui/eventmapper/EventMapper.h"

#include "selection/SelectionBox.h"

#include <boost/lexical_cast.hpp>

struct xywindow_globals_private_t
{
  bool  d_showgrid;

  // these are in the View > Show menu with Show coordinates
  bool  show_names;
  bool  show_coordinates;
  bool  show_angles;
  bool  show_outline;
  bool  show_axis;

  bool d_show_work;

  bool     show_blocks;
  int		       blockSize;

  bool m_bCamXYUpdate;
  bool m_bChaseMouse;

  xywindow_globals_private_t() :
    d_showgrid(true),

    show_names(false),
    show_coordinates(true),
    show_angles(true),
    show_outline(false),
    show_axis(true),

    d_show_work(false),

    show_blocks(false),

    m_bCamXYUpdate(true),
    m_bChaseMouse(true)
  {
  }

};

xywindow_globals_t g_xywindow_globals;
xywindow_globals_private_t g_xywindow_globals_private;

void XYWnd::SetScale(float f)
{
  m_fScale = f;
  updateProjection();
  updateModelview();
  XYWnd_Update(*this);
}

EViewType GlobalXYWnd_getCurrentViewType()
{
  ASSERT_NOTNULL(g_pParentWnd);
  ASSERT_NOTNULL(g_pParentWnd->ActiveXY());
  return g_pParentWnd->ActiveXY()->GetViewType();
}

// =============================================================================
// variables

bool g_bCrossHairs = false;

GtkMenu* XYWnd::m_mnuDrop = 0;

// this is disabled, and broken
// http://zerowing.idsoftware.com/bugzilla/show_bug.cgi?id=394
#if 0
void WXY_Print()
{
  long width, height;
  width = g_pParentWnd->ActiveXY()->Width();
  height = g_pParentWnd->ActiveXY()->Height();
  unsigned char* img;
  const char* filename;

  filename = file_dialog(GTK_WIDGET(MainFrame_getWindow()), FALSE, "Save Image", 0, FILTER_BMP);
  if (!filename)
    return;

  g_pParentWnd->ActiveXY()->MakeCurrent();
  img = (unsigned char*)malloc (width*height*3);
  glReadPixels (0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,img);

  FILE *fp; 
  fp = fopen(filename, "wb");
  if (fp)
  {
    unsigned short bits;
    unsigned long cmap, bfSize;

    bits = 24;
    cmap = 0;
    bfSize = 54 + width*height*3;

    long byteswritten = 0;
    long pixoff = 54 + cmap*4;
    short res = 0;
    char m1 ='B', m2 ='M';
    fwrite(&m1, 1, 1, fp);      byteswritten++; // B
    fwrite(&m2, 1, 1, fp);      byteswritten++; // M
    fwrite(&bfSize, 4, 1, fp);  byteswritten+=4;// bfSize
    fwrite(&res, 2, 1, fp);     byteswritten+=2;// bfReserved1
    fwrite(&res, 2, 1, fp);     byteswritten+=2;// bfReserved2
    fwrite(&pixoff, 4, 1, fp);  byteswritten+=4;// bfOffBits

    unsigned long biSize = 40, compress = 0, size = 0;
    long pixels = 0;
    unsigned short planes = 1;
    fwrite(&biSize, 4, 1, fp);  byteswritten+=4;// biSize
    fwrite(&width, 4, 1, fp);   byteswritten+=4;// biWidth
    fwrite(&height, 4, 1, fp);  byteswritten+=4;// biHeight
    fwrite(&planes, 2, 1, fp);  byteswritten+=2;// biPlanes
    fwrite(&bits, 2, 1, fp);    byteswritten+=2;// biBitCount
    fwrite(&compress, 4, 1, fp);byteswritten+=4;// biCompression
    fwrite(&size, 4, 1, fp);    byteswritten+=4;// biSizeImage
    fwrite(&pixels, 4, 1, fp);  byteswritten+=4;// biXPelsPerMeter
    fwrite(&pixels, 4, 1, fp);  byteswritten+=4;// biYPelsPerMeter
    fwrite(&cmap, 4, 1, fp);    byteswritten+=4;// biClrUsed
    fwrite(&cmap, 4, 1, fp);    byteswritten+=4;// biClrImportant

    unsigned long widthDW = (((width*24) + 31) / 32 * 4);
    long row, row_size = width*3;
    for (row = 0; row < height; row++)
    {
        unsigned char* buf = img+row*row_size;

      // write a row
      int col;
      for (col = 0; col < row_size; col += 3)
        {
          putc(buf[col+2], fp);
          putc(buf[col+1], fp);
          putc(buf[col], fp);
        }
      byteswritten += row_size; 

      unsigned long count;
      for (count = row_size; count < widthDW; count++)
        {
        putc(0, fp);    // dummy
          byteswritten++;
        }
    }

    fclose(fp);
  }

  free (img);
}
#endif


#include "timer.h"

Timer g_chasemouse_timer;

/* greebo: This gets repeatedly called during a mouse chase operation.
 * The call is triggered by a timer, that gets start in XYWnd::chaseMouseMotion();
 */
void XYWnd::ChaseMouse() {
	float multiplier = g_chasemouse_timer.elapsed_msec() / 10.0f;
	Scroll(float_to_integer(multiplier * m_chasemouse_delta_x), float_to_integer(multiplier * -m_chasemouse_delta_y));

	//globalOutputStream() << "chasemouse: multiplier=" << multiplier << " x=" << m_chasemouse_delta_x << " y=" << m_chasemouse_delta_y << '\n';

	mouseMoved(m_chasemouse_current_x, m_chasemouse_current_y , _event->state);
  
	// greebo: Restart the timer, so that it can trigger again
	g_chasemouse_timer.start();
}

// This is the chase mouse handler that gets connected by XYWnd::chaseMouseMotion()
// It passes te call on to the XYWnd::ChaseMouse() method. 
gboolean xywnd_chasemouse(gpointer data) {
	// Convert the pointer <data> in and XYWnd* pointer and call the method
	reinterpret_cast<XYWnd*>(data)->ChaseMouse();
	return TRUE;
}

/* greebo: This handles the "chase mouse" behaviour, if the user drags something
 * beyond the XY window boundaries. If the chaseMouse option (currently a global)
 * is set true, the view origin gets relocated along with the mouse movements.
 * 
 * @returns: true, if the mousechase has been performed, false if no mouse chase was necessary
 */
bool XYWnd::chaseMouseMotion(int pointx, int pointy, const unsigned int& state) {
	m_chasemouse_delta_x = 0;
	m_chasemouse_delta_y = 0;

	// These are the events that are allowed
	bool isAllowedEvent = GlobalEventMapper().stateMatchesXYViewEvent(ui::xySelect, state)
						  || GlobalEventMapper().stateMatchesXYViewEvent(ui::xyNewBrushDrag, state);

	// greebo: The mouse chase is only active when the according global is set to true and if we 
	// are in the right state
	if (g_xywindow_globals_private.m_bChaseMouse && isAllowedEvent) {
		const int epsilon = 16;

		// Calculate the X delta
		if (pointx < epsilon) {
			m_chasemouse_delta_x = std::max(pointx, 0) - epsilon;
		}
		else if ((pointx - m_nWidth) > -epsilon) {
			m_chasemouse_delta_x = std::min((pointx - m_nWidth), 0) + epsilon;
		}

		// Calculate the Y delta
		if (pointy < epsilon) {
			m_chasemouse_delta_y = std::max(pointy, 0) - epsilon;
		}
		else if ((pointy - m_nHeight) > -epsilon) {
			m_chasemouse_delta_y = std::min((pointy - m_nHeight), 0) + epsilon;
		}

		// If any of the deltas is uneqal to zero the mouse chase is to be performed
		if (m_chasemouse_delta_y != 0 || m_chasemouse_delta_x != 0) {
			
			//globalOutputStream() << "chasemouse motion: x=" << pointx << " y=" << pointy << "... ";
			m_chasemouse_current_x = pointx;
			m_chasemouse_current_y = pointy;
			
			// Start the timer, if there isn't one connected already
			if (m_chasemouse_handler == 0) {
				//globalOutputStream() << "chasemouse timer start... ";
				g_chasemouse_timer.start();
				
				m_chasemouse_handler = g_idle_add(xywnd_chasemouse, this);
			}
			// Return true to signal that there are no other mouseMotion handlers to be performed
			// see xywnd_motion() callback function
			return true;
		}
		else {
			// All deltas are zero, so there is no more mouse chasing necessary, remove the handlers
			if (m_chasemouse_handler != 0) {
				//globalOutputStream() << "chasemouse cancel\n";
				g_source_remove(m_chasemouse_handler);
				m_chasemouse_handler = 0;
			}
		}
	}
	else {
		// Remove the handlers, the user has probably released the mouse button during chase
		if(m_chasemouse_handler != 0) {
			//globalOutputStream() << "chasemouse cancel\n";
			g_source_remove(m_chasemouse_handler);
			m_chasemouse_handler = 0;
		}
	}
	
	// No mouse chasing has been performed, return false
	return false;
}

// =============================================================================
// XYWnd class
Shader* XYWnd::m_state_selected = 0;

/* greebo: This is the callback for the mouse_press event that is invoked by GTK
 * it checks for the correct event type and passes the call to the according xy view window.
 * 
 * Note: I think these should be static members of the XYWnd class, shouldn't they? 
 */
gboolean xywnd_button_press(GtkWidget* widget, GdkEventButton* event, XYWnd* xywnd) {
	if (event->type == GDK_BUTTON_PRESS) {
		// Put the focus on the xy view that has been clicked on
		g_pParentWnd->SetActiveXY(xywnd);

		//xywnd->ButtonState_onMouseDown(buttons_for_event_button(event));
		xywnd->setEvent(event);
		
		// Pass the GdkEventButton* to the XYWnd class, the boolean <true> is passed but never used
		xywnd->onMouseDown(static_cast<int>(event->x), static_cast<int>(event->y), event);
	}
	return FALSE;
}

// greebo: This is the GTK callback for mouseUp. 
gboolean xywnd_button_release(GtkWidget* widget, GdkEventButton* event, XYWnd* xywnd) {
	
	// greebo: Check for the correct event type (redundant?)
	if (event->type == GDK_BUTTON_RELEASE) {
		// Call the according mouseUp method
		xywnd->mouseUp(static_cast<int>(event->x), static_cast<int>(event->y), event);

		// Clear the buttons that the button_release has been called with
		//xywnd->ButtonState_onMouseUp(buttons_for_event_button(event));
		xywnd->setEvent(event);
	}
	return FALSE;
}

/* greebo: This is the GTK callback for mouse movement. */
void xywnd_motion(gdouble x, gdouble y, guint state, void* data) {
	
	// Convert the passed pointer into a XYWnd pointer
	XYWnd* xywnd = reinterpret_cast<XYWnd*>(data);
	
	// Call the chaseMouse method
	if (xywnd->chaseMouseMotion(static_cast<int>(x), static_cast<int>(y), state)) {
		return;
	}
	
	// This gets executed, if the above chaseMouse call returned false, i.e. no mouse chase has been performed
	xywnd->mouseMoved(static_cast<int>(x), static_cast<int>(y), state);
}

// This is the onWheelScroll event, that is used to Zoom in/out in the xyview
gboolean xywnd_wheel_scroll(GtkWidget* widget, GdkEventScroll* event, XYWnd* xywnd)
{
	if (event->direction == GDK_SCROLL_UP) {
		xywnd->zoomIn();
	}
	else if (event->direction == GDK_SCROLL_DOWN) {
		xywnd->zoomOut();
	}
	return FALSE;
}

gboolean xywnd_size_allocate(GtkWidget* widget, GtkAllocation* allocation, XYWnd* xywnd)
{
  xywnd->m_nWidth = allocation->width;
  xywnd->m_nHeight = allocation->height;
  xywnd->updateProjection();
  xywnd->m_window_observer->onSizeChanged(xywnd->Width(), xywnd->Height());
  return FALSE;
}

gboolean xywnd_expose(GtkWidget* widget, GdkEventExpose* event, XYWnd* xywnd)
{
  if(glwidget_make_current(xywnd->GetWidget()) != FALSE)
  {
    if(Map_Valid(g_map) && ScreenUpdates_Enabled())
    {
      GlobalOpenGL_debugAssertNoErrors();
      xywnd->XY_Draw();
      GlobalOpenGL_debugAssertNoErrors();

      xywnd->m_XORRectangle.set(rectangle_t());
    }
    glwidget_swap_buffers(xywnd->GetWidget());
  }
  return FALSE;
}

// Constructor
XYWnd::XYWnd() :
	m_gl_widget(glwidget_new(FALSE)),
	m_deferredDraw(WidgetQueueDrawCaller(*m_gl_widget)),
	m_deferred_motion(xywnd_motion, this),
	_minWorldCoord(GlobalRegistry().getFloat("game/defaults/minWorldCoord")),
	_maxWorldCoord(GlobalRegistry().getFloat("game/defaults/maxWorldCoord")),
	m_parent(0),
	m_window_observer(NewWindowObserver()),
	m_XORRectangle(m_gl_widget),
	m_chasemouse_handler(0) 
{
	m_bActive = false;
	m_buttonstate = 0;

	m_bNewBrushDrag = false;
	m_move_started = false;
	m_zoom_started = false;

	m_nWidth = 0;
	m_nHeight = 0;

	m_vOrigin[0] = 0;
	m_vOrigin[1] = 20;
	m_vOrigin[2] = 46;
	m_fScale = 1;
	m_viewType = XY;

	m_entityCreate = false;

	m_mnuDrop = 0;

	GlobalWindowObservers_add(m_window_observer);
	GlobalWindowObservers_connectWidget(m_gl_widget);

	m_window_observer->setRectangleDrawCallback(MemberCaller1<XYWnd, Rectangle, &XYWnd::updateXORRectangle>(*this));
	m_window_observer->setView(m_view);

	gtk_widget_ref(m_gl_widget);

	gtk_widget_set_events(m_gl_widget, GDK_DESTROY | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);
	GTK_WIDGET_SET_FLAGS(m_gl_widget, GTK_CAN_FOCUS);
	gtk_widget_set_size_request(m_gl_widget, XYWND_MINSIZE_X, XYWND_MINSIZE_Y);

	m_sizeHandler = g_signal_connect(G_OBJECT(m_gl_widget), "size_allocate", G_CALLBACK(xywnd_size_allocate), this);
	m_exposeHandler = g_signal_connect(G_OBJECT(m_gl_widget), "expose_event", G_CALLBACK(xywnd_expose), this);

	g_signal_connect(G_OBJECT(m_gl_widget), "button_press_event", G_CALLBACK(xywnd_button_press), this);
	g_signal_connect(G_OBJECT(m_gl_widget), "button_release_event", G_CALLBACK(xywnd_button_release), this);
	g_signal_connect(G_OBJECT(m_gl_widget), "motion_notify_event", G_CALLBACK(DeferredMotion::gtk_motion), &m_deferred_motion);

	g_signal_connect(G_OBJECT(m_gl_widget), "scroll_event", G_CALLBACK(xywnd_wheel_scroll), this);

	Map_addValidCallback(g_map, DeferredDrawOnMapValidChangedCaller(m_deferredDraw));

	updateProjection();
	updateModelview();

	AddSceneChangeCallback(ReferenceCaller<XYWnd, &XYWnd_Update>(*this));
	// greebo: Connect <self> as CameraObserver to the CamWindow. This way this class gets notified on camera change
	GlobalCamera().addCameraObserver(this);

	PressedButtons_connect(g_pressedButtons, m_gl_widget);

	onMouseDown.connectLast(makeSignalHandler3(MouseDownCaller(), *this));
}

// Destructor
XYWnd::~XYWnd() {
	onDestroyed();

	if (m_mnuDrop != 0) {
		gtk_widget_destroy(GTK_WIDGET(m_mnuDrop));
		m_mnuDrop = 0;
	}

	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_sizeHandler);
	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_exposeHandler);

	gtk_widget_unref(m_gl_widget);

	m_window_observer->release();
}


void XYWnd::setEvent(GdkEventButton* event) {
	_event = event;
}

void XYWnd::captureStates() {
	m_state_selected = GlobalShaderCache().capture("$XY_OVERLAY");
}

void XYWnd::releaseStates() {
	GlobalShaderCache().release("$XY_OVERLAY");
}

const Vector3& XYWnd::GetOrigin() {
	return m_vOrigin;
}

void XYWnd::SetOrigin(const Vector3& origin) {
	m_vOrigin = origin;
	updateModelview();
}

void XYWnd::Scroll(int x, int y) {
	int nDim1 = (m_viewType == YZ) ? 1 : 0;
	int nDim2 = (m_viewType == XY) ? 1 : 2;
	m_vOrigin[nDim1] += x / m_fScale;
	m_vOrigin[nDim2] += y / m_fScale;
	updateModelview();
	queueDraw();
}

void XYWnd::DropClipPoint(int pointx, int pointy) {
	Vector3 point;

	XY_ToPoint(pointx, pointy, point);

	Vector3 mid;
	Select_GetMid(mid);
	GlobalClipper().setViewType(static_cast<EViewType>(GetViewType()));
	int nDim = (GlobalClipper().getViewType() == YZ ) ?  0 : ( (GlobalClipper().getViewType() == XZ) ? 1 : 2 );
	point[nDim] = mid[nDim];
	vector3_snap(point, GetGridSize());
	GlobalClipper().newClipPoint(point);
}

void XYWnd::Clipper_OnLButtonDown(int x, int y) {
	Vector3 mousePosition;
	XY_ToPoint(x, y , mousePosition);
	
	ClipPoint* foundClipPoint = GlobalClipper().find(mousePosition, (EViewType)m_viewType, m_fScale);
	
	GlobalClipper().setMovingClip(foundClipPoint);
	
	if (foundClipPoint == NULL) {
		DropClipPoint(x, y);
	}
}

void XYWnd::Clipper_OnLButtonUp(int x, int y) {
	GlobalClipper().setMovingClip(NULL);
}

void XYWnd::Clipper_OnMouseMoved(int x, int y) {
	ClipPoint* movingClip = GlobalClipper().getMovingClip();
	
	if (movingClip != NULL) {
		XY_ToPoint(x, y , GlobalClipper().getMovingClipCoords());
		XY_SnapToGrid(GlobalClipper().getMovingClipCoords());
		GlobalClipper().update();
		ClipperChangeNotify();
	}
}

void XYWnd::Clipper_Crosshair_OnMouseMoved(int x, int y) {
	Vector3 mousePosition;
	XY_ToPoint(x, y , mousePosition);
	if (GlobalClipper().clipMode() && GlobalClipper().find(mousePosition, (EViewType)m_viewType, m_fScale) != 0) {
		GdkCursor *cursor;
		cursor = gdk_cursor_new (GDK_CROSSHAIR);
		gdk_window_set_cursor (m_gl_widget->window, cursor);
		gdk_cursor_unref (cursor);
	} else {
		gdk_window_set_cursor (m_gl_widget->window, 0);
	}
}

void XYWnd::positionCamera(int x, int y, CamWnd& camwnd) {
	Vector3 origin = camwnd.getCameraOrigin();
	XY_ToPoint(x, y, origin);
	XY_SnapToGrid(origin);
	camwnd.setCameraOrigin(origin);
}

void XYWnd::orientCamera(int x, int y, CamWnd& camwnd) {
	Vector3	point = g_vector3_identity;
	XY_ToPoint(x, y, point);
	XY_SnapToGrid(point);
	point -= camwnd.getCameraOrigin();

	int n1 = (GetViewType() == XY) ? 1 : 2;
	int n2 = (GetViewType() == YZ) ? 1 : 0;
	int nAngle = (GetViewType() == XY) ? CAMERA_YAW : CAMERA_PITCH;
	if (point[n1] || point[n2]) {
		Vector3 angles(camwnd.getCameraAngles());
		angles[nAngle] = static_cast<float>(radians_to_degrees(atan2 (point[n1], point[n2])));
		camwnd.setCameraAngles(angles);
	}
}

// Callback that gets invoked on camera move
void XYWnd::cameraMoved() {
	if (g_xywindow_globals_private.m_bCamXYUpdate) {
		XYWnd_Update(*this);
	}
}


/*
==============
NewBrushDrag
==============
*/
void XYWnd::NewBrushDrag_Begin(int x, int y)
{
  m_NewBrushDrag = 0;
  m_nNewBrushPressx = x;
  m_nNewBrushPressy = y;

  m_bNewBrushDrag = true;
  GlobalUndoSystem().start();
}

void XYWnd::NewBrushDrag_End(int x, int y)
{
  if(m_NewBrushDrag != 0)
  {
    GlobalUndoSystem().finish("brushDragNew");
  }
}

void XYWnd::NewBrushDrag(int x, int y)
{
  Vector3	mins, maxs;
  XY_ToPoint(m_nNewBrushPressx, m_nNewBrushPressy, mins);
  XY_SnapToGrid(mins);
	XY_ToPoint(x, y, maxs);
  XY_SnapToGrid(maxs);

  int nDim = (m_viewType == XY) ? 2 : (m_viewType == YZ) ? 0 : 1;

  mins[nDim] = float_snapped(Select_getWorkZone().d_work_min[nDim], GetGridSize());
  maxs[nDim] = float_snapped(Select_getWorkZone().d_work_max[nDim], GetGridSize());

  if (maxs[nDim] <= mins[nDim])
    maxs[nDim] = mins[nDim] + GetGridSize();

  for(int i=0 ; i<3 ; i++)
  {
    if (mins[i] == maxs[i])
      return;	// don't create a degenerate brush
    if (mins[i] > maxs[i])
    {
      float	temp = mins[i];
      mins[i] = maxs[i];
      maxs[i] = temp;
    }
  }

  if(m_NewBrushDrag == 0)
  {
    NodeSmartReference node(GlobalBrushCreator().createBrush());
    Node_getTraversable(Map_FindOrInsertWorldspawn(g_map))->insert(node);

    scene::Path brushpath(makeReference(GlobalSceneGraph().root()));
    brushpath.push(makeReference(*Map_GetWorldspawn(g_map)));
    brushpath.push(makeReference(node.get()));
    selectPath(brushpath, true);

    m_NewBrushDrag = node.get_pointer();
  }

	Scene_BrushResize_Selected(GlobalSceneGraph(), 
  							   AABB::createFromMinMax(mins, maxs), 
  							   TextureBrowser_GetSelectedShader(GlobalTextureBrowser()));
}

void entitycreate_activated(GtkWidget* item)
{
  g_pParentWnd->ActiveXY()->OnEntityCreate(gtk_label_get_text(GTK_LABEL(GTK_BIN(item)->child)));
}

void EntityClassMenu_addItem(GtkMenu* menu, const char* name)
{
  GtkMenuItem* item = GTK_MENU_ITEM(gtk_menu_item_new_with_label(name));
  g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(entitycreate_activated), item);
  gtk_widget_show(GTK_WIDGET(item));
  menu_add_item(menu, item);
}

class EntityClassMenuInserter : public EntityClassVisitor
{
  typedef std::pair<GtkMenu*, CopiedString> MenuPair;
  typedef std::vector<MenuPair> MenuStack;
  MenuStack m_stack;
  std::string m_previous;
public:
  EntityClassMenuInserter(GtkMenu* menu)
  {
    m_stack.reserve(2);
    m_stack.push_back(MenuPair(menu, ""));
  }
  ~EntityClassMenuInserter()
  {
    if(!string_empty(m_previous.c_str()))
    {
      addItem(m_previous.c_str(), "");
    }
  }
  void visit(IEntityClass* e)
  {
    if(m_previous.size() > 0)
    {
      addItem(m_previous.c_str(), e->getName().c_str());
    }
    m_previous = e->getName();
  }
  void pushMenu(const CopiedString& name)
  {
    GtkMenuItem* item = GTK_MENU_ITEM(gtk_menu_item_new_with_label(name.c_str()));
    gtk_widget_show(GTK_WIDGET(item));
    container_add_widget(GTK_CONTAINER(m_stack.back().first), GTK_WIDGET(item));

    GtkMenu* submenu = GTK_MENU(gtk_menu_new());
    gtk_menu_item_set_submenu(item, GTK_WIDGET(submenu));

    m_stack.push_back(MenuPair(submenu, name));
  }
  void popMenu()
  {
    m_stack.pop_back();
  }
  void addItem(const char* name, const char* next)
  {
    const char* underscore = strchr(name, '_');

    if(underscore != 0 && underscore != name)
    {
      bool nextEqual = string_equal_n(name, next, (underscore + 1) - name);
      const char* parent = m_stack.back().second.c_str();

      if(!string_empty(parent)
        && string_length(parent) == std::size_t(underscore - name)
        && string_equal_n(name, parent, underscore - name)) // this is a child
      {
      }
      else if(nextEqual)
      {
        if(m_stack.size() == 2)
        {
          popMenu();
        }
        pushMenu(CopiedString(StringRange(name, underscore)));
      }
      else if(m_stack.size() == 2)
      {
        popMenu();
      }
    }
    else if(m_stack.size() == 2)
    {
      popMenu();
    }

    EntityClassMenu_addItem(m_stack.back().first, name);
  }
};

/* Context menu
 */

#include "ui/ortho/OrthoContextMenu.h"

void XYWnd::OnContextMenu() {
	// Get the click point in 3D space
	Vector3 point;
	mouseToPoint(m_entityCreate_x, m_entityCreate_y, point);
	// Display the menu, passing the coordinates for creation
	ui::OrthoContextMenu::displayInstance(point);	
}

FreezePointer g_xywnd_freezePointer;

void XYWnd_moveDelta(int x, int y, unsigned int state, void* data)
{
  reinterpret_cast<XYWnd*>(data)->EntityCreate_MouseMove(x, y);
  reinterpret_cast<XYWnd*>(data)->Scroll(-x, y);
}

gboolean XYWnd_Move_focusOut(GtkWidget* widget, GdkEventFocus* event, XYWnd* xywnd)
{
  xywnd->Move_End();
  return FALSE;
}

void XYWnd::Move_Begin()
{
  if(m_move_started)
  {
    Move_End();
  }
  m_move_started = true;
  g_xywnd_freezePointer.freeze_pointer(m_parent != 0 ? m_parent : MainFrame_getWindow(), XYWnd_moveDelta, this);
  m_move_focusOut = g_signal_connect(G_OBJECT(m_gl_widget), "focus_out_event", G_CALLBACK(XYWnd_Move_focusOut), this);
}

void XYWnd::Move_End()
{
  m_move_started = false;
  g_xywnd_freezePointer.unfreeze_pointer(m_parent != 0 ? m_parent : MainFrame_getWindow());
  g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_move_focusOut);
}

int g_dragZoom = 0;

void XYWnd_zoomDelta(int x, int y, unsigned int state, void* data)
{
  if(y != 0)
  {
    g_dragZoom += y;

    while(abs(g_dragZoom) > 8)
    {
      if(g_dragZoom > 0)
      {
      	reinterpret_cast<XYWnd*>(data)->zoomOut();
        g_dragZoom -= 8;
      }
      else
      {
        reinterpret_cast<XYWnd*>(data)->zoomIn();
        g_dragZoom += 8;
      }
    }
  }
}

gboolean XYWnd_Zoom_focusOut(GtkWidget* widget, GdkEventFocus* event, XYWnd* xywnd)
{
  xywnd->Zoom_End();
  return FALSE;
}

void XYWnd::Zoom_Begin()
{
  if(m_zoom_started)
  {
    Zoom_End();
  }
  m_zoom_started = true;
  g_dragZoom = 0;
  g_xywnd_freezePointer.freeze_pointer(m_parent != 0 ? m_parent : MainFrame_getWindow(), XYWnd_zoomDelta, this);
  m_zoom_focusOut = g_signal_connect(G_OBJECT(m_gl_widget), "focus_out_event", G_CALLBACK(XYWnd_Zoom_focusOut), this);
}

void XYWnd::Zoom_End()
{
  m_zoom_started = false;
  g_xywnd_freezePointer.unfreeze_pointer(m_parent != 0 ? m_parent : MainFrame_getWindow());
  g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_zoom_focusOut);
}

// makes sure the selected brush or camera is in view
void XYWnd::PositionView(const Vector3& position)
{
  int nDim1 = (m_viewType == YZ) ? 1 : 0;
  int nDim2 = (m_viewType == XY) ? 1 : 2;

  m_vOrigin[nDim1] = position[nDim1];
  m_vOrigin[nDim2] = position[nDim2];

  updateModelview();

  XYWnd_Update(*this);
}

void XYWnd::SetViewType(EViewType viewType)
{
  m_viewType = viewType; 
  updateModelview();

  if(m_parent != 0)
  {
    gtk_window_set_title(m_parent, ViewType_getTitle(m_viewType));
  }
}

/* This gets called by the GTK callback function.
 */
void XYWnd::mouseDown(int x, int y, GdkEventButton* event) {

	if (GlobalEventMapper().stateMatchesXYViewEvent(ui::xyMoveView, event)) {
		Move_Begin();
    	EntityCreate_MouseDown(x, y);
	}
	
	if (GlobalEventMapper().stateMatchesXYViewEvent(ui::xyZoom, event)) {
		Zoom_Begin();
	}
	
	if (GlobalEventMapper().stateMatchesXYViewEvent(ui::xyCameraMove, event)) {
		positionCamera(x, y, *g_pParentWnd->GetCamWnd());
	}
	
	if (GlobalEventMapper().stateMatchesXYViewEvent(ui::xyCameraAngle, event)) {
		orientCamera(x, y, *g_pParentWnd->GetCamWnd());
	}
	
	// Only start a NewBrushDrag operation, if not other elements are selected
	if (GlobalSelectionSystem().countSelected() == 0 && 
		GlobalEventMapper().stateMatchesXYViewEvent(ui::xyNewBrushDrag, event)) 
	{
		NewBrushDrag_Begin(x, y);
		return; // Prevent the call from being passed to the windowobserver
	}
	
	if (GlobalEventMapper().stateMatchesXYViewEvent(ui::xySelect, event)) {
		// There are two possibilites for the "select" click: Clip or Select
		if (GlobalClipper().clipMode()) {
			Clipper_OnLButtonDown(x, y);
			return; // Prevent the call from being passed to the windowobserver
		}
	}
	
	// Pass the call to the window observer
	m_window_observer->onMouseDown(WindowVector(x, y), event);
}

// This gets called by either the GTK Callback or the method that is triggered by the mousechase timer 
void XYWnd::mouseMoved(int x, int y, const unsigned int& state) {
	
	if (GlobalEventMapper().stateMatchesXYViewEvent(ui::xyCameraMove, state)) {
		positionCamera(x, y, *g_pParentWnd->GetCamWnd());
	}
	
	if (GlobalEventMapper().stateMatchesXYViewEvent(ui::xyCameraAngle, state)) {
		orientCamera(x, y, *g_pParentWnd->GetCamWnd());
	}
	
	// Check, if we are in a NewBrushDrag operation and continue it
	if (m_bNewBrushDrag && GlobalEventMapper().stateMatchesXYViewEvent(ui::xyNewBrushDrag, state)) {
		NewBrushDrag(x, y);
		return; // Prevent the call from being passed to the windowobserver
	}
	
	if (GlobalEventMapper().stateMatchesXYViewEvent(ui::xySelect, state)) {
		// Check, if we have a clip point operation running
		if (GlobalClipper().clipMode() && GlobalClipper().getMovingClip() != 0) {
			Clipper_OnMouseMoved(x, y);
			return; // Prevent the call from being passed to the windowobserver
		}
	}
	
	// default windowobserver::mouseMotion call, if no other clauses called "return" till now
	m_window_observer->onMouseMotion(WindowVector(x, y), state);

	m_mousePosition[0] = m_mousePosition[1] = m_mousePosition[2] = 0.0;
	XY_ToPoint(x, y , m_mousePosition);
	XY_SnapToGrid(m_mousePosition);

	StringOutputStream status(64);
	status << "x:: " << FloatFormat(m_mousePosition[0], 6, 1)
			<< "  y:: " << FloatFormat(m_mousePosition[1], 6, 1)
			<< "  z:: " << FloatFormat(m_mousePosition[2], 6, 1);
	g_pParentWnd->SetStatusText(g_pParentWnd->m_position_status, status.c_str());

	if (g_bCrossHairs) {
		XYWnd_Update(*this);
	}

	Clipper_Crosshair_OnMouseMoved(x, y);
}

// greebo: The mouseUp method gets called by the GTK callback above
void XYWnd::mouseUp(int x, int y, GdkEventButton* event) {
	
	// End move
	if (m_move_started) {
		Move_End();
		EntityCreate_MouseUp(x, y);
	}
	
	// End zoom
	if (m_zoom_started) {
		Zoom_End();
	}
	
	// Finish any pending NewBrushDrag operations
	if (m_bNewBrushDrag) {
		// End the NewBrushDrag operation
		m_bNewBrushDrag = false;
		NewBrushDrag_End(x, y);
		return; // Prevent the call from being passed to the windowobserver
	}
	
	if (GlobalClipper().clipMode() && GlobalEventMapper().stateMatchesXYViewEvent(ui::xySelect, event)) {
		// End the clip operation
		Clipper_OnLButtonUp(x, y);
		return; // Prevent the call from being passed to the windowobserver
	}
	
	// Pass the call to the window observer
	m_window_observer->onMouseUp(WindowVector(x, y), event);
}

void XYWnd::EntityCreate_MouseDown(int x, int y) {
	m_entityCreate = true;
	m_entityCreate_x = x;
	m_entityCreate_y = y;
}

void XYWnd::EntityCreate_MouseMove(int x, int y) {
	if (m_entityCreate && (m_entityCreate_x != x || m_entityCreate_y != y)) {
		m_entityCreate = false;
	}
}

void XYWnd::EntityCreate_MouseUp(int x, int y) {
	if (m_entityCreate) {
		m_entityCreate = false;
		OnContextMenu();
	}
}

inline float screen_normalised(int pos, unsigned int size)
{
  return ((2.0f * pos) / size) - 1.0f;
}

inline float normalised_to_world(float normalised, float world_origin, float normalised2world_scale)
{
  return world_origin + normalised * normalised2world_scale;
}


// TTimo: watch it, this doesn't init one of the 3 coords
void XYWnd::XY_ToPoint (int x, int y, Vector3& point) {
	float normalised2world_scale_x = m_nWidth / 2 / m_fScale;
	float normalised2world_scale_y = m_nHeight / 2 / m_fScale;
	
	if (m_viewType == XY) {
		point[0] = normalised_to_world(screen_normalised(x, m_nWidth), m_vOrigin[0], normalised2world_scale_x);
		point[1] = normalised_to_world(-screen_normalised(y, m_nHeight), m_vOrigin[1], normalised2world_scale_y);
	} 
	else if (m_viewType == YZ) {
		point[1] = normalised_to_world(screen_normalised(x, m_nWidth), m_vOrigin[1], normalised2world_scale_x);
		point[2] = normalised_to_world(-screen_normalised(y, m_nHeight), m_vOrigin[2], normalised2world_scale_y);
	} 
	else {
		point[0] = normalised_to_world(screen_normalised(x, m_nWidth), m_vOrigin[0], normalised2world_scale_x);
		point[2] = normalised_to_world(-screen_normalised(y, m_nHeight), m_vOrigin[2], normalised2world_scale_y);
	}
}


void XYWnd::XY_SnapToGrid(Vector3& point) {
	if (m_viewType == XY) {
		point[0] = float_snapped(point[0], GetGridSize());
		point[1] = float_snapped(point[1], GetGridSize());
	}
	else if (m_viewType == YZ) {
		point[1] = float_snapped(point[1], GetGridSize());
		point[2] = float_snapped(point[2], GetGridSize());
	}
	else {
		point[0] = float_snapped(point[0], GetGridSize());
		point[2] = float_snapped(point[2], GetGridSize());
	}
}


/*
============================================================================

DRAWING

============================================================================
*/

/*
==============
XY_DrawGrid
==============
*/

double two_to_the_power(int power)
{
  return pow(2.0f, power);
}

void XYWnd::XY_DrawGrid()
{
  float	x, y, xb, xe, yb, ye;
  float		w, h;
  char	text[32];
  float step, minor_step, stepx, stepy;
  step = minor_step = stepx = stepy = GetGridSize();
  
  int minor_power = Grid_getPower();
  int mask;

  while((minor_step * m_fScale) <= 4.0f) // make sure minor grid spacing is at least 4 pixels on the screen
  {
    ++minor_power;
    minor_step *= 2;
  }
  int power = minor_power;
  while((power % 3) != 0 || (step * m_fScale) <= 32.0f) // make sure major grid spacing is at least 32 pixels on the screen
  {
    ++power;
    step = float(two_to_the_power(power));
  }
  mask = (1 << (power - minor_power)) - 1;
  while ((stepx * m_fScale) <= 32.0f) // text step x must be at least 32
    stepx *= 2;
  while ((stepy * m_fScale) <= 32.0f) // text step y must be at least 32
    stepy *= 2;
  

  glDisable(GL_TEXTURE_2D);
  glDisable(GL_TEXTURE_1D);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  glLineWidth(1);

  w = (m_nWidth / 2 / m_fScale);
  h = (m_nHeight / 2 / m_fScale);

  int nDim1 = (m_viewType == YZ) ? 1 : 0;
  int nDim2 = (m_viewType == XY) ? 1 : 2;

  xb = m_vOrigin[nDim1] - w;
  if (xb < region_mins[nDim1])
    xb = region_mins[nDim1];
  xb = step * floor (xb/step);

  xe = m_vOrigin[nDim1] + w;
  if (xe > region_maxs[nDim1])
    xe = region_maxs[nDim1];
  xe = step * ceil (xe/step);

  yb = m_vOrigin[nDim2] - h;
  if (yb < region_mins[nDim2])
    yb = region_mins[nDim2];
  yb = step * floor (yb/step);

  ye = m_vOrigin[nDim2] + h;
  if (ye > region_maxs[nDim2])
    ye = region_maxs[nDim2];
  ye = step * ceil (ye/step);

  // djbob
  // draw minor blocks
  if (g_xywindow_globals_private.d_showgrid)
  {
  	ui::ColourItem& colourGridBack = ColourSchemes().getColour("grid_background");
  	ui::ColourItem& colourGridMinor = ColourSchemes().getColour("grid_minor");
  	ui::ColourItem& colourGridMajor = ColourSchemes().getColour("grid_major");
  	
  	if (colourGridMinor != colourGridBack) {
      glColor3fv(colourGridMinor);

      glBegin (GL_LINES);
      int i = 0;
      for (x = xb ; x < xe ; x += minor_step, ++i)
      {
        if((i & mask) != 0)
        {
          glVertex2f (x, yb);
          glVertex2f (x, ye);
        }
      }
      i = 0;
      for (y = yb ; y < ye ; y += minor_step, ++i)
      {
        if((i & mask) != 0)
        {
          glVertex2f (xb, y);
          glVertex2f (xe, y);
        }
      }
      glEnd();
    }

    // draw major blocks
    if (colourGridMajor != colourGridBack) {
      glColor3fv(colourGridMajor);

      glBegin (GL_LINES);
      for (x=xb ; x<=xe ; x+=step)
      {
        glVertex2f (x, yb);
        glVertex2f (x, ye);
      }
      for (y=yb ; y<=ye ; y+=step)
      {
        glVertex2f (xb, y);
        glVertex2f (xe, y);
      }
      glEnd();
    }
  }

  // draw coordinate text if needed
  if ( g_xywindow_globals_private.show_coordinates)
  {
  	glColor3fv(ColourSchemes().getColourVector3("grid_text"));
		float offx = m_vOrigin[nDim2] + h - 6 / m_fScale, offy = m_vOrigin[nDim1] - w + 1 / m_fScale;
		for (x = xb - fmod(xb, stepx); x <= xe ; x += stepx)
		{
		  glRasterPos2f (x, offx);
			sprintf (text, "%g", x);
			GlobalOpenGL().drawString(text);
		}
		for (y = yb - fmod(yb, stepy); y <= ye ; y += stepy)
		{
		  glRasterPos2f (offy, y);
			sprintf (text, "%g", y);
			GlobalOpenGL().drawString(text);
		}

    if (Active()) {
    	glColor3fv(ColourSchemes().getColourVector3("active_view_name"));
    }

    // we do this part (the old way) only if show_axis is disabled
    if (!g_xywindow_globals_private.show_axis)
    {
      glRasterPos2f ( m_vOrigin[nDim1] - w + 35 / m_fScale, m_vOrigin[nDim2] + h - 20 / m_fScale );

      GlobalOpenGL().drawString(ViewType_getTitle(m_viewType));
    }
  }

  if ( g_xywindow_globals_private.show_axis)
  {
    const char g_AxisName[3] = { 'X', 'Y', 'Z' };

	const std::string colourNameX = (m_viewType == YZ) ? "axis_y" : "axis_x";
	const std::string colourNameY = (m_viewType == XY) ? "axis_y" : "axis_z";
	const Vector3& colourX = ColourSchemes().getColourVector3(colourNameX);
	const Vector3& colourY = ColourSchemes().getColourVector3(colourNameY);

    // draw two lines with corresponding axis colors to highlight current view
    // horizontal line: nDim1 color
    glLineWidth(2);
    glBegin( GL_LINES );
    glColor3fv (colourX);
    glVertex2f( m_vOrigin[nDim1] - w + 40 / m_fScale, m_vOrigin[nDim2] + h - 45 / m_fScale );
    glVertex2f( m_vOrigin[nDim1] - w + 65 / m_fScale, m_vOrigin[nDim2] + h - 45 / m_fScale );
    glVertex2f( 0, 0 );
    glVertex2f( 32 / m_fScale, 0 );
    glColor3fv (colourY);
    glVertex2f( m_vOrigin[nDim1] - w + 40 / m_fScale, m_vOrigin[nDim2] + h - 45 / m_fScale );
    glVertex2f( m_vOrigin[nDim1] - w + 40 / m_fScale, m_vOrigin[nDim2] + h - 20 / m_fScale );
    glVertex2f( 0, 0 );
    glVertex2f( 0, 32 / m_fScale );
    glEnd();
    glLineWidth(1);
    // now print axis symbols
    glColor3fv (colourX);
    glRasterPos2f ( m_vOrigin[nDim1] - w + 55 / m_fScale, m_vOrigin[nDim2] + h - 55 / m_fScale );
    GlobalOpenGL().drawChar(g_AxisName[nDim1]);
    glRasterPos2f (28 / m_fScale, -10 / m_fScale );
    GlobalOpenGL().drawChar(g_AxisName[nDim1]);
    glColor3fv (colourY);
    glRasterPos2f ( m_vOrigin[nDim1] - w + 25 / m_fScale, m_vOrigin[nDim2] + h - 30 / m_fScale );
    GlobalOpenGL().drawChar(g_AxisName[nDim2]);
    glRasterPos2f ( -10 / m_fScale, 28 / m_fScale );
    GlobalOpenGL().drawChar(g_AxisName[nDim2]);

  }

  // show current work zone?
  // the work zone is used to place dropped points and brushes
  if (g_xywindow_globals_private.d_show_work)
  {
    glColor3fv( ColourSchemes().getColourVector3("workzone") );
    glBegin( GL_LINES );
    glVertex2f( xb, Select_getWorkZone().d_work_min[nDim2] );
    glVertex2f( xe, Select_getWorkZone().d_work_min[nDim2] );
    glVertex2f( xb, Select_getWorkZone().d_work_max[nDim2] );
    glVertex2f( xe, Select_getWorkZone().d_work_max[nDim2] );
    glVertex2f( Select_getWorkZone().d_work_min[nDim1], yb );
    glVertex2f( Select_getWorkZone().d_work_min[nDim1], ye );
    glVertex2f( Select_getWorkZone().d_work_max[nDim1], yb );
    glVertex2f( Select_getWorkZone().d_work_max[nDim1], ye );
    glEnd();
  }
}

/*
==============
XY_DrawBlockGrid
==============
*/
void XYWnd::XY_DrawBlockGrid()
{
  if(Map_FindWorldspawn(g_map) == 0)
  {
    return;
  }
	// Set a default blocksize of 1024
	g_xywindow_globals_private.blockSize = 1024;

	// Check the worldspawn for a custom blocksize
	Entity* worldSpawn = Node_getEntity(*Map_GetWorldspawn(g_map));
	assert(worldSpawn);
	std::string sizeVal = worldSpawn->getKeyValue("_blocksize");

	// Parse and set the custom blocksize if found
	if (!sizeVal.empty()) {
		g_xywindow_globals_private.blockSize = 
			boost::lexical_cast<int>(sizeVal);
	}

  float	x, y, xb, xe, yb, ye;
  float		w, h;
  char	text[32];

  glDisable(GL_TEXTURE_2D);
  glDisable(GL_TEXTURE_1D);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);

  w = (m_nWidth / 2 / m_fScale);
  h = (m_nHeight / 2 / m_fScale);

  int nDim1 = (m_viewType == YZ) ? 1 : 0;
  int nDim2 = (m_viewType == XY) ? 1 : 2;

  xb = m_vOrigin[nDim1] - w;
  if (xb < region_mins[nDim1])
    xb = region_mins[nDim1];
  xb = static_cast<float>(g_xywindow_globals_private.blockSize * floor (xb/g_xywindow_globals_private.blockSize));

  xe = m_vOrigin[nDim1] + w;
  if (xe > region_maxs[nDim1])
    xe = region_maxs[nDim1];
  xe = static_cast<float>(g_xywindow_globals_private.blockSize * ceil (xe/g_xywindow_globals_private.blockSize));

  yb = m_vOrigin[nDim2] - h;
  if (yb < region_mins[nDim2])
    yb = region_mins[nDim2];
  yb = static_cast<float>(g_xywindow_globals_private.blockSize * floor (yb/g_xywindow_globals_private.blockSize));

  ye = m_vOrigin[nDim2] + h;
  if (ye > region_maxs[nDim2])
    ye = region_maxs[nDim2];
  ye = static_cast<float>(g_xywindow_globals_private.blockSize * ceil (ye/g_xywindow_globals_private.blockSize));

  // draw major blocks

  glColor3fv(ColourSchemes().getColourVector3("grid_block"));
  glLineWidth (2);

  glBegin (GL_LINES);
	
  for (x=xb ; x<=xe ; x+=g_xywindow_globals_private.blockSize)
  {
    glVertex2f (x, yb);
    glVertex2f (x, ye);
  }

  if (m_viewType == XY)
  {
	for (y=yb ; y<=ye ; y+=g_xywindow_globals_private.blockSize)
	{
	  glVertex2f (xb, y);
	  glVertex2f (xe, y);
	}
  }
	
  glEnd();
  glLineWidth (1);

  // draw coordinate text if needed

  if (m_viewType == XY && m_fScale > .1)
  {
	for (x=xb ; x<xe ; x+=g_xywindow_globals_private.blockSize)
		for (y=yb ; y<ye ; y+=g_xywindow_globals_private.blockSize)
		{
		  glRasterPos2f (x+(g_xywindow_globals_private.blockSize/2), y+(g_xywindow_globals_private.blockSize/2));
			sprintf (text, "%i,%i",(int)floor(x/g_xywindow_globals_private.blockSize), (int)floor(y/g_xywindow_globals_private.blockSize) );
			GlobalOpenGL().drawString(text);
		}
  }

  glColor4f(0, 0, 0, 0);
}

void XYWnd::DrawCameraIcon(const Vector3& origin, const Vector3& angles)
{
  float	x, y, fov, box;
  double a;

  fov = 48 / m_fScale;
  box = 16 / m_fScale;

  if (m_viewType == XY)
  {
    x = origin[0];
    y = origin[1];
    a = degrees_to_radians(angles[CAMERA_YAW]);
  }
  else if (m_viewType == YZ)
  {
    x = origin[1];
    y = origin[2];
    a = degrees_to_radians(angles[CAMERA_PITCH]);
  }
  else
  {
    x = origin[0];
    y = origin[2];
    a = degrees_to_radians(angles[CAMERA_PITCH]);
  }

  glColor3fv ( ColourSchemes().getColourVector3("camera_icon") );
  glBegin(GL_LINE_STRIP);
  glVertex3f (x-box,y,0);
  glVertex3f (x,y+(box/2),0);
  glVertex3f (x+box,y,0);
  glVertex3f (x,y-(box/2),0);
  glVertex3f (x-box,y,0);
  glVertex3f (x+box,y,0);
  glEnd();
	
  glBegin(GL_LINE_STRIP);
  glVertex3f (x + static_cast<float>(fov*cos(a+c_pi/4)), y + static_cast<float>(fov*sin(a+c_pi/4)), 0);
  glVertex3f (x, y, 0);
  glVertex3f (x + static_cast<float>(fov*cos(a-c_pi/4)), y + static_cast<float>(fov*sin(a-c_pi/4)), 0);
  glEnd();

}


float Betwixt(float f1, float f2)
{
  if (f1 > f2)
    return f2 + ((f1 - f2) / 2);
  else
    return f1 + ((f2 - f1) / 2);
}


// can be greatly simplified but per usual i am in a hurry 
// which is not an excuse, just a fact
void XYWnd::PaintSizeInfo(int nDim1, int nDim2, Vector3& vMinBounds, Vector3& vMaxBounds)
{
  if (vMinBounds == vMaxBounds) {
    return;
  }
  const char* g_pDimStrings[] = {"x:", "y:", "z:"};
  typedef const char* OrgStrings[2];
  const OrgStrings g_pOrgStrings[] = { { "x:", "y:", }, { "x:", "z:", }, { "y:", "z:", } };

  Vector3 vSize(vMaxBounds - vMinBounds);

  glColor3fv(ColourSchemes().getColourVector3("brush_size_info"));

  StringOutputStream dimensions(16);

  if (m_viewType == XY)
  {
    glBegin (GL_LINES);

    glVertex3f(vMinBounds[nDim1], vMinBounds[nDim2] - 6.0f  / m_fScale, 0.0f);
    glVertex3f(vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f / m_fScale, 0.0f);

    glVertex3f(vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f  / m_fScale, 0.0f);
    glVertex3f(vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f  / m_fScale, 0.0f);

    glVertex3f(vMaxBounds[nDim1], vMinBounds[nDim2] - 6.0f  / m_fScale, 0.0f);
    glVertex3f(vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f / m_fScale, 0.0f);
  

    glVertex3f(vMaxBounds[nDim1] + 6.0f  / m_fScale, vMinBounds[nDim2], 0.0f);
    glVertex3f(vMaxBounds[nDim1] + 10.0f  / m_fScale, vMinBounds[nDim2], 0.0f);

    glVertex3f(vMaxBounds[nDim1] + 10.0f  / m_fScale, vMinBounds[nDim2], 0.0f);
    glVertex3f(vMaxBounds[nDim1] + 10.0f  / m_fScale, vMaxBounds[nDim2], 0.0f);
  
    glVertex3f(vMaxBounds[nDim1] + 6.0f  / m_fScale, vMaxBounds[nDim2], 0.0f);
    glVertex3f(vMaxBounds[nDim1] + 10.0f  / m_fScale, vMaxBounds[nDim2], 0.0f);

    glEnd();

    glRasterPos3f (Betwixt(vMinBounds[nDim1], vMaxBounds[nDim1]),  vMinBounds[nDim2] - 20.0f  / m_fScale, 0.0f);
    dimensions << g_pDimStrings[nDim1] << vSize[nDim1];
    GlobalOpenGL().drawString(dimensions.c_str());
    dimensions.clear();
    
    glRasterPos3f (vMaxBounds[nDim1] + 16.0f  / m_fScale, Betwixt(vMinBounds[nDim2], vMaxBounds[nDim2]), 0.0f);
    dimensions << g_pDimStrings[nDim2] << vSize[nDim2];
    GlobalOpenGL().drawString(dimensions.c_str());
    dimensions.clear();

    glRasterPos3f (vMinBounds[nDim1] + 4, vMaxBounds[nDim2] + 8 / m_fScale, 0.0f);
    dimensions << "(" << g_pOrgStrings[0][0] << vMinBounds[nDim1] << "  " << g_pOrgStrings[0][1] << vMaxBounds[nDim2] << ")";
    GlobalOpenGL().drawString(dimensions.c_str());
  }
  else if (m_viewType == XZ)
  {
    glBegin (GL_LINES);

    glVertex3f(vMinBounds[nDim1], 0, vMinBounds[nDim2] - 6.0f  / m_fScale);
    glVertex3f(vMinBounds[nDim1], 0, vMinBounds[nDim2] - 10.0f / m_fScale);

    glVertex3f(vMinBounds[nDim1], 0,vMinBounds[nDim2] - 10.0f  / m_fScale);
    glVertex3f(vMaxBounds[nDim1], 0,vMinBounds[nDim2] - 10.0f  / m_fScale);

    glVertex3f(vMaxBounds[nDim1], 0,vMinBounds[nDim2] - 6.0f  / m_fScale);
    glVertex3f(vMaxBounds[nDim1], 0,vMinBounds[nDim2] - 10.0f / m_fScale);
  

    glVertex3f(vMaxBounds[nDim1] + 6.0f  / m_fScale, 0,vMinBounds[nDim2]);
    glVertex3f(vMaxBounds[nDim1] + 10.0f  / m_fScale, 0,vMinBounds[nDim2]);

    glVertex3f(vMaxBounds[nDim1] + 10.0f  / m_fScale, 0,vMinBounds[nDim2]);
    glVertex3f(vMaxBounds[nDim1] + 10.0f  / m_fScale, 0,vMaxBounds[nDim2]);
  
    glVertex3f(vMaxBounds[nDim1] + 6.0f  / m_fScale, 0,vMaxBounds[nDim2]);
    glVertex3f(vMaxBounds[nDim1] + 10.0f  / m_fScale, 0,vMaxBounds[nDim2]);

    glEnd();

    glRasterPos3f (Betwixt(vMinBounds[nDim1], vMaxBounds[nDim1]), 0, vMinBounds[nDim2] - 20.0f  / m_fScale);
    dimensions << g_pDimStrings[nDim1] << vSize[nDim1];
    GlobalOpenGL().drawString(dimensions.c_str());
    dimensions.clear();
    
    glRasterPos3f (vMaxBounds[nDim1] + 16.0f  / m_fScale, 0, Betwixt(vMinBounds[nDim2], vMaxBounds[nDim2]));
    dimensions << g_pDimStrings[nDim2] << vSize[nDim2];
    GlobalOpenGL().drawString(dimensions.c_str());
    dimensions.clear();

    glRasterPos3f (vMinBounds[nDim1] + 4, 0, vMaxBounds[nDim2] + 8 / m_fScale);
    dimensions << "(" << g_pOrgStrings[1][0] << vMinBounds[nDim1] << "  " << g_pOrgStrings[1][1] << vMaxBounds[nDim2] << ")";
    GlobalOpenGL().drawString(dimensions.c_str());
  }
  else
  {
    glBegin (GL_LINES);

    glVertex3f(0, vMinBounds[nDim1], vMinBounds[nDim2] - 6.0f  / m_fScale);
    glVertex3f(0, vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f / m_fScale);

    glVertex3f(0, vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f  / m_fScale);
    glVertex3f(0, vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f  / m_fScale);

    glVertex3f(0, vMaxBounds[nDim1], vMinBounds[nDim2] - 6.0f  / m_fScale);
    glVertex3f(0, vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f / m_fScale);
  

    glVertex3f(0, vMaxBounds[nDim1] + 6.0f  / m_fScale, vMinBounds[nDim2]);
    glVertex3f(0, vMaxBounds[nDim1] + 10.0f  / m_fScale, vMinBounds[nDim2]);

    glVertex3f(0, vMaxBounds[nDim1] + 10.0f  / m_fScale, vMinBounds[nDim2]);
    glVertex3f(0, vMaxBounds[nDim1] + 10.0f  / m_fScale, vMaxBounds[nDim2]);
  
    glVertex3f(0, vMaxBounds[nDim1] + 6.0f  / m_fScale, vMaxBounds[nDim2]);
    glVertex3f(0, vMaxBounds[nDim1] + 10.0f  / m_fScale, vMaxBounds[nDim2]);

    glEnd();

    glRasterPos3f (0, Betwixt(vMinBounds[nDim1], vMaxBounds[nDim1]),  vMinBounds[nDim2] - 20.0f  / m_fScale);
    dimensions << g_pDimStrings[nDim1] << vSize[nDim1];
    GlobalOpenGL().drawString(dimensions.c_str());
    dimensions.clear();
    
    glRasterPos3f (0, vMaxBounds[nDim1] + 16.0f  / m_fScale, Betwixt(vMinBounds[nDim2], vMaxBounds[nDim2]));
    dimensions << g_pDimStrings[nDim2] << vSize[nDim2];
    GlobalOpenGL().drawString(dimensions.c_str());
    dimensions.clear();

    glRasterPos3f (0, vMinBounds[nDim1] + 4.0f, vMaxBounds[nDim2] + 8 / m_fScale);
    dimensions << "(" << g_pOrgStrings[2][0] << vMinBounds[nDim1] << "  " << g_pOrgStrings[2][1] << vMaxBounds[nDim2] << ")";
    GlobalOpenGL().drawString(dimensions.c_str());
  }
}

class XYRenderer: public Renderer
{
  struct state_type
  {
    state_type() :
    m_highlight(0),
    m_state(0)
    {
    }  
    unsigned int m_highlight;
    Shader* m_state;
  };
public:
  XYRenderer(RenderStateFlags globalstate, Shader* selected) :
    m_globalstate(globalstate),
    m_state_selected(selected)
  {
    ASSERT_NOTNULL(selected);
    m_state_stack.push_back(state_type());
  }

  void SetState(Shader* state, EStyle style)
  {
    ASSERT_NOTNULL(state);
    if(style == eWireframeOnly)
      m_state_stack.back().m_state = state;
  }
  const EStyle getStyle() const
  {
    return eWireframeOnly;
  }
  void PushState()
  {
    m_state_stack.push_back(m_state_stack.back());
  }
  void PopState()
  {
    ASSERT_MESSAGE(!m_state_stack.empty(), "popping empty stack");
    m_state_stack.pop_back();
  }
  void Highlight(EHighlightMode mode, bool bEnable = true)
  {
    (bEnable)
      ? m_state_stack.back().m_highlight |= mode
      : m_state_stack.back().m_highlight &= ~mode;
  }
  void addRenderable(const OpenGLRenderable& renderable, const Matrix4& localToWorld)
  {
    if(m_state_stack.back().m_highlight & ePrimitive)
    {
      m_state_selected->addRenderable(renderable, localToWorld);
    }
    else
    {
      m_state_stack.back().m_state->addRenderable(renderable, localToWorld);
    }
  }

  void render(const Matrix4& modelview, const Matrix4& projection)
  {
    GlobalShaderCache().render(m_globalstate, modelview, projection);
  }
private:
  std::vector<state_type> m_state_stack;
  RenderStateFlags m_globalstate;
  Shader* m_state_selected;
};

void XYWnd::updateProjection()
{
  m_projection[0] = 1.0f / static_cast<float>(m_nWidth / 2);
  m_projection[5] = 1.0f / static_cast<float>(m_nHeight / 2);
  m_projection[10] = 1.0f / (_maxWorldCoord * m_fScale);

  m_projection[12] = 0.0f;
  m_projection[13] = 0.0f;
  m_projection[14] = -1.0f;

  m_projection[1] =
  m_projection[2] =
  m_projection[3] =

  m_projection[4] =
  m_projection[6] =
  m_projection[7] =

  m_projection[8] =
  m_projection[9] =
  m_projection[11] = 0.0f;

  m_projection[15] = 1.0f;

  m_view.Construct(m_projection, m_modelview, m_nWidth, m_nHeight);
}

// note: modelview matrix must have a uniform scale, otherwise strange things happen when rendering the rotation manipulator.
void XYWnd::updateModelview()
{
  int nDim1 = (m_viewType == YZ) ? 1 : 0;
  int nDim2 = (m_viewType == XY) ? 1 : 2;

  // translation
  m_modelview[12] = -m_vOrigin[nDim1] * m_fScale;
  m_modelview[13] = -m_vOrigin[nDim2] * m_fScale;
  m_modelview[14] = _maxWorldCoord * m_fScale;

  // axis base
  switch(m_viewType)
  {
  case XY:
    m_modelview[0]  =  m_fScale;
    m_modelview[1]  =  0;
    m_modelview[2]  =  0;

    m_modelview[4]  =  0;
    m_modelview[5]  =  m_fScale;
    m_modelview[6]  =  0;

    m_modelview[8]  =  0;
    m_modelview[9]  =  0;
    m_modelview[10] = -m_fScale;
    break;
  case XZ:
    m_modelview[0]  =  m_fScale;
    m_modelview[1]  =  0;
    m_modelview[2]  =  0;

    m_modelview[4]  =  0;
    m_modelview[5]  =  0;
    m_modelview[6]  =  m_fScale;

    m_modelview[8]  =  0;
    m_modelview[9]  =  m_fScale;
    m_modelview[10] =  0;
    break;
  case YZ:
    m_modelview[0]  =  0;
    m_modelview[1]  =  0;
    m_modelview[2]  = -m_fScale;

    m_modelview[4]  =  m_fScale;
    m_modelview[5]  =  0;
    m_modelview[6]  =  0;

    m_modelview[8]  =  0;
    m_modelview[9]  =  m_fScale;
    m_modelview[10] =  0;
    break;
  }

  m_modelview[3] = m_modelview[7] = m_modelview[11] = 0;
  m_modelview[15] = 1;

  m_view.Construct(m_projection, m_modelview, m_nWidth, m_nHeight);
}

/*
==============
XY_Draw
==============
*/

//#define DBG_SCENEDUMP

void XYWnd::XY_Draw()
{
  //
  // clear
  //
  glViewport(0, 0, m_nWidth, m_nHeight);
  Vector3 colourGridBack = ColourSchemes().getColourVector3("grid_background");
  glClearColor (colourGridBack[0], colourGridBack[1], colourGridBack[2], 0);

  glClear(GL_COLOR_BUFFER_BIT);

  //
  // set up viewpoint
  //

  glMatrixMode(GL_PROJECTION);
  glLoadMatrixf(reinterpret_cast<const float*>(&m_projection));

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glScalef(m_fScale, m_fScale, 1);
  int nDim1 = (m_viewType == YZ) ? 1 : 0;
  int nDim2 = (m_viewType == XY) ? 1 : 2;
  glTranslatef(-m_vOrigin[nDim1], -m_vOrigin[nDim2], 0);

  glDisable (GL_LINE_STIPPLE);
  glLineWidth(1);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glDisableClientState(GL_NORMAL_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_LIGHTING);
  glDisable(GL_COLOR_MATERIAL);
  glDisable(GL_DEPTH_TEST);

  XY_DrawGrid();
  if ( g_xywindow_globals_private.show_blocks)
    XY_DrawBlockGrid();

  glLoadMatrixf(reinterpret_cast<const float*>(&m_modelview));

  unsigned int globalstate = RENDER_COLOURARRAY | RENDER_COLOURWRITE | RENDER_POLYGONSMOOTH | RENDER_LINESMOOTH;
  if(!g_xywindow_globals.m_bNoStipple)
  {
    globalstate |= RENDER_LINESTIPPLE;
  }

  {
    XYRenderer renderer(globalstate, m_state_selected);

    Scene_Render(renderer, m_view);

    GlobalOpenGL_debugAssertNoErrors();
    renderer.render(m_modelview, m_projection);
    GlobalOpenGL_debugAssertNoErrors();
  }

  glDepthMask(GL_FALSE);

  GlobalOpenGL_debugAssertNoErrors();

  glLoadMatrixf(reinterpret_cast<const float*>(&m_modelview));
  
  GlobalOpenGL_debugAssertNoErrors();
  glDisable(GL_LINE_STIPPLE);
  GlobalOpenGL_debugAssertNoErrors();
  glLineWidth(1);
  GlobalOpenGL_debugAssertNoErrors();
  if(GLEW_VERSION_1_3)
  {
    glActiveTexture(GL_TEXTURE0);
    glClientActiveTexture(GL_TEXTURE0);
  }
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  GlobalOpenGL_debugAssertNoErrors();
  glDisableClientState(GL_NORMAL_ARRAY);
  GlobalOpenGL_debugAssertNoErrors();
  glDisableClientState(GL_COLOR_ARRAY);
  GlobalOpenGL_debugAssertNoErrors();
  glDisable(GL_TEXTURE_2D);
  GlobalOpenGL_debugAssertNoErrors();
  glDisable(GL_LIGHTING);
  GlobalOpenGL_debugAssertNoErrors();
  glDisable(GL_COLOR_MATERIAL);
  GlobalOpenGL_debugAssertNoErrors();

  GlobalOpenGL_debugAssertNoErrors();


  // greebo: Check, if the brush/patch size info should be displayed (if there are any items selected)
  if (GlobalRegistry().get("user/ui/showSizeInfo")=="1" && GlobalSelectionSystem().countSelected() != 0) {
    Vector3 min, max;
    Select_GetBounds(min, max);
    PaintSizeInfo(nDim1, nDim2, min, max);
  }

  if (g_bCrossHairs)
  {
    glColor4f(0.2f, 0.9f, 0.2f, 0.8f);
    glBegin (GL_LINES);
    if (m_viewType == XY)
    {
      glVertex2f(2.0f * _minWorldCoord, m_mousePosition[1]);
      glVertex2f(2.0f * _maxWorldCoord, m_mousePosition[1]);
      glVertex2f(m_mousePosition[0], 2.0f * _minWorldCoord);
      glVertex2f(m_mousePosition[0], 2.0f * _maxWorldCoord);
    }
    else if (m_viewType == YZ)
    {
      glVertex3f(m_mousePosition[0], 2.0f * _minWorldCoord, m_mousePosition[2]);
      glVertex3f(m_mousePosition[0], 2.0f * _maxWorldCoord, m_mousePosition[2]);
      glVertex3f(m_mousePosition[0], m_mousePosition[1], 2.0f * _minWorldCoord);
      glVertex3f(m_mousePosition[0], m_mousePosition[1], 2.0f * _maxWorldCoord);
    }
    else
    {
      glVertex3f (2.0f * _minWorldCoord, m_mousePosition[1], m_mousePosition[2]);
      glVertex3f (2.0f * _maxWorldCoord, m_mousePosition[1], m_mousePosition[2]);
      glVertex3f(m_mousePosition[0], m_mousePosition[1], 2.0f * _minWorldCoord);
      glVertex3f(m_mousePosition[0], m_mousePosition[1], 2.0f * _maxWorldCoord);
    }
    glEnd();
  }

  if (GlobalClipper().clipMode()) {
	GlobalClipper().draw(m_fScale);
  }

  GlobalOpenGL_debugAssertNoErrors();
    
    // reset modelview
  glLoadIdentity();
  glScalef(m_fScale, m_fScale, 1);
  glTranslatef(-m_vOrigin[nDim1], -m_vOrigin[nDim2], 0);

  DrawCameraIcon (g_pParentWnd->GetCamWnd()->getCameraOrigin(), g_pParentWnd->GetCamWnd()->getCameraAngles());

  if (g_xywindow_globals_private.show_outline)
  {
    if (Active())
    {
      glMatrixMode (GL_PROJECTION);
      glLoadIdentity();
      glOrtho (0, m_nWidth, 0, m_nHeight, 0, 1);

      glMatrixMode (GL_MODELVIEW);
      glLoadIdentity();

      // four view mode doesn't colorize
      if (g_pParentWnd->CurrentStyle() == MainFrame::eSplit) {
        glColor3fv(ColourSchemes().getColourVector3("active_view_name"));
      }
      else {
        switch(m_viewType) {
        	case YZ:
          		glColor3fv(ColourSchemes().getColourVector3("axis_x"));
          		break;
        	case XZ:
          		glColor3fv(ColourSchemes().getColourVector3("axis_y"));
          		break;
        	case XY:
          		glColor3fv(ColourSchemes().getColourVector3("axis_z"));
          		break;
        	}
      }
      
      glBegin (GL_LINE_LOOP);
      glVertex2i (0, 0);
      glVertex2i (m_nWidth-1, 0);
      glVertex2i (m_nWidth-1, m_nHeight-1);
      glVertex2i (0, m_nHeight-1);
      glEnd();
    }
  }

  GlobalOpenGL_debugAssertNoErrors();

  glFinish();
}

void XYWnd::mouseToPoint(int x, int y, Vector3& point) {
	XY_ToPoint(x, y, point);
	XY_SnapToGrid(point);

	int nDim = (GetViewType() == XY) ? 2 : (GetViewType() == YZ) ? 0 : 1;
	float fWorkMid = float_mid(Select_getWorkZone().d_work_min[nDim], Select_getWorkZone().d_work_max[nDim]);
	point[nDim] = float_snapped(fWorkMid, GetGridSize());
}


void XYWnd::OnEntityCreate (const std::string& item) {
	StringOutputStream command;
	command << "entityCreate -class " << item.c_str();
	UndoableCommand undo(command.c_str());
	Vector3 point;
	mouseToPoint(m_entityCreate_x, m_entityCreate_y, point);
	Entity_createFromSelection(item.c_str(), point);
}

void XYWnd::updateXORRectangle(Rectangle area) {
	if(GTK_WIDGET_VISIBLE(GetWidget())) {
		m_XORRectangle.set(rectangle_from_area(area.min, area.max, Width(), Height()));
	}
}

// NOTE: the zoom out factor is 4/5, we could think about customizing it
//  we don't go below a zoom factor corresponding to 10% of the max world size
//  (this has to be computed against the window size)
//void XYWnd_ZoomOut(XYWnd* xy);
void XYWnd::zoomOut() {
	float min_scale = MIN(Width(),Height()) / ( 1.1f * (_maxWorldCoord - _minWorldCoord));
	float scale = Scale() * 4.0f / 5.0f;
	if (scale < min_scale) {
		if (Scale() != min_scale) {
			SetScale (min_scale);
		}
	} else {
		SetScale(scale);
	}
}

void XYWnd::zoomIn() {
	float max_scale = 64;
	float scale = Scale() * 5.0f / 4.0f;
	
	if (scale > max_scale) {
		if (Scale() != max_scale) {
			SetScale (max_scale);
		}
	}
	else {
		SetScale(scale);
	}
}

void GetFocusPosition(Vector3& position)
{
  if(GlobalSelectionSystem().countSelected() != 0)
  {
    Select_GetMid(position);
  }
  else
  {
    position = g_pParentWnd->GetCamWnd()->getCameraOrigin();
  }
}

void XYWnd_Focus(XYWnd* xywnd)
{
  Vector3 position;
  GetFocusPosition(position);
  xywnd->PositionView(position);
}

void XY_Split_Focus()
{
  Vector3 position;
  GetFocusPosition(position);
  g_pParentWnd->GetXYWnd()->PositionView(position);
  g_pParentWnd->GetXZWnd()->PositionView(position);
  g_pParentWnd->GetYZWnd()->PositionView(position);
}

void XY_Focus()
{
  XYWnd* xywnd = g_pParentWnd->GetXYWnd();
  XYWnd_Focus(xywnd);
}

void XY_Top()
{
  XYWnd* xywnd = g_pParentWnd->GetXYWnd();
  xywnd->SetViewType(XY);
  XYWnd_Focus(xywnd);
}

void XY_Side()
{
  XYWnd* xywnd = g_pParentWnd->GetXYWnd();
  xywnd->SetViewType(XZ);
  XYWnd_Focus(xywnd);
}

void XY_Front()
{
  g_pParentWnd->GetXYWnd()->SetViewType(YZ);
  XYWnd_Focus(g_pParentWnd->GetXYWnd());
}

void XY_Next()
{
  XYWnd* xywnd = g_pParentWnd->GetXYWnd();
  if (xywnd->GetViewType() == XY)
    xywnd->SetViewType(XZ);
  else if (xywnd->GetViewType() ==  XZ)
    xywnd->SetViewType(YZ);
  else
    xywnd->SetViewType(XY);
  XYWnd_Focus(xywnd);
}

void XY_Zoom100()
{
  if (g_pParentWnd->GetXYWnd())
    g_pParentWnd->GetXYWnd()->SetScale(1);
  if (g_pParentWnd->GetXZWnd())
    g_pParentWnd->GetXZWnd()->SetScale(1);
  if (g_pParentWnd->GetYZWnd())
    g_pParentWnd->GetYZWnd()->SetScale(1);
}

void XY_ZoomIn() {
	g_pParentWnd->ActiveXY()->zoomIn();
}

// NOTE: the zoom out factor is 4/5, we could think about customizing it
//  we don't go below a zoom factor corresponding to 10% of the max world size
//  (this has to be computed against the window size)
void XY_ZoomOut() {
	g_pParentWnd->ActiveXY()->zoomOut();
}



void ToggleShowCrosshair()
{
  g_bCrossHairs ^= 1; 
  XY_UpdateAllWindows();
}

void ToggleShowGrid()
{
  g_xywindow_globals_private.d_showgrid = !g_xywindow_globals_private.d_showgrid;
  XY_UpdateAllWindows();
}

ToggleShown g_xy_top_shown(true);

void XY_Top_Shown_Construct(GtkWindow* parent)
{
  g_xy_top_shown.connect(GTK_WIDGET(parent));
}

ToggleShown g_yz_side_shown(false);

void YZ_Side_Shown_Construct(GtkWindow* parent)
{
  g_yz_side_shown.connect(GTK_WIDGET(parent));
}

ToggleShown g_xz_front_shown(false);

void XZ_Front_Shown_Construct(GtkWindow* parent)
{
  g_xz_front_shown.connect(GTK_WIDGET(parent));
}


class EntityClassMenu : public ModuleObserver
{
  std::size_t m_unrealised;
public:
  EntityClassMenu() : m_unrealised(1)
  {
  }
  void realise()
  {
    if(--m_unrealised == 0)
    {
    }
  }
  void unrealise()
  {
    if(++m_unrealised == 1)
    {
      if(XYWnd::m_mnuDrop != 0)
      {
        gtk_widget_destroy(GTK_WIDGET(XYWnd::m_mnuDrop));
        XYWnd::m_mnuDrop = 0;
      }
    }
  }
};

EntityClassMenu g_EntityClassMenu;




void ShowNamesToggle()
{
  GlobalEntityCreator().setShowNames(!GlobalEntityCreator().getShowNames());
  XY_UpdateAllWindows();
}
typedef FreeCaller<ShowNamesToggle> ShowNamesToggleCaller;
void ShowNamesExport(const BoolImportCallback& importer)
{
  importer(GlobalEntityCreator().getShowNames());
}
typedef FreeCaller1<const BoolImportCallback&, ShowNamesExport> ShowNamesExportCaller;

void ShowAnglesToggle()
{
  GlobalEntityCreator().setShowAngles(!GlobalEntityCreator().getShowAngles());
  XY_UpdateAllWindows();
}
typedef FreeCaller<ShowAnglesToggle> ShowAnglesToggleCaller;
void ShowAnglesExport(const BoolImportCallback& importer)
{
  importer(GlobalEntityCreator().getShowAngles());
}
typedef FreeCaller1<const BoolImportCallback&, ShowAnglesExport> ShowAnglesExportCaller;

void ShowBlocksToggle()
{
  g_xywindow_globals_private.show_blocks ^= 1;
  XY_UpdateAllWindows();
}
typedef FreeCaller<ShowBlocksToggle> ShowBlocksToggleCaller;
void ShowBlocksExport(const BoolImportCallback& importer)
{
  importer(g_xywindow_globals_private.show_blocks);
}
typedef FreeCaller1<const BoolImportCallback&, ShowBlocksExport> ShowBlocksExportCaller;

void ShowCoordinatesToggle()
{
  g_xywindow_globals_private.show_coordinates ^= 1;
  XY_UpdateAllWindows();
}
typedef FreeCaller<ShowCoordinatesToggle> ShowCoordinatesToggleCaller;
void ShowCoordinatesExport(const BoolImportCallback& importer)
{
  importer(g_xywindow_globals_private.show_coordinates);
}
typedef FreeCaller1<const BoolImportCallback&, ShowCoordinatesExport> ShowCoordinatesExportCaller;

void ShowOutlineToggle()
{
  g_xywindow_globals_private.show_outline ^= 1;
  XY_UpdateAllWindows();
}
typedef FreeCaller<ShowOutlineToggle> ShowOutlineToggleCaller;
void ShowOutlineExport(const BoolImportCallback& importer)
{
  importer(g_xywindow_globals_private.show_outline);
}
typedef FreeCaller1<const BoolImportCallback&, ShowOutlineExport> ShowOutlineExportCaller;

void ShowAxesToggle()
{
  g_xywindow_globals_private.show_axis ^= 1;
  XY_UpdateAllWindows();
}
typedef FreeCaller<ShowAxesToggle> ShowAxesToggleCaller;
void ShowAxesExport(const BoolImportCallback& importer)
{
  importer(g_xywindow_globals_private.show_axis);
}
typedef FreeCaller1<const BoolImportCallback&, ShowAxesExport> ShowAxesExportCaller;

void ShowWorkzoneToggle()
{
  g_xywindow_globals_private.d_show_work ^= 1;
  XY_UpdateAllWindows();
}
typedef FreeCaller<ShowWorkzoneToggle> ShowWorkzoneToggleCaller;
void ShowWorkzoneExport(const BoolImportCallback& importer)
{
  importer(g_xywindow_globals_private.d_show_work);
}
typedef FreeCaller1<const BoolImportCallback&, ShowWorkzoneExport> ShowWorkzoneExportCaller;

ShowNamesExportCaller g_show_names_caller;
BoolExportCallback g_show_names_callback(g_show_names_caller);
ToggleItem g_show_names(g_show_names_callback);

ShowAnglesExportCaller g_show_angles_caller;
BoolExportCallback g_show_angles_callback(g_show_angles_caller);
ToggleItem g_show_angles(g_show_angles_callback);

ShowBlocksExportCaller g_show_blocks_caller;
BoolExportCallback g_show_blocks_callback(g_show_blocks_caller);
ToggleItem g_show_blocks(g_show_blocks_callback);

ShowCoordinatesExportCaller g_show_coordinates_caller;
BoolExportCallback g_show_coordinates_callback(g_show_coordinates_caller);
ToggleItem g_show_coordinates(g_show_coordinates_callback);

ShowOutlineExportCaller g_show_outline_caller;
BoolExportCallback g_show_outline_callback(g_show_outline_caller);
ToggleItem g_show_outline(g_show_outline_callback);

ShowAxesExportCaller g_show_axes_caller;
BoolExportCallback g_show_axes_callback(g_show_axes_caller);
ToggleItem g_show_axes(g_show_axes_callback);

ShowWorkzoneExportCaller g_show_workzone_caller;
BoolExportCallback g_show_workzone_callback(g_show_workzone_caller);
ToggleItem g_show_workzone(g_show_workzone_callback);

void XYShow_registerCommands()
{
  GlobalToggles_insert("ShowAngles", ShowAnglesToggleCaller(), ToggleItem::AddCallbackCaller(g_show_angles));
  GlobalToggles_insert("ShowNames", ShowNamesToggleCaller(), ToggleItem::AddCallbackCaller(g_show_names));
  GlobalToggles_insert("ShowBlocks", ShowBlocksToggleCaller(), ToggleItem::AddCallbackCaller(g_show_blocks));
  GlobalToggles_insert("ShowCoordinates", ShowCoordinatesToggleCaller(), ToggleItem::AddCallbackCaller(g_show_coordinates));
  GlobalToggles_insert("ShowWindowOutline", ShowOutlineToggleCaller(), ToggleItem::AddCallbackCaller(g_show_outline));
  GlobalToggles_insert("ShowAxes", ShowAxesToggleCaller(), ToggleItem::AddCallbackCaller(g_show_axes));
  GlobalToggles_insert("ShowWorkzone", ShowWorkzoneToggleCaller(), ToggleItem::AddCallbackCaller(g_show_workzone));
}

void XYWnd_registerShortcuts()
{
  command_connect_accelerator("ToggleCrosshairs");
}



void Orthographic_constructPreferences(PrefPage* page)
{
  page->appendCheckBox("", "Solid selection boxes", g_xywindow_globals.m_bNoStipple);
  page->appendCheckBox("", "Chase mouse during drags", g_xywindow_globals_private.m_bChaseMouse);
  page->appendCheckBox("", "Update views on camera move", g_xywindow_globals_private.m_bCamXYUpdate);
}
void Orthographic_constructPage(PreferenceGroup& group)
{
  PreferencesPage* page(group.createPage("Orthographic", "Orthographic View Preferences"));
  Orthographic_constructPreferences(reinterpret_cast<PrefPage*>(page));
}
void Orthographic_registerPreferencesPage()
{
  PreferencesDialog_addSettingsPage(FreeCaller1<PreferenceGroup&, Orthographic_constructPage>());
}

#include "preferencesystem.h"
#include "stringio.h"




void ToggleShown_importBool(ToggleShown& self, bool value)
{
  self.set(value);
}
typedef ReferenceCaller1<ToggleShown, bool, ToggleShown_importBool> ToggleShownImportBoolCaller;
void ToggleShown_exportBool(const ToggleShown& self, const BoolImportCallback& importer)
{
  importer(self.active());
}
typedef ConstReferenceCaller1<ToggleShown, const BoolImportCallback&, ToggleShown_exportBool> ToggleShownExportBoolCaller;


void XYWindow_Construct()
{
  GlobalCommands_insert("ToggleCrosshairs", FreeCaller<ToggleShowCrosshair>(), Accelerator('X', (GdkModifierType)GDK_SHIFT_MASK));
  GlobalCommands_insert("ToggleGrid", FreeCaller<ToggleShowGrid>(), Accelerator('0'));

  GlobalToggles_insert("ToggleView", ToggleShown::ToggleCaller(g_xy_top_shown), ToggleItem::AddCallbackCaller(g_xy_top_shown.m_item), Accelerator('V', (GdkModifierType)(GDK_SHIFT_MASK|GDK_CONTROL_MASK)));
  GlobalToggles_insert("ToggleSideView", ToggleShown::ToggleCaller(g_yz_side_shown), ToggleItem::AddCallbackCaller(g_yz_side_shown.m_item));
  GlobalToggles_insert("ToggleFrontView", ToggleShown::ToggleCaller(g_xz_front_shown), ToggleItem::AddCallbackCaller(g_xz_front_shown.m_item));
  GlobalCommands_insert("NextView", FreeCaller<XY_Next>(), Accelerator(GDK_Tab, (GdkModifierType)GDK_CONTROL_MASK));
  GlobalCommands_insert("ZoomIn", FreeCaller<XY_ZoomIn>(), Accelerator(GDK_Delete));
  GlobalCommands_insert("ZoomOut", FreeCaller<XY_ZoomOut>(), Accelerator(GDK_Insert));
  GlobalCommands_insert("ViewTop", FreeCaller<XY_Top>());
  GlobalCommands_insert("ViewSide", FreeCaller<XY_Side>());
  GlobalCommands_insert("ViewFront", FreeCaller<XY_Front>());
  GlobalCommands_insert("Zoom100", FreeCaller<XY_Zoom100>());
  GlobalCommands_insert("CenterXYViews", FreeCaller<XY_Split_Focus>(), Accelerator(GDK_Tab, (GdkModifierType)(GDK_SHIFT_MASK|GDK_CONTROL_MASK)));
  GlobalCommands_insert("CenterXYView", FreeCaller<XY_Focus>(), Accelerator(GDK_Tab, (GdkModifierType)(GDK_SHIFT_MASK|GDK_CONTROL_MASK)));

  GlobalPreferenceSystem().registerPreference("ChaseMouse", BoolImportStringCaller(g_xywindow_globals_private.m_bChaseMouse), BoolExportStringCaller(g_xywindow_globals_private.m_bChaseMouse));
  GlobalPreferenceSystem().registerPreference("NoStipple", BoolImportStringCaller(g_xywindow_globals.m_bNoStipple), BoolExportStringCaller(g_xywindow_globals.m_bNoStipple));
  GlobalPreferenceSystem().registerPreference("SI_ShowCoords", BoolImportStringCaller(g_xywindow_globals_private.show_coordinates), BoolExportStringCaller(g_xywindow_globals_private.show_coordinates));
  GlobalPreferenceSystem().registerPreference("SI_ShowOutlines", BoolImportStringCaller(g_xywindow_globals_private.show_outline), BoolExportStringCaller(g_xywindow_globals_private.show_outline));
  GlobalPreferenceSystem().registerPreference("SI_ShowAxis", BoolImportStringCaller(g_xywindow_globals_private.show_axis), BoolExportStringCaller(g_xywindow_globals_private.show_axis));
  GlobalPreferenceSystem().registerPreference("CamXYUpdate", BoolImportStringCaller(g_xywindow_globals_private.m_bCamXYUpdate), BoolExportStringCaller(g_xywindow_globals_private.m_bCamXYUpdate));

  GlobalPreferenceSystem().registerPreference("XZVIS", makeBoolStringImportCallback(ToggleShownImportBoolCaller(g_xz_front_shown)), makeBoolStringExportCallback(ToggleShownExportBoolCaller(g_xz_front_shown)));
  GlobalPreferenceSystem().registerPreference("YZVIS", makeBoolStringImportCallback(ToggleShownImportBoolCaller(g_yz_side_shown)), makeBoolStringExportCallback(ToggleShownExportBoolCaller(g_yz_side_shown)));

  Orthographic_registerPreferencesPage();  

  XYWnd::captureStates();
  GlobalEntityClassManager().attach(g_EntityClassMenu);
}

void XYWindow_Destroy()
{
  GlobalEntityClassManager().detach(g_EntityClassMenu);
  XYWnd::releaseStates();
}
