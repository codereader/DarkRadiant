#pragma once

#include <wx/wxprec.h>
#include <wx/event.h>
#include <functional>

namespace wxutil
{

class FreezePointer :
	public wxEvtHandler
{
public:
	typedef std::function<void(int, int, unsigned int state)> MotionFunction;
	typedef std::function<void()> EndMoveFunction;
	typedef std::function<void(wxMouseEvent&)> MouseEventFunction;

private:
	int _freezePosX;
	int _freezePosY;

    // Whether to lock the cursor in its position
    bool _freezePointer;

    // Whether to hide the cursor during capture
    bool _hidePointer;

    // Whether the motion callback receives deltas or absolute coords
    bool _motionReceivesDeltas;

	MotionFunction _motionFunction;
	EndMoveFunction _endMoveFunction;

	wxWindow* _capturedWindow;
	bool _callEndMoveOnMouseUp;

	MouseEventFunction _onMouseUp;
	MouseEventFunction _onMouseDown;
	MouseEventFunction _onMouseMotion;

public:
	FreezePointer() : 
		_freezePosX(0),
		_freezePosY(0),
        _freezePointer(true),
        _hidePointer(true),
        _motionReceivesDeltas(true),
		_capturedWindow(NULL),
		_callEndMoveOnMouseUp(false)
	{}

	/**
	 * Catch any mouse pointer movements. 
	 * Any mouse movement will be reported to the given MotionFunction.
	 * The EndMoveFunction will be invoked as soon as the cursor capture is lost or 
	 * any mouse button is released again.
	 */
    void startCapture(wxWindow& window, const MotionFunction& function,
		const EndMoveFunction& endMove);

	/**
	 * Un-capture the cursor again. If the cursor was frozen, this moves 
     * it back to where it was before.
	 */
	void endCapture();

    // Activate or deactivate the freeze pointer behaviour
    // when activated, the cursor will be forced to stay at the current position
    void setFreezePointer(bool shouldFreeze);

    // Set this to true to hide the cursor while the capture is active
    void setHidePointer(bool shouldHide);

    // Controls whether (during capture) the MotionFunction should receive
    // deltas (relative to start point) or absolute coordinates.
    void setSendMotionDeltas(bool shouldSendDeltasOnly);

	/**
	 * Whether to end the freeze when the mouse button is released.
	 * Defaults to FALSE on construction.
	 */
	void setCallEndMoveOnMouseUp(bool callEndMoveOnMouseUp);

	/**
	 * During freeze mouse button events might be eaten by the window.
	 * Use these to enable event propagation.
	 */
	void connectMouseEvents(const MouseEventFunction& onMouseDown, 
							const MouseEventFunction& onMouseUp,
							const MouseEventFunction& onMouseMotion);
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
