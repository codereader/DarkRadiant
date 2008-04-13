#include "CamWnd.h"

#include "iclipper.h"
#include "iuimanager.h"
#include "ieventmanager.h"

#include "gdk/gdkkeysyms.h"

#include "gtkutil/widget.h"
#include "gtkutil/GLWidgetSentry.h"
#include <time.h>

#include "selectable.h"
#include "selectionlib.h"
#include "windowobservers.h"
#include "mainframe.h"
#include "map/Map.h"
#include "CamRenderer.h"
#include "CameraSettings.h"
#include "GlobalCamera.h"
#include "render/RenderStatistics.h"

class ObjectFinder :
	public scene::Graph::Walker
{
	mutable scene::INodePtr _node;
	SelectionTest& _selectionTest;
	
	// To store the best intersection candidate
	mutable SelectionIntersection _bestIntersection;
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
	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
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
			
		return true;
	}
};

inline WindowVector windowvector_for_widget_centre(GtkWidget* widget) {
	return WindowVector(static_cast<float>(widget->allocation.width / 2), static_cast<float>(widget->allocation.height / 2));
}

class FloorHeightWalker : public scene::Graph::Walker
{
	float m_current;
	float& m_bestUp;
	float& m_bestDown;

public:
	FloorHeightWalker(float current, float& bestUp, float& bestDown) :
			m_current(current), m_bestUp(bestUp), m_bestDown(bestDown) {
		bestUp = GlobalRegistry().getFloat("game/defaults/maxWorldCoord");
		bestDown = -GlobalRegistry().getFloat("game/defaults/maxWorldCoord");
	}

	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		
		if (node->visible() && Node_isBrush(node)) // this node is a floor
		{
			const AABB& aabb = node->worldAABB();

			float floorHeight = aabb.origin.z() + aabb.extents.z();

			if (floorHeight > m_current && floorHeight < m_bestUp) {
				m_bestUp = floorHeight;
			}

			if (floorHeight < m_current && floorHeight > m_bestDown) {
				m_bestDown = floorHeight;
			}
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
	m_view(true),
	m_Camera(&m_view, CamWndQueueDraw(*this)),
	m_cameraview(m_Camera, &m_view, CamWndUpdate(*this)),
	m_drawing(false),
	m_bFreeMove(false),
	m_gl_widget(true),
	_parentWidget(NULL),
	m_window_observer(NewWindowObserver()),
	m_XORRectangle(m_gl_widget),
	m_deferredDraw(WidgetQueueDrawCaller(*m_gl_widget)),
	m_deferred_motion(selection_motion, m_window_observer),
	m_selection_button_press_handler(0),
	m_selection_button_release_handler(0),
	m_selection_motion_handler(0),
	m_freelook_button_press_handler(0),
	m_freelook_button_release_handler(0)
{
	GtkWidget* glWidget = m_gl_widget;
	GlobalWindowObservers_add(m_window_observer);
	GlobalWindowObservers_connectWidget(glWidget);

	m_window_observer->setRectangleDrawCallback(updateXORRectangleCallback(*this));
	m_window_observer->setView(m_view);

	gtk_widget_ref(glWidget);

	gtk_widget_set_events(glWidget, GDK_DESTROY | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);
	GTK_WIDGET_SET_FLAGS (glWidget, GTK_CAN_FOCUS);
	gtk_widget_set_size_request(glWidget, CAMWND_MINSIZE_X, CAMWND_MINSIZE_Y);

	m_sizeHandler = g_signal_connect(G_OBJECT(glWidget), "size_allocate", G_CALLBACK(camera_size_allocate), this);
	m_exposeHandler = g_signal_connect(G_OBJECT(glWidget), "expose_event", G_CALLBACK(camera_expose), this);

	GlobalMap().addValidCallback(DeferredDrawOnMapValidChangedCaller(m_deferredDraw));

	// Deactivate all commands, just to make sure
	disableDiscreteMoveEvents();
	disableFreeMoveEvents();

	// Now add the handlers for the non-freelook mode, the events are activated by this
	addHandlersMove();

	g_signal_connect(G_OBJECT(glWidget), "scroll_event", G_CALLBACK(wheelmove_scroll), this);

	// Subscribe to the global scene graph update 
	GlobalSceneGraph().addSceneObserver(this);

	GlobalEventManager().connect(GTK_OBJECT(glWidget));
}

void CamWnd::jumpToObject(SelectionTest& selectionTest) {
	// Find a suitable target node
	ObjectFinder finder(selectionTest);
	GlobalSceneGraph().traverse(finder);

	if (finder.getNode() != NULL) {
		// A node has been found, get the bounding box
		AABB found = finder.getNode()->worldAABB();
		
		// Focus the view at the center of the found AABB
		map::Map::focusViews(found.origin, getCameraAngles());
	}
}

void CamWnd::updateXORRectangle(Rectangle area) {
	if (GTK_WIDGET_VISIBLE(static_cast<GtkWidget*>(m_gl_widget))) {
		m_XORRectangle.set(rectangle_from_area(area.min, area.max, m_Camera.width, m_Camera.height));
	}
}

void CamWnd::changeFloor(const bool up) {
	float current = m_Camera.origin[2] - 48;
	float bestUp;
	float bestDown;
	GlobalSceneGraph().traverse(FloorHeightWalker(current, bestUp, bestDown));

	if (up && bestUp != GlobalRegistry().getFloat("game/defaults/maxWorldCoord")) {
		current = bestUp;
	}

	if (!up && bestDown != -GlobalRegistry().getFloat("game/defaults/maxWorldCoord")) {
		current = bestDown;
	}

	m_Camera.origin[2] = current + 48;

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
#if 0
	GLint viewprt[4];
	glGetIntegerv (GL_VIEWPORT, viewprt);
#endif

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


	unsigned int globalstate = RENDER_DEPTHTEST|RENDER_COLOURWRITE|RENDER_DEPTHWRITE|RENDER_ALPHATEST|RENDER_BLEND|RENDER_CULLFACE|RENDER_COLOURARRAY|RENDER_OFFSETLINE|RENDER_POLYGONSMOOTH|RENDER_LINESMOOTH|RENDER_FOG|RENDER_COLOURCHANGE;

	switch (getCameraSettings()->getMode()) {

		case drawWire:
			break;

		case drawSolid:
			globalstate |= RENDER_FILL
			               | RENDER_LIGHTING
			               | RENDER_SMOOTH
			               | RENDER_SCALED;

			break;

		case drawTexture:
			globalstate |= RENDER_FILL
			               | RENDER_LIGHTING
			               | RENDER_TEXTURE
			               | RENDER_SMOOTH
			               | RENDER_SCALED;

			break;

		case drawLighting:
			globalstate |= RENDER_FILL
			               | RENDER_LIGHTING
			               | RENDER_TEXTURE
			               | RENDER_SMOOTH
			               | RENDER_SCALED
			               | RENDER_BUMP
			               | RENDER_PROGRAM
			               | RENDER_SCREEN;

			break;

		default:
			globalstate = 0;

			break;
	}

	if (!getCameraSettings()->solidSelectionBoxes()) {
		globalstate |= RENDER_LINESTIPPLE|RENDER_POLYGONSTIPPLE;
	}

	{
		CamRenderer renderer(globalstate, m_state_select2, m_state_select1, m_view.getViewer());

		Scene_Render(renderer, m_view);

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

	// bind back to the default texture so that we don't have problems
	// elsewhere using/modifying texture maps between contexts
	glBindTexture( GL_TEXTURE_2D, 0 );
}

void CamWnd::draw() {
	m_drawing = true;

	// Scoped object handling the GL context switching
	gtkutil::GLWidgetSentry sentry(m_gl_widget);

	if (GlobalMap().isValid() && ScreenUpdates_Enabled()) {
		GlobalOpenGL_debugAssertNoErrors();
		Cam_Draw();
		GlobalOpenGL_debugAssertNoErrors();

		m_XORRectangle.set(rectangle_t());
	}

	m_drawing = false;
}

void CamWnd::benchmark() {
	double dStart = clock()/ 1000.0;

	for (int i=0 ; i < 100 ; i++) {
		Vector3 angles;
		angles[CAMERA_ROLL] = 0;
		angles[CAMERA_PITCH] = 0;
		angles[CAMERA_YAW] = static_cast<double>(i * (360.0 / 100.0));
		setCameraAngles(angles);
	}

	double dEnd = clock()/ 1000.0;

	globalOutputStream() << FloatFormat(dEnd - dStart, 5, 2) << " seconds\n";
}

void CamWnd::onSceneGraphChange() {
	// Just pass the call to the update method
	update();
}

CamWnd::~CamWnd() {
	// Subscribe to the global scene graph update 
	GlobalSceneGraph().removeSceneObserver(this);
	
	GtkWidget* glWidget = m_gl_widget; // cast
	
	// Disconnect self from EventManager
	GlobalEventManager().disconnect(GTK_OBJECT(glWidget));
	if (_parentWidget != NULL) {
		GlobalEventManager().disconnect(GTK_OBJECT(_parentWidget));
	}
	
	if (m_bFreeMove) {
		disableFreeMove();
	}

	removeHandlersMove();

	g_signal_handler_disconnect(G_OBJECT(glWidget), m_sizeHandler);
	g_signal_handler_disconnect(G_OBJECT(glWidget), m_exposeHandler);

	gtk_widget_unref(glWidget);

	m_window_observer->release();
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
	m_state_select1 = GlobalShaderCache().capture("$CAM_HIGHLIGHT");
	m_state_select2 = GlobalShaderCache().capture("$CAM_OVERLAY");
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
	_parentWidget = newParent;
	GlobalEventManager().connect(GTK_OBJECT(_parentWidget));
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
	g_pParentWnd->SetGridStatus();
}

void CamWnd::cubicScaleIn() {
	getCameraSettings()->setCubicScale( getCameraSettings()->cubicScale() - 1 );

	m_Camera.updateProjection();
	update();
	
	g_pParentWnd->SetGridStatus();
}

// -------------------------------------------------------------------------------

ShaderPtr CamWnd::m_state_select1;
ShaderPtr CamWnd::m_state_select2;
