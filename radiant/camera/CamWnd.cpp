#include "CamWnd.h"

#include "iclipper.h"
#include "iuimanager.h"
#include "ieventmanager.h"
#include "imainframe.h"

#include "gdk/gdkkeysyms.h"

#include "gtkutil/widget.h"
#include "gtkutil/GLWidgetSentry.h"
#include <time.h>
#include <boost/format.hpp>

#include "iselectable.h"
#include "selectionlib.h"
#include "map/Map.h"
#include "CamRenderer.h"
#include "CameraSettings.h"
#include "GlobalCamera.h"
#include "render/RenderStatistics.h"

#include <boost/bind.hpp>

class ObjectFinder :
	public scene::NodeVisitor
{
	scene::INodePtr _node;
	SelectionTest& _selectionTest;
	
	// To store the best intersection candidate
	SelectionIntersection _bestIntersection;
public:
	// Constructor
	ObjectFinder(SelectionTest& test) :
		_selectionTest(test)
	{}

	// Return the found node
	const scene::INodePtr& getNode() const {
		return _node;
	}
	
	// The visitor function
	bool pre(const scene::INodePtr& node) {
		// Check if the node is filtered
		if (node->visible()) {
			SelectionTestablePtr selectionTestable = Node_getSelectionTestable(node);
			
			if (selectionTestable != NULL) {
				bool occluded;
				OccludeSelector selector(_bestIntersection, occluded);
				selectionTestable->testSelect(selector, _selectionTest);
				
				if (occluded) {
					_node = node;
				}
			}
		}
		else {
			return false; // don't traverse filtered nodes
		}
			
		return true;
	}
};

inline WindowVector windowvector_for_widget_centre(GtkWidget* widget) {
	return WindowVector(static_cast<float>(widget->allocation.width / 2), static_cast<float>(widget->allocation.height / 2));
}

class FloorHeightWalker : 
	public scene::NodeVisitor
{
	float _current;
	float& _bestUp;
	float& _bestDown;

public:
	FloorHeightWalker(float current, float& bestUp, float& bestDown) :
		_current(current), 
		_bestUp(bestUp), 
		_bestDown(bestDown)
	{
		_bestUp = GlobalRegistry().getFloat("game/defaults/maxWorldCoord");
		_bestDown = -GlobalRegistry().getFloat("game/defaults/maxWorldCoord");
	}

	bool pre(const scene::INodePtr& node) {

		if (!node->visible()) return false; // don't traverse hidden nodes
		
		if (Node_isBrush(node)) // this node is a floor
		{
			const AABB& aabb = node->worldAABB();

			float floorHeight = aabb.origin.z() + aabb.extents.z();

			if (floorHeight > _current && floorHeight < _bestUp) {
				_bestUp = floorHeight;
			}

			if (floorHeight < _current && floorHeight > _bestDown) {
				_bestDown = floorHeight;
			}

			return false;
		}

		return true;
	}
};

// --------------- Callbacks ---------------------------------------------------------

void selection_motion(gdouble x, gdouble y, guint state, void* data) {
	reinterpret_cast<WindowObserver*>(data)->onMouseMotion(WindowVector(x, y), state);
}

gboolean camera_size_allocate(GtkWidget* widget, GtkAllocation* allocation, CamWnd* camwnd) {
	camwnd->getCamera().width = allocation->width;
	camwnd->getCamera().height = allocation->height;
	camwnd->getCamera().updateProjection();
	camwnd->m_window_observer->onSizeChanged(camwnd->getCamera().width, camwnd->getCamera().height);
	camwnd->queueDraw();
	return FALSE;
}

gboolean camera_expose(GtkWidget* widget, GdkEventExpose* event, gpointer data) {
	reinterpret_cast<CamWnd*>(data)->draw();
	return FALSE;
}

void Camera_motionDelta(int x, int y, unsigned int state, void* data) {
	Camera* cam = reinterpret_cast<Camera*>(data);

	cam->m_mouseMove.motion_delta(x, y, state);
	cam->m_strafe = GlobalEventManager().MouseEvents().strafeActive(state);

	if (cam->m_strafe) {
		cam->m_strafe_forward = GlobalEventManager().MouseEvents().strafeForwardActive(state);
	} else {
		cam->m_strafe_forward = false;
	}
}

// greebo: The GTK Callback during freemove mode for mouseDown. Passes the call on to the Windowobserver
gboolean selection_button_press_freemove(GtkWidget* widget, GdkEventButton* event, WindowObserver* observer) {
	// Check for the correct event type
	if (event->type == GDK_BUTTON_PRESS) {
		observer->onMouseDown(windowvector_for_widget_centre(widget), event);
	}
	return FALSE;
}

// greebo: The GTK Callback during freemove mode for mouseUp. Passes the call on to the Windowobserver
gboolean selection_button_release_freemove(GtkWidget* widget, GdkEventButton* event, WindowObserver* observer) {
	if (event->type == GDK_BUTTON_RELEASE) {
		observer->onMouseUp(windowvector_for_widget_centre(widget), event);
	}
	return FALSE;
}

// greebo: The GTK Callback during freemove mode for mouseMoved. Passes the call on to the Windowobserver
gboolean selection_motion_freemove(GtkWidget *widget, GdkEventMotion *event, WindowObserver* observer) {
	observer->onMouseMotion(windowvector_for_widget_centre(widget), event->state);
	return FALSE;
}

// greebo: The GTK Callback during freemove mode for scroll events.
gboolean wheelmove_scroll(GtkWidget* widget, GdkEventScroll* event, CamWnd* camwnd) {
	// Set the GTK focus to this widget
	gtk_widget_grab_focus(widget);

	// Determine the direction we are moving.
	if (event->direction == GDK_SCROLL_UP) {
		camwnd->getCamera().freemoveUpdateAxes();
		camwnd->setCameraOrigin(camwnd->getCameraOrigin() + camwnd->getCamera().forward * static_cast<float>(getCameraSettings()->movementSpeed()));
	}
	else if (event->direction == GDK_SCROLL_DOWN) {
		camwnd->getCamera().freemoveUpdateAxes();
		camwnd->setCameraOrigin(camwnd->getCameraOrigin() + camwnd->getCamera().forward * (-static_cast<float>(getCameraSettings()->movementSpeed())));
	}

	return FALSE;
}

/* greebo: GTK Callback: This gets called on "button_press_event" and basically just passes the call on 
 * to the according window observer. */
gboolean selection_button_press(GtkWidget* widget, GdkEventButton* event, WindowObserver* observer) {
	
	// Set the GTK focus to this widget
	gtk_widget_grab_focus(widget);

	// Check for the correct event type
	if (event->type == GDK_BUTTON_PRESS) {
		observer->onMouseDown(WindowVector(event->x, event->y), event);
	}
	return FALSE;
}

/* greebo: GTK Callback: This gets called on "button_release_event" and basically just passes the call on 
 * to the according window observer. */
gboolean selection_button_release(GtkWidget* widget, GdkEventButton* event, WindowObserver* observer) {
	if (event->type == GDK_BUTTON_RELEASE) {
		observer->onMouseUp(WindowVector(event->x, event->y), event);
	}
	return FALSE;
}

gboolean enable_freelook_button_press(GtkWidget* widget, GdkEventButton* event, CamWnd* camwnd) {
	if (event->type == GDK_BUTTON_PRESS) {
		
		if (GlobalEventManager().MouseEvents().stateMatchesCameraViewEvent(ui::camEnableFreeLookMode, event)) {
			camwnd->enableFreeMove();
			return TRUE;
		}
	}
	return FALSE;
}

gboolean disable_freelook_button_press(GtkWidget* widget, GdkEventButton* event, CamWnd* camwnd) {
	if (event->type == GDK_BUTTON_PRESS) {
		if (GlobalEventManager().MouseEvents().stateMatchesCameraViewEvent(ui::camDisableFreeLookMode, event)) {
			camwnd->disableFreeMove();
			return TRUE;
		}
	}
	return FALSE;
}

gboolean disable_freelook_button_release(GtkWidget* widget, GdkEventButton* event, CamWnd* camwnd) {
	if (event->type == GDK_BUTTON_RELEASE) {
		if (GlobalEventManager().MouseEvents().stateMatchesCameraViewEvent(ui::camDisableFreeLookMode, event)) {
			camwnd->disableFreeMove();
			return TRUE;
		}
	}
	return FALSE;
}

// ---------- CamWnd Implementation --------------------------------------------------

CamWnd::CamWnd() :
	_id(++_maxId),
	m_view(true),
	m_Camera(&m_view, CamWndQueueDraw(*this)),
	m_cameraview(m_Camera, &m_view, CamWndUpdate(*this)),
	m_drawing(false),
	m_bFreeMove(false),
	m_gl_widget(true, "CamWnd"),
	_parentWidget(NULL),
	m_window_observer(NewWindowObserver()),
	m_deferredDraw(boost::bind(widget_queue_draw, m_gl_widget)),
	m_deferred_motion(selection_motion, m_window_observer),
	m_selection_button_press_handler(0),
	m_selection_button_release_handler(0),
	m_selection_motion_handler(0),
	m_freelook_button_press_handler(0),
	m_freelook_button_release_handler(0)
{
	GtkWidget* glWidget = m_gl_widget;
	
	m_window_observer->setRectangleDrawCallback(boost::bind(&CamWnd::updateSelectionBox, this, _1));
	m_window_observer->setView(m_view);

	gtk_widget_set_events(glWidget, GDK_DESTROY | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);
	GTK_WIDGET_SET_FLAGS (glWidget, GTK_CAN_FOCUS);
	gtk_widget_set_size_request(glWidget, CAMWND_MINSIZE_X, CAMWND_MINSIZE_Y);

	g_object_set(m_gl_widget, "can-focus", TRUE, NULL);

	m_sizeHandler = g_signal_connect(G_OBJECT(glWidget), "size_allocate", G_CALLBACK(camera_size_allocate), this);
	m_exposeHandler = g_signal_connect(G_OBJECT(glWidget), "expose_event", G_CALLBACK(camera_expose), this);

	_mapValidHandle = GlobalMap().addValidCallback(boost::bind(&DeferredDraw::onMapValidChanged, &m_deferredDraw));

	// Deactivate all commands, just to make sure
	disableDiscreteMoveEvents();
	disableFreeMoveEvents();

	// Now add the handlers for the non-freelook mode, the events are activated by this
	addHandlersMove();

	g_signal_connect(G_OBJECT(glWidget), "scroll_event", G_CALLBACK(wheelmove_scroll), this);

	// Subscribe to the global scene graph update 
	GlobalSceneGraph().addSceneObserver(this);

	// Let the window observer connect its handlers to the GL widget first (before the eventmanager)
	m_window_observer->addObservedWidget(m_gl_widget);

	GlobalEventManager().connect(GTK_OBJECT(glWidget));
}

CamWnd::~CamWnd() {
	// Unsubscribe from the global scene graph update 
	GlobalSceneGraph().removeSceneObserver(this);
	
	GtkWidget* glWidget = m_gl_widget; // cast

	m_window_observer->removeObservedWidget(glWidget);
	
	// Disconnect self from EventManager
	GlobalEventManager().disconnect(GTK_OBJECT(glWidget));

	GlobalMap().removeValidCallback(_mapValidHandle);
	
	if (m_bFreeMove) {
		disableFreeMove();
	}

	removeHandlersMove();

	g_signal_handler_disconnect(G_OBJECT(glWidget), m_sizeHandler);
	g_signal_handler_disconnect(G_OBJECT(glWidget), m_exposeHandler);

	m_window_observer->release();

	// Notify the camera manager about our destruction
	GlobalCamera().removeCamWnd(_id);
}

int CamWnd::getId() {
	return _id;
}

void CamWnd::jumpToObject(SelectionTest& selectionTest) {
	// Find a suitable target node
	ObjectFinder finder(selectionTest);
	Node_traverseSubgraph(GlobalSceneGraph().root(), finder);

	if (finder.getNode() != NULL) {
		// A node has been found, get the bounding box
		AABB found = finder.getNode()->worldAABB();
		
		// Focus the view at the center of the found AABB
		map::Map::focusViews(found.origin, getCameraAngles());
	}
}

void CamWnd::updateSelectionBox(const Rectangle& area)
{
	if (GTK_WIDGET_VISIBLE(static_cast<GtkWidget*>(m_gl_widget)))
	{
		// Get the rectangle and convert it to screen coordinates
		_dragRectangle = area;
		_dragRectangle.toScreenCoords(m_Camera.width, m_Camera.height);

		queueDraw();
	}
}

void CamWnd::changeFloor(const bool up) {
	float current = m_Camera.getOrigin()[2] - 48;
	float bestUp;
	float bestDown;
	FloorHeightWalker walker(current, bestUp, bestDown);
	Node_traverseSubgraph(GlobalSceneGraph().root(), walker);

	if (up && bestUp != GlobalRegistry().getFloat("game/defaults/maxWorldCoord")) {
		current = bestUp;
	}

	if (!up && bestDown != -GlobalRegistry().getFloat("game/defaults/maxWorldCoord")) {
		current = bestDown;
	}

	const Vector3& org = m_Camera.getOrigin();
	m_Camera.setOrigin(Vector3(org[0], org[1], current + 48));

	m_Camera.updateModelview();
	update();
	GlobalCamera().movedNotify();
}

// NOTE TTimo if there's an OS-level focus out of the application
//   then we can release the camera cursor grab
static gboolean camwindow_freemove_focusout(GtkWidget* widget, GdkEventFocus* event, gpointer data) {
	reinterpret_cast<CamWnd*>(data)->disableFreeMove();
	return FALSE;
}

void CamWnd::enableFreeMove() {
	ASSERT_MESSAGE(!m_bFreeMove, "EnableFreeMove: free-move was already enabled");
	m_bFreeMove = true;
	m_Camera.clearMovementFlags(MOVE_ALL);

	removeHandlersMove();
	
	GtkWidget* glWidget = m_gl_widget; // cast
	
	m_selection_button_press_handler = g_signal_connect(G_OBJECT(glWidget), "button_press_event", G_CALLBACK(selection_button_press_freemove), m_window_observer);
	m_selection_button_release_handler = g_signal_connect(G_OBJECT(glWidget), "button_release_event", G_CALLBACK(selection_button_release_freemove), m_window_observer);
	m_selection_motion_handler = g_signal_connect(G_OBJECT(glWidget), "motion_notify_event", G_CALLBACK(selection_motion_freemove), m_window_observer);

	if (getCameraSettings()->toggleFreelook()) {
		m_freelook_button_press_handler = g_signal_connect(G_OBJECT(glWidget), "button_press_event", G_CALLBACK(disable_freelook_button_press), this);
	}
	else {
		m_freelook_button_release_handler = g_signal_connect(G_OBJECT(glWidget), "button-release-event", G_CALLBACK(disable_freelook_button_release), this);
	}

	enableFreeMoveEvents();

	// greebo: For entering free move, we need a valid parent window
	assert(_parentWidget != NULL);
	
	gtk_window_set_focus(_parentWidget, glWidget);
	m_freemove_handle_focusout = g_signal_connect(G_OBJECT(glWidget), "focus_out_event", G_CALLBACK(camwindow_freemove_focusout), this);
	m_freezePointer.freeze_pointer(_parentWidget, Camera_motionDelta, &m_Camera);

	update();
}

void CamWnd::disableFreeMove() {
	ASSERT_MESSAGE(m_bFreeMove, "DisableFreeMove: free-move was not enabled");
	m_bFreeMove = false;
	m_Camera.clearMovementFlags(MOVE_ALL);

	disableFreeMoveEvents();
	
	GtkWidget* glWidget = m_gl_widget; // cast

	g_signal_handler_disconnect(G_OBJECT(glWidget), m_selection_button_press_handler);
	g_signal_handler_disconnect(G_OBJECT(glWidget), m_selection_button_release_handler);
	g_signal_handler_disconnect(G_OBJECT(glWidget), m_selection_motion_handler);

	if (getCameraSettings()->toggleFreelook()) {
		g_signal_handler_disconnect(G_OBJECT(glWidget), m_freelook_button_press_handler);
	}
	else {
		g_signal_handler_disconnect(G_OBJECT(glWidget), m_freelook_button_release_handler);
	}	

	addHandlersMove();

	assert(_parentWidget != NULL);
	m_freezePointer.unfreeze_pointer(_parentWidget);
	g_signal_handler_disconnect(G_OBJECT(glWidget), m_freemove_handle_focusout);

	update();
}

bool CamWnd::freeMoveEnabled() const {
	return m_bFreeMove;
}

void CamWnd::Cam_Draw() {
	glViewport(0, 0, m_Camera.width, m_Camera.height);

	// enable depth buffer writes
	glDepthMask(GL_TRUE);

	Vector3 clearColour(0, 0, 0);

	if (getCameraSettings()->getMode() != drawLighting) {
		clearColour = ColourSchemes().getColour("camera_background");
	}

	glClearColor(clearColour[0], clearColour[1], clearColour[2], 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	render::RenderStatistics::Instance().resetStats();

	extern void Cull_ResetStats();

	Cull_ResetStats();

	glMatrixMode(GL_PROJECTION);

	glLoadMatrixd(m_Camera.projection);

	glMatrixMode(GL_MODELVIEW);

	glLoadMatrixd(m_Camera.modelview);


	// one directional light source directly behind the viewer
	{
		GLfloat inverse_cam_dir[4], ambient[4], diffuse[4];//, material[4];

		ambient[0] = ambient[1] = ambient[2] = 0.4f;
		ambient[3] = 1.0f;
		diffuse[0] = diffuse[1] = diffuse[2] = 0.4f;
		diffuse[3] = 1.0f;
		//material[0] = material[1] = material[2] = 0.8f;
		//material[3] = 1.0f;

		inverse_cam_dir[0] = m_Camera.vpn[0];
		inverse_cam_dir[1] = m_Camera.vpn[1];
		inverse_cam_dir[2] = m_Camera.vpn[2];
		inverse_cam_dir[3] = 0;

		glLightfv(GL_LIGHT0, GL_POSITION, inverse_cam_dir);

		glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

		glEnable(GL_LIGHT0);
	}


    // Set the allowed render flags for this view
	unsigned int allowedRenderFlags = RENDER_DEPTHTEST
                                     | RENDER_COLOURWRITE
                                     | RENDER_DEPTHWRITE
                                     | RENDER_ALPHATEST
                                     | RENDER_BLEND
                                     | RENDER_CULLFACE
                                     | RENDER_COLOURARRAY
                                     | RENDER_OFFSETLINE
                                     | RENDER_POLYGONSMOOTH
                                     | RENDER_LINESMOOTH
                                     | RENDER_COLOURCHANGE;

    // Add mode-specific render flags
	switch (getCameraSettings()->getMode()) 
    {
		case drawWire:
			break;

		case drawSolid:
			allowedRenderFlags |= RENDER_FILL
			               | RENDER_LIGHTING
			               | RENDER_SMOOTH
			               | RENDER_SCALED;

			break;

		case drawTexture:
			allowedRenderFlags |= RENDER_FILL
			               | RENDER_LIGHTING
			               | RENDER_TEXTURE_2D
			               | RENDER_SMOOTH
			               | RENDER_SCALED;

			break;

		case drawLighting:
			allowedRenderFlags |= RENDER_FILL
			               | RENDER_LIGHTING
			               | RENDER_TEXTURE_2D
                           | RENDER_TEXTURE_CUBEMAP
			               | RENDER_SMOOTH
			               | RENDER_SCALED
			               | RENDER_BUMP
			               | RENDER_PROGRAM
                           | RENDER_MATERIAL_VCOL
                           | RENDER_VCOL_INVERT
			               | RENDER_SCREEN;

			break;

		default:
			allowedRenderFlags = 0;

			break;
	}

	if (!getCameraSettings()->solidSelectionBoxes()) {
		allowedRenderFlags |= RENDER_LINESTIPPLE|RENDER_POLYGONSTIPPLE;
	}

	{
		CamRenderer renderer(allowedRenderFlags, m_state_select2, m_state_select1, m_view.getViewer());

		render::collectRenderablesInScene(renderer, m_view);

		renderer.render(m_Camera.modelview, m_Camera.projection);
	}
	
	// greebo: Draw the clipper's points (skipping the depth-test)
	{
		glDisable(GL_DEPTH_TEST);
		
		glColor4f(1, 1, 1, 1);
		glPointSize(5);
		
		if (GlobalClipper().clipMode()) {
			GlobalClipper().draw(1.0f);
		}
		
		glPointSize(1);
	}
	
	// prepare for 2d stuff
	glColor4f(1, 1, 1, 1);

	glDisable(GL_BLEND);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, (float)m_Camera.width, 0, (float)m_Camera.height, -100, 100);

	glScalef(1, -1, 1);
	glTranslatef(0, -(float)m_Camera.height, 0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (GLEW_VERSION_1_3) {
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

	// draw the crosshair

	if (m_bFreeMove) {
		glBegin( GL_LINES );
		glVertex2f( (float)m_Camera.width / 2.f, (float)m_Camera.height / 2.f + 6 );
		glVertex2f( (float)m_Camera.width / 2.f, (float)m_Camera.height / 2.f + 2 );
		glVertex2f( (float)m_Camera.width / 2.f, (float)m_Camera.height / 2.f - 6 );
		glVertex2f( (float)m_Camera.width / 2.f, (float)m_Camera.height / 2.f - 2 );
		glVertex2f( (float)m_Camera.width / 2.f + 6, (float)m_Camera.height / 2.f );
		glVertex2f( (float)m_Camera.width / 2.f + 2, (float)m_Camera.height / 2.f );
		glVertex2f( (float)m_Camera.width / 2.f - 6, (float)m_Camera.height / 2.f );
		glVertex2f( (float)m_Camera.width / 2.f - 2, (float)m_Camera.height / 2.f );
		glEnd();
	}

	glRasterPos3f(1.0f, static_cast<float>(m_Camera.height) - 1.0f, 0.0f);

	GlobalOpenGL().drawString(render::RenderStatistics::Instance().getStatString());

	glRasterPos3f(1.0f, static_cast<float>(m_Camera.height) - 11.0f, 0.0f);

	extern const char* Cull_GetStats();

	GlobalOpenGL().drawString(Cull_GetStats());

	// Draw the selection drag rectangle
	if (!_dragRectangle.empty())
	{
		// Define the blend function for transparency
		glEnable(GL_BLEND);
		glBlendColor(0, 0, 0, 0.2f);
		glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);
		
		Vector3 dragBoxColour = ColourSchemes().getColour("drag_selection");
		glColor3dv(dragBoxColour);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// Correct the glScale and glTranslate calls above
		Rectangle rect = _dragRectangle;

		double width = rect.max.x() - rect.min.x();
		double height = rect.max.y() - rect.min.y();

		rect.min.y() = m_Camera.height - rect.min.y();
		height *= -1;

		// The transparent fill rectangle
		glBegin(GL_QUADS);
		glVertex2d(rect.min.x(), rect.min.y() + height);
		glVertex2d(rect.min.x() + width, rect.min.y() + height);
		glVertex2d(rect.min.x() + width, rect.min.y());
		glVertex2d(rect.min.x(), rect.min.y());
		glEnd();

		// The solid borders
		glColor3f(0.9f, 0.9f, 0.9f);
		glBlendColor(0, 0, 0, 0.8f);

		glBegin(GL_LINE_LOOP);
		glVertex2d(rect.min.x(), rect.min.y() + height);
		glVertex2d(rect.min.x() + width, rect.min.y() + height);
		glVertex2d(rect.min.x() + width, rect.min.y());
		glVertex2d(rect.min.x(), rect.min.y());
		glEnd();

		glDisable(GL_BLEND);
	}

	// bind back to the default texture so that we don't have problems
	// elsewhere using/modifying texture maps between contexts
	glBindTexture( GL_TEXTURE_2D, 0 );
}

void CamWnd::draw() {
	m_drawing = true;

	// Scoped object handling the GL context switching
	gtkutil::GLWidgetSentry sentry(m_gl_widget);

	if (GlobalMap().isValid() && GlobalMainFrame().screenUpdatesEnabled()) {
		GlobalOpenGL_debugAssertNoErrors();
		Cam_Draw();
		GlobalOpenGL_debugAssertNoErrors();
	}

	m_drawing = false;
}

void CamWnd::benchmark() {
	double dStart = clock() / 1000.0;

	for (int i=0 ; i < 100 ; i++) {
		Vector3 angles;
		angles[CAMERA_ROLL] = 0;
		angles[CAMERA_PITCH] = 0;
		angles[CAMERA_YAW] = static_cast<double>(i * (360.0 / 100.0));
		setCameraAngles(angles);
	}

	double dEnd = clock() / 1000.0;

	globalOutputStream() << (boost::format("%5.2lf") % (dEnd - dStart)) << " seconds\n";
}

void CamWnd::onSceneGraphChange() {
	// Just pass the call to the update method
	update();
}

// ----------------------------------------------------------

void CamWnd::enableFreeMoveEvents() {
	GlobalEventManager().enableEvent("CameraFreeMoveForward");
	GlobalEventManager().enableEvent("CameraFreeMoveBack");
	GlobalEventManager().enableEvent("CameraFreeMoveLeft");
	GlobalEventManager().enableEvent("CameraFreeMoveRight");
	GlobalEventManager().enableEvent("CameraFreeMoveUp");
	GlobalEventManager().enableEvent("CameraFreeMoveDown");
}

void CamWnd::disableFreeMoveEvents() {
	GlobalEventManager().disableEvent("CameraFreeMoveForward");
	GlobalEventManager().disableEvent("CameraFreeMoveBack");
	GlobalEventManager().disableEvent("CameraFreeMoveLeft");
	GlobalEventManager().disableEvent("CameraFreeMoveRight");
	GlobalEventManager().disableEvent("CameraFreeMoveUp");
	GlobalEventManager().disableEvent("CameraFreeMoveDown");
}

void CamWnd::enableDiscreteMoveEvents() {
	GlobalEventManager().enableEvent("CameraForward");
	GlobalEventManager().enableEvent("CameraBack");
	GlobalEventManager().enableEvent("CameraLeft");
	GlobalEventManager().enableEvent("CameraRight");
	GlobalEventManager().enableEvent("CameraStrafeRight");
	GlobalEventManager().enableEvent("CameraStrafeLeft");
	GlobalEventManager().enableEvent("CameraUp");
	GlobalEventManager().enableEvent("CameraDown");
	GlobalEventManager().enableEvent("CameraAngleUp");
	GlobalEventManager().enableEvent("CameraAngleDown");
}

void CamWnd::disableDiscreteMoveEvents() {
	GlobalEventManager().disableEvent("CameraForward");
	GlobalEventManager().disableEvent("CameraBack");
	GlobalEventManager().disableEvent("CameraLeft");
	GlobalEventManager().disableEvent("CameraRight");
	GlobalEventManager().disableEvent("CameraStrafeRight");
	GlobalEventManager().disableEvent("CameraStrafeLeft");
	GlobalEventManager().disableEvent("CameraUp");
	GlobalEventManager().disableEvent("CameraDown");
	GlobalEventManager().disableEvent("CameraAngleUp");
	GlobalEventManager().disableEvent("CameraAngleDown");
}

void CamWnd::addHandlersMove() {
	GtkWidget* glWidget = m_gl_widget;
	
	m_selection_button_press_handler = g_signal_connect(G_OBJECT(glWidget), "button_press_event", G_CALLBACK(selection_button_press), m_window_observer);
	m_selection_button_release_handler = g_signal_connect(G_OBJECT(glWidget), "button_release_event", G_CALLBACK(selection_button_release), m_window_observer);
	m_selection_motion_handler = g_signal_connect(G_OBJECT(glWidget), "motion_notify_event", G_CALLBACK(DeferredMotion::gtk_motion), &m_deferred_motion);

	m_freelook_button_press_handler = g_signal_connect(G_OBJECT(glWidget), "button_press_event", G_CALLBACK(enable_freelook_button_press), this);

	// Enable either the free-look movement commands or the discrete ones, depending on the selection
	if (getCameraSettings()->discreteMovement()) {
		enableDiscreteMoveEvents();
	} else {
		enableFreeMoveEvents();
	}
}

void CamWnd::removeHandlersMove() {
	GtkWidget* glWidget = m_gl_widget; // cast to GtkWidget*
	
	g_signal_handler_disconnect(G_OBJECT(glWidget), m_selection_button_press_handler);
	g_signal_handler_disconnect(G_OBJECT(glWidget), m_selection_button_release_handler);
	g_signal_handler_disconnect(G_OBJECT(glWidget), m_selection_motion_handler);

	g_signal_handler_disconnect(G_OBJECT(glWidget), m_freelook_button_press_handler);

	// Disable either the free-look movement commands or the discrete ones, depending on the selection
	if (getCameraSettings()->discreteMovement()) {
		disableDiscreteMoveEvents();
	} else {
		disableFreeMoveEvents();
	}
}

void CamWnd::update() {
	queueDraw();
}

Camera& CamWnd::getCamera() {
	return m_Camera;
}

void CamWnd::captureStates() {
	m_state_select1 = GlobalRenderSystem().capture("$CAM_HIGHLIGHT");
	m_state_select2 = GlobalRenderSystem().capture("$CAM_OVERLAY");
}

void CamWnd::releaseStates() {
	m_state_select1 = ShaderPtr();
	m_state_select2 = ShaderPtr();
}

void CamWnd::queueDraw() {
	if (m_drawing) {
		return;
	}

	m_deferredDraw.draw();
}

CameraView* CamWnd::getCameraView() {
	return &m_cameraview;
}

GtkWidget* CamWnd::getWidget() const {
	return m_gl_widget;
}

GtkWindow* CamWnd::getParent() const {
	return _parentWidget;
}

void CamWnd::setContainer(GtkWindow* newParent) {
	if (newParent == _parentWidget) {
		// Do nothing if no change required
		return;
	}

	if (_parentWidget != NULL)
	{
		// Parent change, disconnect first
		m_window_observer->removeObservedWidget(GTK_WIDGET(_parentWidget));
		GlobalEventManager().disconnect(GTK_OBJECT(_parentWidget));

		if (m_bFreeMove)
		{
			disableFreeMove();
		}

		_parentWidget = NULL;
	}

	if (newParent != NULL)
	{
		_parentWidget = newParent;
		m_window_observer->addObservedWidget(GTK_WIDGET(_parentWidget));
		GlobalEventManager().connect(GTK_OBJECT(_parentWidget));
	}
}

Vector3 CamWnd::getCameraOrigin() const {
	return m_Camera.getOrigin();
}

void CamWnd::setCameraOrigin(const Vector3& origin) {
	m_Camera.setOrigin(origin);
}

Vector3 CamWnd::getCameraAngles() const {
	return m_Camera.getAngles();
}

void CamWnd::setCameraAngles(const Vector3& angles) {
	m_Camera.setAngles(angles);
}

void CamWnd::cubicScaleOut() {
	getCameraSettings()->setCubicScale( getCameraSettings()->cubicScale() + 1 );

	m_Camera.updateProjection();
	update();
}

void CamWnd::cubicScaleIn() {
	getCameraSettings()->setCubicScale( getCameraSettings()->cubicScale() - 1 );

	m_Camera.updateProjection();
	update();
}

// -------------------------------------------------------------------------------

ShaderPtr CamWnd::m_state_select1;
ShaderPtr CamWnd::m_state_select2;
int CamWnd::_maxId = 0;
