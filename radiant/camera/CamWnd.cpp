#include "CamWnd.h"

#include "iscenegraph.h"

#include "gdk/gdkkeysyms.h"

#include "gtkutil/glwidget.h"
#include "gtkutil/widget.h"
#include "cmdlib.h"

#include "commands.h"
#include "windowobservers.h"
#include "plugin.h"
#include "xywindow.h"
#include "mainframe.h"
#include "renderstate.h"

#include "CamRenderer.h"
#include "CameraSettings.h"

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
		bestUp = g_MaxWorldCoord;
		bestDown = -g_MaxWorldCoord;
	}

	bool pre(const scene::Path& path, scene::Instance& instance) const {
		if (path.top().get().visible() && Node_isBrush(path.top())) // this node is a floor
		{

			const AABB& aabb = instance.worldAABB();

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
	//globalOutputStream() << "motion... ";
	reinterpret_cast<WindowObserver*>(data)->onMouseMotion(WindowVector(x, y), state);
}

void camwnd_update_xor_rectangle(CamWnd& self, Rectangle area) {
	if (GTK_WIDGET_VISIBLE(self.m_gl_widget)) {
		self.m_XORRectangle.set(rectangle_from_area(area.min, area.max, self.getCamera().width, self.getCamera().height));
	}
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

Signal0 g_cameraMoved_callbacks;

void AddCameraMovedCallback(const SignalHandler& handler) {
	g_cameraMoved_callbacks.connectLast(handler);
}

void CameraMovedNotify() {

	g_cameraMoved_callbacks();
}

camwindow_globals_private_t g_camwindow_globals_private;

void Camera_MoveForward_KeyDown(Camera& camera) {
	camera.setMovementFlags(MOVE_FORWARD);
}

void Camera_MoveForward_KeyUp(Camera& camera) {
	camera.clearMovementFlags(MOVE_FORWARD);
}

void Camera_MoveBack_KeyDown(Camera& camera) {
	camera.setMovementFlags(MOVE_BACK);
}

void Camera_MoveBack_KeyUp(Camera& camera) {
	camera.clearMovementFlags(MOVE_BACK);
}

void Camera_MoveLeft_KeyDown(Camera& camera) {
	camera.setMovementFlags(MOVE_STRAFELEFT);
}

void Camera_MoveLeft_KeyUp(Camera& camera) {
	camera.clearMovementFlags(MOVE_STRAFELEFT);
}

void Camera_MoveRight_KeyDown(Camera& camera) {
	camera.setMovementFlags(MOVE_STRAFERIGHT);
}

void Camera_MoveRight_KeyUp(Camera& camera) {
	camera.clearMovementFlags(MOVE_STRAFERIGHT);
}

void Camera_MoveUp_KeyDown(Camera& camera) {
	camera.setMovementFlags(MOVE_UP);
}

void Camera_MoveUp_KeyUp(Camera& camera) {
	camera.clearMovementFlags(MOVE_UP);
}

void Camera_MoveDown_KeyDown(Camera& camera) {
	camera.setMovementFlags(MOVE_DOWN);
}

void Camera_MoveDown_KeyUp(Camera& camera) {
	camera.clearMovementFlags(MOVE_DOWN);
}

void Camera_RotateLeft_KeyDown(Camera& camera) {
	camera.setMovementFlags(MOVE_ROTLEFT);
}

void Camera_RotateLeft_KeyUp(Camera& camera) {
	camera.clearMovementFlags(MOVE_ROTLEFT);
}

void Camera_RotateRight_KeyDown(Camera& camera) {
	camera.setMovementFlags(MOVE_ROTRIGHT);
}

void Camera_RotateRight_KeyUp(Camera& camera) {
	camera.clearMovementFlags(MOVE_ROTRIGHT);
}

void Camera_PitchUp_KeyDown(Camera& camera) {
	camera.setMovementFlags(MOVE_PITCHUP);
}

void Camera_PitchUp_KeyUp(Camera& camera) {
	camera.clearMovementFlags(MOVE_PITCHUP);
}

void Camera_PitchDown_KeyDown(Camera& camera) {
	camera.setMovementFlags(MOVE_PITCHDOWN);
}

void Camera_PitchDown_KeyUp(Camera& camera) {
	camera.clearMovementFlags(MOVE_PITCHDOWN);
}

typedef ReferenceCaller<Camera, &Camera_MoveForward_KeyDown> FreeMoveCameraMoveForwardKeyDownCaller;
typedef ReferenceCaller<Camera, &Camera_MoveForward_KeyUp> FreeMoveCameraMoveForwardKeyUpCaller;
typedef ReferenceCaller<Camera, &Camera_MoveBack_KeyDown> FreeMoveCameraMoveBackKeyDownCaller;
typedef ReferenceCaller<Camera, &Camera_MoveBack_KeyUp> FreeMoveCameraMoveBackKeyUpCaller;
typedef ReferenceCaller<Camera, &Camera_MoveLeft_KeyDown> FreeMoveCameraMoveLeftKeyDownCaller;
typedef ReferenceCaller<Camera, &Camera_MoveLeft_KeyUp> FreeMoveCameraMoveLeftKeyUpCaller;
typedef ReferenceCaller<Camera, &Camera_MoveRight_KeyDown> FreeMoveCameraMoveRightKeyDownCaller;
typedef ReferenceCaller<Camera, &Camera_MoveRight_KeyUp> FreeMoveCameraMoveRightKeyUpCaller;
typedef ReferenceCaller<Camera, &Camera_MoveUp_KeyDown> FreeMoveCameraMoveUpKeyDownCaller;
typedef ReferenceCaller<Camera, &Camera_MoveUp_KeyUp> FreeMoveCameraMoveUpKeyUpCaller;
typedef ReferenceCaller<Camera, &Camera_MoveDown_KeyDown> FreeMoveCameraMoveDownKeyDownCaller;
typedef ReferenceCaller<Camera, &Camera_MoveDown_KeyUp> FreeMoveCameraMoveDownKeyUpCaller;

void Camera_MoveForward_Discrete(Camera& camera) {
	camera.moveForwardDiscrete();
}

void Camera_MoveBack_Discrete(Camera& camera) {
	camera.moveBackDiscrete();
}

void Camera_MoveUp_Discrete(Camera& camera) {
	camera.moveUpDiscrete();
}

void Camera_MoveDown_Discrete(Camera& camera) {
	camera.moveDownDiscrete();
}

void Camera_MoveLeft_Discrete(Camera& camera) {
	camera.moveLeftDiscrete();
}

void Camera_MoveRight_Discrete(Camera& camera) {
	camera.moveRightDiscrete();
}

void Camera_RotateLeft_Discrete(Camera& camera) {
	camera.rotateLeftDiscrete();
}

void Camera_RotateRight_Discrete(Camera& camera) {
	camera.rotateRightDiscrete();
}

void Camera_PitchUp_Discrete(Camera& camera) {
	camera.pitchUpDiscrete();
}

void Camera_PitchDown_Discrete(Camera& camera) {
	camera.pitchDownDiscrete();
}

void Camera_motionDelta(int x, int y, unsigned int state, void* data) {
	Camera* cam = reinterpret_cast<Camera*>(data);

	cam->m_mouseMove.motion_delta(x, y, state);
	cam->m_strafe = GlobalEventMapper().strafeActive(state);

	if (cam->m_strafe) {
		cam->m_strafe_forward = GlobalEventMapper().strafeForwardActive(state);
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
		Camera_setOrigin(*camwnd, Camera_getOrigin(*camwnd) + camwnd->getCamera().forward * static_cast<float>(g_camwindow_globals_private.m_nMoveSpeed));
	}
	else if (event->direction == GDK_SCROLL_DOWN) {
		camwnd->getCamera().freemoveUpdateAxes();
		Camera_setOrigin(*camwnd, Camera_getOrigin(*camwnd) + camwnd->getCamera().forward * (-static_cast<float>(g_camwindow_globals_private.m_nMoveSpeed)));
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
		
		if (GlobalEventMapper().stateMatchesCameraViewEvent(ui::camEnableFreeLookMode, event)) {
			camwnd->EnableFreeMove();
			return TRUE;
		}
	}
	return FALSE;
}

gboolean disable_freelook_button_press(GtkWidget* widget, GdkEventButton* event, CamWnd* camwnd) {
	if (event->type == GDK_BUTTON_PRESS) {
		if (GlobalEventMapper().stateMatchesCameraViewEvent(ui::camDisableFreeLookMode, event)) {
			camwnd->DisableFreeMove();
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
		m_gl_widget(glwidget_new(TRUE)),
		m_window_observer(NewWindowObserver()),
		m_XORRectangle(m_gl_widget),
		m_deferredDraw(WidgetQueueDrawCaller(*m_gl_widget)),
		m_deferred_motion(selection_motion, m_window_observer),
		m_selection_button_press_handler(0),
		m_selection_button_release_handler(0),
		m_selection_motion_handler(0),
		m_freelook_button_press_handler(0),
		m_drawing(false) {
	m_bFreeMove = false;

	GlobalWindowObservers_add(m_window_observer);
	GlobalWindowObservers_connectWidget(m_gl_widget);

	m_window_observer->setRectangleDrawCallback(ReferenceCaller1<CamWnd, Rectangle, camwnd_update_xor_rectangle>(*this));
	m_window_observer->setView(m_view);

	gtk_widget_ref(m_gl_widget);

	gtk_widget_set_events(m_gl_widget, GDK_DESTROY | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);
	GTK_WIDGET_SET_FLAGS (m_gl_widget, GTK_CAN_FOCUS);
	gtk_widget_set_size_request(m_gl_widget, CAMWND_MINSIZE_X, CAMWND_MINSIZE_Y);

	m_sizeHandler = g_signal_connect(G_OBJECT(m_gl_widget), "size_allocate", G_CALLBACK(camera_size_allocate), this);
	m_exposeHandler = g_signal_connect(G_OBJECT(m_gl_widget), "expose_event", G_CALLBACK(camera_expose), this);

	Map_addValidCallback(g_map, DeferredDrawOnMapValidChangedCaller(m_deferredDraw));

	registerCommands();

	addHandlersMove();

	g_signal_connect(G_OBJECT(m_gl_widget), "scroll_event", G_CALLBACK(wheelmove_scroll), this);

	AddSceneChangeCallback(CamWndUpdate(*this));

	PressedButtons_connect(g_pressedButtons, m_gl_widget);
}

void CamWnd::registerCommands() {
	GlobalKeyEvents_insert("CameraForward", Accelerator(GDK_Up),
	                       ReferenceCaller<Camera, Camera_MoveForward_KeyDown>(getCamera()),
	                       ReferenceCaller<Camera, Camera_MoveForward_KeyUp>(getCamera())
	                      );
	GlobalKeyEvents_insert("CameraBack", Accelerator(GDK_Down),
	                       ReferenceCaller<Camera, Camera_MoveBack_KeyDown>(getCamera()),
	                       ReferenceCaller<Camera, Camera_MoveBack_KeyUp>(getCamera())
	                      );
	GlobalKeyEvents_insert("CameraLeft", Accelerator(GDK_Left),
	                       ReferenceCaller<Camera, Camera_RotateLeft_KeyDown>(getCamera()),
	                       ReferenceCaller<Camera, Camera_RotateLeft_KeyUp>(getCamera())
	                      );
	GlobalKeyEvents_insert("CameraRight", Accelerator(GDK_Right),
	                       ReferenceCaller<Camera, Camera_RotateRight_KeyDown>(getCamera()),
	                       ReferenceCaller<Camera, Camera_RotateRight_KeyUp>(getCamera())
	                      );
	GlobalKeyEvents_insert("CameraStrafeRight", Accelerator(GDK_period),
	                       ReferenceCaller<Camera, Camera_MoveRight_KeyDown>(getCamera()),
	                       ReferenceCaller<Camera, Camera_MoveRight_KeyUp>(getCamera())
	                      );
	GlobalKeyEvents_insert("CameraStrafeLeft", Accelerator(GDK_comma),
	                       ReferenceCaller<Camera, Camera_MoveLeft_KeyDown>(getCamera()),
	                       ReferenceCaller<Camera, Camera_MoveLeft_KeyUp>(getCamera())
	                      );
	GlobalKeyEvents_insert("CameraUp", Accelerator('D'),
	                       ReferenceCaller<Camera, Camera_MoveUp_KeyDown>(getCamera()),
	                       ReferenceCaller<Camera, Camera_MoveUp_KeyUp>(getCamera())
	                      );
	GlobalKeyEvents_insert("CameraDown", Accelerator('C'),
	                       ReferenceCaller<Camera, Camera_MoveDown_KeyDown>(getCamera()),
	                       ReferenceCaller<Camera, Camera_MoveDown_KeyUp>(getCamera())
	                      );
	GlobalKeyEvents_insert("CameraAngleDown", Accelerator('A'),
	                       ReferenceCaller<Camera, Camera_PitchDown_KeyDown>(getCamera()),
	                       ReferenceCaller<Camera, Camera_PitchDown_KeyUp>(getCamera())
	                      );
	GlobalKeyEvents_insert("CameraAngleUp", Accelerator('Z'),
	                       ReferenceCaller<Camera, Camera_PitchUp_KeyDown>(getCamera()),
	                       ReferenceCaller<Camera, Camera_PitchUp_KeyUp>(getCamera())
	                      );

	GlobalKeyEvents_insert("CameraFreeMoveForward", Accelerator(GDK_Up),
	                       FreeMoveCameraMoveForwardKeyDownCaller(getCamera()),
	                       FreeMoveCameraMoveForwardKeyUpCaller(getCamera())
	                      );
	GlobalKeyEvents_insert("CameraFreeMoveBack", Accelerator(GDK_Down),
	                       FreeMoveCameraMoveBackKeyDownCaller(getCamera()),
	                       FreeMoveCameraMoveBackKeyUpCaller(getCamera())
	                      );
	GlobalKeyEvents_insert("CameraFreeMoveLeft", Accelerator(GDK_Left),
	                       FreeMoveCameraMoveLeftKeyDownCaller(getCamera()),
	                       FreeMoveCameraMoveLeftKeyUpCaller(getCamera())
	                      );
	GlobalKeyEvents_insert("CameraFreeMoveRight", Accelerator(GDK_Right),
	                       FreeMoveCameraMoveRightKeyDownCaller(getCamera()),
	                       FreeMoveCameraMoveRightKeyUpCaller(getCamera())
	                      );
	GlobalKeyEvents_insert("CameraFreeMoveUp", Accelerator('D'),
	                       FreeMoveCameraMoveUpKeyDownCaller(getCamera()),
	                       FreeMoveCameraMoveUpKeyUpCaller(getCamera())
	                      );
	GlobalKeyEvents_insert("CameraFreeMoveDown", Accelerator('C'),
	                       FreeMoveCameraMoveDownKeyDownCaller(getCamera()),
	                       FreeMoveCameraMoveDownKeyUpCaller(getCamera())
	                      );

	GlobalCommands_insert("CameraForward", ReferenceCaller<Camera, Camera_MoveForward_Discrete>(getCamera()), Accelerator(GDK_Up));
	GlobalCommands_insert("CameraBack", ReferenceCaller<Camera, Camera_MoveBack_Discrete>(getCamera()), Accelerator(GDK_Down));
	GlobalCommands_insert("CameraLeft", ReferenceCaller<Camera, Camera_RotateLeft_Discrete>(getCamera()), Accelerator(GDK_Left));
	GlobalCommands_insert("CameraRight", ReferenceCaller<Camera, Camera_RotateRight_Discrete>(getCamera()), Accelerator(GDK_Right));
	GlobalCommands_insert("CameraStrafeRight", ReferenceCaller<Camera, Camera_MoveRight_Discrete>(getCamera()), Accelerator(GDK_period));
	GlobalCommands_insert("CameraStrafeLeft", ReferenceCaller<Camera, Camera_MoveLeft_Discrete>(getCamera()), Accelerator(GDK_comma));

	GlobalCommands_insert("CameraUp", ReferenceCaller<Camera, Camera_MoveUp_Discrete>(getCamera()), Accelerator('D'));
	GlobalCommands_insert("CameraDown", ReferenceCaller<Camera, Camera_MoveDown_Discrete>(getCamera()), Accelerator('C'));
	GlobalCommands_insert("CameraAngleUp", ReferenceCaller<Camera, Camera_PitchUp_Discrete>(getCamera()), Accelerator('A'));
	GlobalCommands_insert("CameraAngleDown", ReferenceCaller<Camera, Camera_PitchDown_Discrete>(getCamera()), Accelerator('Z'));
}

void CamWnd::Cam_ChangeFloor(bool up) {
	float current = m_Camera.origin[2] - 48;
	float bestUp;
	float bestDown;
	GlobalSceneGraph().traverse(FloorHeightWalker(current, bestUp, bestDown));

	if (up && bestUp != g_MaxWorldCoord) {
		current = bestUp;
	}

	if (!up && bestDown != -g_MaxWorldCoord) {
		current = bestDown;
	}

	m_Camera.origin[2] = current + 48;

	getCamera().updateModelview();
	update();
	CameraMovedNotify();
}

// NOTE TTimo if there's an OS-level focus out of the application
//   then we can release the camera cursor grab
static gboolean camwindow_freemove_focusout(GtkWidget* widget, GdkEventFocus* event, gpointer data) {
	reinterpret_cast<CamWnd*>(data)->DisableFreeMove();
	return FALSE;
}

void CamWnd::EnableFreeMove() {

	//globalOutputStream() << "EnableFreeMove\n";

	ASSERT_MESSAGE(!m_bFreeMove, "EnableFreeMove: free-move was already enabled");
	m_bFreeMove = true;
	getCamera().clearMovementFlags(MOVE_ALL);

	removeHandlersMove();
	addHandlersFreeMove();

	gtk_window_set_focus(m_parent, m_gl_widget);
	m_freemove_handle_focusout = g_signal_connect(G_OBJECT(m_gl_widget), "focus_out_event", G_CALLBACK(camwindow_freemove_focusout), this);
	m_freezePointer.freeze_pointer(m_parent, Camera_motionDelta, &m_Camera);

	update();
}

void CamWnd::DisableFreeMove() {
	//globalOutputStream() << "DisableFreeMove\n";

	ASSERT_MESSAGE(m_bFreeMove, "DisableFreeMove: free-move was not enabled");
	m_bFreeMove = false;
	getCamera().clearMovementFlags(MOVE_ALL);

	removeHandlersFreeMove();
	addHandlersMove();

	m_freezePointer.unfreeze_pointer(m_parent);
	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_freemove_handle_focusout);

	update();
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

	if (m_Camera.draw_mode != cd_lighting) {
		clearColour = ColourSchemes().getColourVector3("camera_background");
	}

	glClearColor(clearColour[0], clearColour[1], clearColour[2], 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	extern void Renderer_ResetStats();

	Renderer_ResetStats();

	extern void Cull_ResetStats();

	Cull_ResetStats();

	glMatrixMode(GL_PROJECTION);

	glLoadMatrixf(reinterpret_cast<const float*>(&m_Camera.projection));

	glMatrixMode(GL_MODELVIEW);

	glLoadMatrixf(reinterpret_cast<const float*>(&m_Camera.modelview));


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

	switch (m_Camera.draw_mode) {

		case cd_wire:
			break;

		case cd_solid:
			globalstate |= RENDER_FILL
			               | RENDER_LIGHTING
			               | RENDER_SMOOTH
			               | RENDER_SCALED;

			break;

		case cd_texture:
			globalstate |= RENDER_FILL
			               | RENDER_LIGHTING
			               | RENDER_TEXTURE
			               | RENDER_SMOOTH
			               | RENDER_SCALED;

			break;

		case cd_lighting:
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

	if (!g_xywindow_globals.m_bNoStipple) {
		globalstate |= RENDER_LINESTIPPLE|RENDER_POLYGONSTIPPLE;
	}

	{
		CamRenderer renderer(globalstate, m_state_select2, m_state_select1, m_view.getViewer());

		Scene_Render(renderer, m_view);

		renderer.render(m_Camera.modelview, m_Camera.projection);
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

	extern const char* Renderer_GetStats();

	GlobalOpenGL().drawString(Renderer_GetStats());

	glRasterPos3f(1.0f, static_cast<float>(m_Camera.height) - 11.0f, 0.0f);

	extern const char* Cull_GetStats();

	GlobalOpenGL().drawString(Cull_GetStats());

	// bind back to the default texture so that we don't have problems
	// elsewhere using/modifying texture maps between contexts
	glBindTexture( GL_TEXTURE_2D, 0 );
}

void CamWnd::draw() {
	m_drawing = true;

	//globalOutputStream() << "draw...\n";

	if (glwidget_make_current(m_gl_widget) != FALSE) {
		if (Map_Valid(g_map) && ScreenUpdates_Enabled()) {
			GlobalOpenGL_debugAssertNoErrors();
			Cam_Draw();
			GlobalOpenGL_debugAssertNoErrors();
			//qglFinish();

			m_XORRectangle.set(rectangle_t());
		}

		glwidget_swap_buffers(m_gl_widget);
	}

	m_drawing = false;
}

void CamWnd::BenchMark() {
	double dStart = Sys_DoubleTime();

	for (int i=0 ; i < 100 ; i++) {
		Vector3 angles;
		angles[CAMERA_ROLL] = 0;
		angles[CAMERA_PITCH] = 0;
		angles[CAMERA_YAW] = static_cast<float>(i * (360.0 / 100.0));
		Camera_setAngles(*this, angles);
	}

	double dEnd = Sys_DoubleTime();

	globalOutputStream() << FloatFormat(dEnd - dStart, 5, 2) << " seconds\n";
}

CamWnd::~CamWnd() {
	if (m_bFreeMove) {
		DisableFreeMove();
	}

	removeHandlersMove();

	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_sizeHandler);
	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_exposeHandler);

	gtk_widget_unref(m_gl_widget);

	m_window_observer->release();
}

// ----------------------------------------------------------

Shader* CamWnd::m_state_select1 = 0;
Shader* CamWnd::m_state_select2 = 0;

CamWnd* NewCamWnd() {
	return new CamWnd;
}

void DeleteCamWnd(CamWnd* camwnd) {
	delete camwnd;
}

void CamWnd_constructStatic() {
	CamWnd::captureStates();
}

void CamWnd_destroyStatic() {
	CamWnd::releaseStates();
}

CamWnd*& GlobalCamWnd() {
	static CamWnd* _camWnd;

	return _camWnd;
}

void CamWnd::addHandlersMove() {
	m_selection_button_press_handler = g_signal_connect(G_OBJECT(m_gl_widget), "button_press_event", G_CALLBACK(selection_button_press), m_window_observer);
	m_selection_button_release_handler = g_signal_connect(G_OBJECT(m_gl_widget), "button_release_event", G_CALLBACK(selection_button_release), m_window_observer);
	m_selection_motion_handler = g_signal_connect(G_OBJECT(m_gl_widget), "motion_notify_event", G_CALLBACK(DeferredMotion::gtk_motion), &m_deferred_motion);

	m_freelook_button_press_handler = g_signal_connect(G_OBJECT(m_gl_widget), "button_press_event", G_CALLBACK(enable_freelook_button_press), this);

	if (getCameraSettings()->discreteMovement()) {
		moveDiscreteEnable();
	} else {
		moveEnable();
	}
}


void CamWnd::removeHandlersMove() {
	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_selection_button_press_handler);
	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_selection_button_release_handler);
	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_selection_motion_handler);

	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_freelook_button_press_handler);

	if (getCameraSettings()->discreteMovement()) {
		moveDiscreteDisable();
	} else {
		moveDisable();
	}
}

void CamWnd::moveDiscreteEnable() {
	command_connect_accelerator("CameraForward");
	command_connect_accelerator("CameraBack");
	command_connect_accelerator("CameraLeft");
	command_connect_accelerator("CameraRight");
	command_connect_accelerator("CameraStrafeRight");
	command_connect_accelerator("CameraStrafeLeft");
	command_connect_accelerator("CameraUp");
	command_connect_accelerator("CameraDown");
	command_connect_accelerator("CameraAngleUp");
	command_connect_accelerator("CameraAngleDown");
}

void CamWnd::moveDiscreteDisable() {
	command_disconnect_accelerator("CameraForward");
	command_disconnect_accelerator("CameraBack");
	command_disconnect_accelerator("CameraLeft");
	command_disconnect_accelerator("CameraRight");
	command_disconnect_accelerator("CameraStrafeRight");
	command_disconnect_accelerator("CameraStrafeLeft");
	command_disconnect_accelerator("CameraUp");
	command_disconnect_accelerator("CameraDown");
	command_disconnect_accelerator("CameraAngleUp");
	command_disconnect_accelerator("CameraAngleDown");
}

void CamWnd::addHandlersFreeMove() {
	m_selection_button_press_handler = g_signal_connect(G_OBJECT(m_gl_widget), "button_press_event", G_CALLBACK(selection_button_press_freemove), m_window_observer);
	m_selection_button_release_handler = g_signal_connect(G_OBJECT(m_gl_widget), "button_release_event", G_CALLBACK(selection_button_release_freemove), m_window_observer);
	m_selection_motion_handler = g_signal_connect(G_OBJECT(m_gl_widget), "motion_notify_event", G_CALLBACK(selection_motion_freemove), m_window_observer);

	m_freelook_button_press_handler = g_signal_connect(G_OBJECT(m_gl_widget), "button_press_event", G_CALLBACK(disable_freelook_button_press), this);

	KeyEvent_connect("CameraFreeMoveForward");
	KeyEvent_connect("CameraFreeMoveBack");
	KeyEvent_connect("CameraFreeMoveLeft");
	KeyEvent_connect("CameraFreeMoveRight");
	KeyEvent_connect("CameraFreeMoveUp");
	KeyEvent_connect("CameraFreeMoveDown");
}

void CamWnd::removeHandlersFreeMove() {
	KeyEvent_disconnect("CameraFreeMoveForward");
	KeyEvent_disconnect("CameraFreeMoveBack");
	KeyEvent_disconnect("CameraFreeMoveLeft");
	KeyEvent_disconnect("CameraFreeMoveRight");
	KeyEvent_disconnect("CameraFreeMoveUp");
	KeyEvent_disconnect("CameraFreeMoveDown");

	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_selection_button_press_handler);
	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_selection_button_release_handler);
	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_selection_motion_handler);

	g_signal_handler_disconnect(G_OBJECT(m_gl_widget), m_freelook_button_press_handler);
}

void CamWnd::moveEnable() {
	KeyEvent_connect("CameraForward");
	KeyEvent_connect("CameraBack");
	KeyEvent_connect("CameraLeft");
	KeyEvent_connect("CameraRight");
	KeyEvent_connect("CameraStrafeRight");
	KeyEvent_connect("CameraStrafeLeft");
	KeyEvent_connect("CameraUp");
	KeyEvent_connect("CameraDown");
	KeyEvent_connect("CameraAngleUp");
	KeyEvent_connect("CameraAngleDown");
}

void CamWnd::moveDisable() {
	KeyEvent_disconnect("CameraForward");
	KeyEvent_disconnect("CameraBack");
	KeyEvent_disconnect("CameraLeft");
	KeyEvent_disconnect("CameraRight");
	KeyEvent_disconnect("CameraStrafeRight");
	KeyEvent_disconnect("CameraStrafeLeft");
	KeyEvent_disconnect("CameraUp");
	KeyEvent_disconnect("CameraDown");
	KeyEvent_disconnect("CameraAngleUp");
	KeyEvent_disconnect("CameraAngleDown");
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
	GlobalShaderCache().release("$CAM_HIGHLIGHT");
	GlobalShaderCache().release("$CAM_OVERLAY");
}

void CamWnd::queueDraw() {
	//ASSERT_MESSAGE(!m_drawing, "CamWnd::queue_draw(): called while draw is already in progress");

	if (m_drawing) {
		return;
	}

	//globalOutputStream() << "queue... ";
	m_deferredDraw.draw();
}

void CamWnd::setMode(camera_draw_mode mode) {
	ShaderCache_setBumpEnabled(mode == cd_lighting);
	Camera::setDrawMode(mode);

	if (GlobalCamWnd() != 0) {
		GlobalCamWnd()->update();
	}
}

camera_draw_mode CamWnd::getMode() {
	return Camera::draw_mode;
}

CameraView& CamWnd::getCameraView() {
	return m_cameraview;
}

GtkWidget* CamWnd::getWidget()
{
	return m_gl_widget;
}

GtkWindow* CamWnd::getParent() {
	return m_parent;
}
