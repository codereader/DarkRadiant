#ifndef CAMWND_H_
#define CAMWND_H_

#include "iscenegraph.h"
#include "irender.h"
#include "gtkutil/GLWidget.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/GladeWidgetPopulator.h"
#include "selection/RadiantWindowObserver.h"

#include "view.h"
#include "map/DeferredDraw.h"

#include "RadiantCameraView.h"
#include "Camera.h"

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

const int CAMWND_MINSIZE_X = 240;
const int CAMWND_MINSIZE_Y = 200;

class SelectionTest;

class CamWnd :
	public scene::Graph::Observer,
	public boost::noncopyable,
    private gtkutil::GladeWidgetPopulator
{
	// The ID of this window
	int _id;

	static int _maxId;

	View m_view;

	// The contained camera
	Camera m_Camera;

	RadiantCameraView m_cameraview;

	sigc::connection m_freemove_handle_focusout;

	static ShaderPtr m_state_select1;
	static ShaderPtr m_state_select2;

	FreezePointer m_freezePointer;

	// Is true during an active drawing process
	bool m_drawing;

	bool m_bFreeMove;

    // Outer GUI widget (containing toolbar and GL widget)
    Gtk::Container* _mainWidget;

    // The GL widget
	gtkutil::GLWidget* _camGLWidget;
	Glib::RefPtr<Gtk::Window> _parentWindow;

	std::size_t _mapValidHandle;

	Rectangle _dragRectangle;

public:
	SelectionSystemWindowObserver* m_window_observer;

	DeferredDraw m_deferredDraw;
	DeferredMotion m_deferred_motion;

	sigc::connection m_selection_button_press_handler;
	sigc::connection m_selection_button_release_handler;
	sigc::connection m_selection_motion_handler;
	sigc::connection m_freelook_button_press_handler;
	sigc::connection m_freelook_button_release_handler;

	// Constructor and destructor
	CamWnd();

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

	void updateSelectionBox(const Rectangle& area);

	Vector3 getCameraOrigin() const;
	void setCameraOrigin(const Vector3& origin);

	Vector3 getCameraAngles() const;
	void setCameraAngles(const Vector3& angles);

	// greebo: This measures the rendering time during a 360° turn of the camera.
	void benchmark();

	// This tries to find brushes above/below the current camera position and moves the view upwards/downwards
	void changeFloor(const bool up);

	Gtk::Widget* getWidget() const;
	const Glib::RefPtr<Gtk::Window>& getParent() const;

	/**
	 * Set the immediate parent window of this CamWnd.
	 */
	void setContainer(const Glib::RefPtr<Gtk::Window>& newParent);

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

    void constructGUIComponents();

    // Toggle button callbacks
    void onPreviewButtonToggled();
    void onLightingButtonToggled();

	void Cam_Draw();

	void onSizeAllocate(Gtk::Allocation& allocation);
	bool onExpose(GdkEventExpose* ev);

	bool onMouseScroll(GdkEventScroll* ev);

	bool enableFreelookButtonPress(GdkEventButton* ev);
	bool disableFreelookButtonPress(GdkEventButton* ev);
	bool disableFreelookButtonRelease(GdkEventButton* ev);

	bool freeMoveFocusOut(GdkEventFocus* ev);

	bool selectionButtonPress(GdkEventButton* ev, SelectionSystemWindowObserver* observer);
	bool selectionButtonRelease(GdkEventButton* ev, SelectionSystemWindowObserver* observer);

	bool selectionButtonPressFreemove(GdkEventButton* ev, SelectionSystemWindowObserver* observer);
	bool selectionButtonReleaseFreemove(GdkEventButton* ev, SelectionSystemWindowObserver* observer);
	bool selectionMotionFreemove(GdkEventMotion* ev, SelectionSystemWindowObserver* observer);
};

/**
 * Shared pointer typedef.
 */
typedef boost::shared_ptr<CamWnd> CamWndPtr;
typedef boost::weak_ptr<CamWnd> CamWndWeakPtr;

#endif /*CAMWND_H_*/
