#ifndef CAMWND_H_
#define CAMWND_H_

#include "RadiantCameraView.h"
#include "irender.h"
#include "gtkutil/xorrectangle.h"
#include "selection/RadiantWindowObserver.h"

#include "view.h"
#include "map.h"

const int CAMWND_MINSIZE_X = 240;
const int CAMWND_MINSIZE_Y = 200;

class CamWnd {
	View m_view;
	
	// The contained camera
	Camera m_Camera;
	
	RadiantCameraView m_cameraview;
#if 0
	int m_PositionDragCursorX;
	int m_PositionDragCursorY;
#endif

	guint m_freemove_handle_focusout;
	static Shader* m_state_select1;
	static Shader* m_state_select2;
	FreezePointer m_freezePointer;

public:
	GtkWidget* m_gl_widget;
	GtkWindow* m_parent;

	SelectionSystemWindowObserver* m_window_observer;

	XORRectangle m_XORRectangle;

	DeferredDraw m_deferredDraw;
	DeferredMotion m_deferred_motion;

	guint m_selection_button_press_handler;
	guint m_selection_button_release_handler;
	guint m_selection_motion_handler;
	guint m_freelook_button_press_handler;
	guint m_sizeHandler;
	guint m_exposeHandler;

	CamWnd();
	~CamWnd();

	void registerCommands();

	bool m_drawing;

	void queueDraw();
	void draw();
	void update();

	static void captureStates();
	static void releaseStates();

	static void setMode(CameraDrawMode mode);
	static CameraDrawMode getMode();

	Camera& getCamera();
	
	const Vector3& getCameraOrigin();
	void setCameraOrigin(const Vector3& origin);
	
	const Vector3& getCameraAngles();
	void setCameraAngles(const Vector3& angles);

	void BenchMark();
	void changeFloor(const bool up);

	GtkWidget* getWidget();
	GtkWindow* getParent();

	void EnableFreeMove();
	void DisableFreeMove();

	bool m_bFreeMove;

	CameraView* getCameraView();

	void addHandlersMove();

	void addHandlersFreeMove();

	void removeHandlersMove();
	void removeHandlersFreeMove();

	void moveEnable();
	void moveDiscreteEnable();

	void moveDisable();
	void moveDiscreteDisable();
	
	// This toggles between lighting and solid rendering mode
	void toggleLightingMode();
	
	void cubicScaleIn();
	void cubicScaleOut();

private:
	void Cam_Draw();
};

typedef MemberCaller<CamWnd, &CamWnd::queueDraw> CamWndQueueDraw;
typedef MemberCaller<CamWnd, &CamWnd::update> CamWndUpdate;

struct camwindow_globals_private_t {
	int m_nMoveSpeed;
	int m_nAngleSpeed;

	camwindow_globals_private_t() :
			m_nMoveSpeed(100),
			m_nAngleSpeed(3) {}

};

extern camwindow_globals_private_t g_camwindow_globals_private;

#endif /*CAMWND_H_*/
