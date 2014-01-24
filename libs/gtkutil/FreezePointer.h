#pragma once

#include <gtkmm/window.h>
#include <wx/wxprec.h>
#include <boost/function.hpp>

namespace gtkutil
{

class FreezePointer
{
public:
	typedef boost::function<void(int, int, guint state)> MotionDeltaFunction;

private:
	int _freezePosX;
	int _freezePosY;

	MotionDeltaFunction _function;

	sigc::connection _motionHandler;

public:
	FreezePointer() : 
		_freezePosX(0),
		_freezePosY(0)
	{}

	/**
	 * Catch any mouse pointer movements and make sure the pointer is staying in place.
	 * Any mouse movement will be reported to the given MotionDeltaFunction.
	 */
	void freeze(const Glib::RefPtr<Gtk::Window>& window, const MotionDeltaFunction& function);

	/**
	 * Un-freeze the cursor of the given window again. This moves the cursor back
	 * to where it was before.
	 */
	void unfreeze(const Glib::RefPtr<Gtk::Window>& window);

private:
	// The callback to connect to the motion-notify-event
	bool _onMouseMotion(GdkEventMotion* ev, const Glib::RefPtr<Gtk::Window>& window);
};

} // namespace

namespace wxutil
{

class FreezePointer :
	public wxEvtHandler
{
public:
	typedef boost::function<void(int, int, unsigned int state)> MotionDeltaFunction;
	typedef boost::function<void()> EndMoveFunction;
	typedef boost::function<void(wxMouseEvent&)> MouseEventFunction;

private:
	int _freezePosX;
	int _freezePosY;

	MotionDeltaFunction _motionDeltaFunction;
	EndMoveFunction _endMoveFunction;

	wxWindow* _capturedWindow;
	bool _callEndMoveOnMouseUp;

	MouseEventFunction _onMouseUp;
	MouseEventFunction _onMouseDown;

public:
	FreezePointer() : 
		_freezePosX(0),
		_freezePosY(0),
		_capturedWindow(NULL),
		_callEndMoveOnMouseUp(false)
	{}

	/**
	 * Catch any mouse pointer movements and make sure the pointer is staying in place.
	 * Any mouse movement will be reported to the given MotionDeltaFunction.
	 * The EndMoveFunction will be invoked as soon as the cursor capture is lost or 
	 * any mouse button is released again.
	 */
	void freeze(wxWindow& window, const MotionDeltaFunction& function, 
		const EndMoveFunction& endMove);

	/**
	 * Un-freeze the cursor again. This moves the cursor back
	 * to where it was before.
	 */
	void unfreeze();

	/**
	 * Whether to end the freeze when the mouse button is released.
	 * Defaults to FALSE on construction.
	 */
	void setCallEndMoveOnMouseUp(bool callEndMoveOnMouseUp);

	/**
	 * During freeze mouse button events might be eaten by the window.
	 * Use these to enable event propagation.
	 */
	void connectMouseEvents(const MouseEventFunction& onMouseDown, const MouseEventFunction& onMouseUp);
	void disconnectMouseEvents();

private:
	// During capture we might need to propagate the mouseup and
	// mousedown events to the client
	void onMouseUp(wxMouseEvent& ev);
	void onMouseDown(wxMouseEvent& ev);

	// The callback to connect to the motion-notify-event
	void onMouseMotion(wxMouseEvent& ev);
	void onMouseCaptureLost(wxMouseCaptureLostEvent& ev);
};

} // namespace
