#ifndef CAMWND_H_
#define CAMWND_H_

#include "iscenegraph.h"
#include "irender.h"
#include "gtkutil/GLWidget.h"
#include "gtkutil/xorrectangle.h"
#include "gtkutil/WindowPosition.h"
#include "selection/RadiantWindowObserver.h"

#include "view.h"
#include "map/DeferredDraw.h"

#include "RadiantCameraView.h"
#include "Camera.h"

const int CAMWND_MINSIZE_X = 240;
const int CAMWND_MINSIZE_Y = 200;

class SelectionTest;

class CamWnd :
	public scene::Graph::Observer
{
	View m_view;
	
	// The contained camera
	Camera m_Camera;
	
	RadiantCameraView m_cameraview;

	guint m_freemove_handle_focusout;
	
	static ShaderPtr m_state_select1;
	static ShaderPtr m_state_select2;
	
	FreezePointer m_freezePointer;

	// Is true during an active drawing process
	bool m_drawing;

	bool m_bFreeMove;

	gtkutil::GLWidget m_gl_widget;
	GtkWindow* _parentWidget;

public:

	SelectionSystemWindowObserver* m_window_observer;

	XORRectangle m_XORRectangle;

	DeferredDraw m_deferredDraw;
	DeferredMotion m_deferred_motion;

	guint m_selection_button_press_handler;
	guint m_selection_button_release_handler;
	guint m_selection_motion_handler;
	guint m_freelook_button_press_handler;
	guint m_freelook_button_release_handler;
	guint m_sizeHandler;
	guint m_exposeHandler;

	// Constructor and destructor
	CamWnd();
	~CamWnd();

	void queueDraw();
	void draw();
	void update();
	
	// The callback when the scene gets changed
	void onSceneGraphChange();

	static void captureStates();
	static void releaseStates();

	Camera& getCamera();
	
	void updateXORRectangle(Rectangle area);
	typedef MemberCaller1<CamWnd, Rectangle, &CamWnd::updateXORRectangle> updateXORRectangleCallback;
	
	Vector3 getCameraOrigin() const;
	void setCameraOrigin(const Vector3& origin);
	
	Vector3 getCameraAngles() const;
	void setCameraAngles(const Vector3& angles);

	// greebo: This measures the rendering time during a 360� turn of the camera.
	void benchmark();
	
	// This tries to find brushes above/below the current camera position and moves the view upwards/downwards
	void changeFloor(const bool up);

	GtkWidget* getWidget() const;
	GtkWindow* getParent() const;
	
	/**
	 * Set the immediate GTK container of this CamWnd.
	 */
	void setContainer(GtkWindow* newParent);

	void enableFreeMove();
	void disableFreeMove();
	bool freeMoveEnabled() const;

	void jumpToObject(SelectionTest& selectionTest);

	CameraView* getCameraView();

	// Enables/disables the (ordinary) camera movement (non-freelook)
	void addHandlersMove();
	void removeHandlersMove();

	void enableDiscreteMoveEvents();
	void enableFreeMoveEvents();
	void disableDiscreteMoveEvents();
	void disableFreeMoveEvents();
	
	// Increases/decreases the far clip plane distance
	void cubicScaleIn();
	void cubicScaleOut();
	
private:
	void Cam_Draw();
};

/**
 * Shared pointer typedef.
 */
typedef boost::shared_ptr<CamWnd> CamWndPtr;

typedef MemberCaller<CamWnd, &CamWnd::queueDraw> CamWndQueueDraw;
typedef MemberCaller<CamWnd, &CamWnd::update> CamWndUpdate;

#endif /*CAMWND_H_*/
