#include "FreezePointer.h"

#include <wx/window.h>
#include "debugging/debugging.h"
#include "MouseButton.h"

namespace wxutil
{

FreezePointer::FreezePointer() :
    _freezePosX(0),
    _freezePosY(0),
    _freezePointer(true),
    _hidePointer(true),
    _motionReceivesDeltas(true),
    _capturedWindow(nullptr),
    _callEndMoveOnMouseUp(false)
{}

void FreezePointer::startCapture(wxWindow* window,
                                 const MotionFunction& motionDelta,
                                 const EndMoveFunction& endMove)
{
    startCapture(window, motionDelta, endMove, true, true, true);
}

void FreezePointer::startCapture(wxWindow* window, 
                                 const MotionFunction& motionDelta, 
                                 const EndMoveFunction& endMove,
                                 bool freezePointer,
                                 bool hidePointer,
                                 bool motionReceivesDeltas)
{
    assert(window);
	ASSERT_MESSAGE(motionDelta, "can't capture pointer");
	ASSERT_MESSAGE(endMove, "can't capture pointer");

    // Pass the flags before going ahead
    setFreezePointer(freezePointer);
    setHidePointer(hidePointer);
    setSendMotionDeltas(motionReceivesDeltas);
	
	// Find the toplevel window 
	wxWindow* topLevel = wxGetTopLevelParent(window);

    if (_hidePointer)
    {
        topLevel->SetCursor(wxCursor(wxCURSOR_BLANK));
    }

    // We capture the mouse on the toplevel app, coordinates
    // are measured relative to the child window
    topLevel->CaptureMouse();

    _capturedWindow = window;

    wxPoint windowMousePos = _capturedWindow->ScreenToClient(wxGetMousePosition());

	_freezePosX = windowMousePos.x;
	_freezePosY = windowMousePos.y;

    if (_freezePointer)
    {
        _capturedWindow->WarpPointer(_freezePosX, _freezePosY);
    }

	_motionFunction = motionDelta;
	_endMoveFunction = endMove;

    topLevel->Connect(wxEVT_MOTION, wxMouseEventHandler(FreezePointer::onMouseMotion), NULL, this);

	topLevel->Connect(wxEVT_LEFT_UP, wxMouseEventHandler(FreezePointer::onMouseUp), NULL, this);
	topLevel->Connect(wxEVT_RIGHT_UP, wxMouseEventHandler(FreezePointer::onMouseUp), NULL, this);
	topLevel->Connect(wxEVT_MIDDLE_UP, wxMouseEventHandler(FreezePointer::onMouseUp), NULL, this);
	topLevel->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(FreezePointer::onMouseDown), NULL, this);
	topLevel->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(FreezePointer::onMouseDown), NULL, this);
	topLevel->Connect(wxEVT_MIDDLE_DOWN, wxMouseEventHandler(FreezePointer::onMouseDown), NULL, this);

    topLevel->Connect(wxEVT_MOUSE_CAPTURE_LOST, wxMouseCaptureLostEventHandler(FreezePointer::onMouseCaptureLost), NULL, this);
}

void FreezePointer::endCapture()
{
	if (!_capturedWindow)
	{
		return; // safeguard against duplicate unfreeze() calls
	}

	wxWindow* window = _capturedWindow;
    wxWindow* topLevel = wxGetTopLevelParent(window);

    _capturedWindow = nullptr;

	_motionFunction = MotionFunction();
	_endMoveFunction = EndMoveFunction();

    if (_freezePointer)
    {
        window->WarpPointer(_freezePosX, _freezePosY);
    }

    if (_hidePointer)
    {
        topLevel->SetCursor(wxCursor(wxCURSOR_DEFAULT));
    }

    if (topLevel->HasCapture())
	{
        topLevel->ReleaseMouse();
	}

    topLevel->Disconnect(wxEVT_MOUSE_CAPTURE_LOST, wxMouseCaptureLostEventHandler(FreezePointer::onMouseCaptureLost), NULL, this);
    topLevel->Disconnect(wxEVT_MOTION, wxMouseEventHandler(FreezePointer::onMouseMotion), NULL, this);

	topLevel->Disconnect(wxEVT_LEFT_UP, wxMouseEventHandler(FreezePointer::onMouseUp), NULL, this);
	topLevel->Disconnect(wxEVT_RIGHT_UP, wxMouseEventHandler(FreezePointer::onMouseUp), NULL, this);
	topLevel->Disconnect(wxEVT_MIDDLE_UP, wxMouseEventHandler(FreezePointer::onMouseUp), NULL, this);
}

void FreezePointer::setFreezePointer(bool shouldFreeze)
{
    _freezePointer = shouldFreeze;
}

void FreezePointer::setHidePointer(bool shouldHide)
{
    _hidePointer = shouldHide;
}

void FreezePointer::setSendMotionDeltas(bool shouldSendDeltasOnly)
{
    _motionReceivesDeltas = shouldSendDeltasOnly;
}

void FreezePointer::setCallEndMoveOnMouseUp(bool callEndMoveOnMouseUp)
{
	_callEndMoveOnMouseUp = callEndMoveOnMouseUp;
}

void FreezePointer::connectMouseEvents(const MouseEventFunction& onMouseDown, 
									   const MouseEventFunction& onMouseUp, 
									   const MouseEventFunction& onMouseMotion)
{
	_onMouseUp = onMouseUp;
	_onMouseDown = onMouseDown;
}

void FreezePointer::disconnectMouseEvents()
{
	_onMouseUp = MouseEventFunction();
	_onMouseDown = MouseEventFunction();
	_onMouseMotion = MouseEventFunction();
}

void FreezePointer::onMouseDown(wxMouseEvent& ev)
{
	if (_onMouseDown)
	{
        // The connected mouse up event expects window coordinates, 
        // not coordinates relative to the captured window
        wxMouseEvent copy(ev);
        wxPoint windowMousePos = _capturedWindow->ScreenToClient(wxGetMousePosition());

        copy.SetX(windowMousePos.x);
        copy.SetY(windowMousePos.y);

        _onMouseDown(copy);
	}
}

void FreezePointer::onMouseUp(wxMouseEvent& ev)
{
    if (_onMouseUp && _capturedWindow)
	{
        // The connected mouse up event expects window coordinates, 
        // not coordinates relative to the captured window
        wxMouseEvent copy(ev);
        wxPoint windowMousePos = _capturedWindow->ScreenToClient(wxGetMousePosition());

        copy.SetX(windowMousePos.x);
        copy.SetY(windowMousePos.y);

		_onMouseUp(copy);
	}

	if (_callEndMoveOnMouseUp && _endMoveFunction)
	{
		_endMoveFunction();
	}
}

void FreezePointer::onMouseMotion(wxMouseEvent& ev)
{
	wxPoint windowMousePos = _capturedWindow->ScreenToClient(wxGetMousePosition());
		
	int dx = windowMousePos.x - _freezePosX;
	int dy = windowMousePos.y - _freezePosY;

	if (dx != 0 || dy != 0)
	{
        if (_freezePointer)
        {
            // Force the mouse cursor to stay where it is
            _capturedWindow->WarpPointer(_freezePosX, _freezePosY);
        }
        else
        {
            // Non-freezing, update the reference point for the next delta
            _freezePosX = windowMousePos.x;
            _freezePosY = windowMousePos.y;
        }

		if (_motionFunction)
		{
            if (_motionReceivesDeltas)
            {
                _motionFunction(dx, dy, MouseButton::GetStateForMouseEvent(ev));
            }
            else
            {
                _motionFunction(windowMousePos.x, windowMousePos.y, MouseButton::GetStateForMouseEvent(ev));
            }
		}
	}

	if (_onMouseMotion)
	{
		_onMouseMotion(ev);
	}

	ev.Skip();
}

void FreezePointer::onMouseCaptureLost(wxMouseCaptureLostEvent& ev)
{
    if (_endMoveFunction)
	{
		_endMoveFunction();
	}

    // Regardless of what the client does, we need to end capture now
    endCapture();
}

} // namespace
