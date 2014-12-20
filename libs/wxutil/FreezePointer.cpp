#include "FreezePointer.h"

#include <wx/window.h>
#include "debugging/debugging.h"
#include "MouseButton.h"

namespace wxutil
{

void FreezePointer::startCapture(wxWindow& window, const MotionFunction& motionDelta, 
						   const EndMoveFunction& endMove)
{
	ASSERT_MESSAGE(motionDelta, "can't capture pointer");
	ASSERT_MESSAGE(endMove, "can't capture pointer");
	
	// Find the toplevel window 
	wxWindow* topLevel = &window;

	while (topLevel->GetParent() != NULL)
	{
		topLevel = topLevel->GetParent();
	}

    // Hide cursor and grab the pointer	
    if (_hidePointer)
    {
        topLevel->SetCursor(wxCursor(wxCURSOR_BLANK));
    }

	topLevel->CaptureMouse();

	_capturedWindow = topLevel;

	wxPoint windowMousePos = topLevel->ScreenToClient(wxGetMousePosition());

	_freezePosX = windowMousePos.x;
	_freezePosY = windowMousePos.y;

    if (_freezePointer)
    {
        topLevel->WarpPointer(_freezePosX, _freezePosY);
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
	if (_capturedWindow == NULL)
	{
		return; // safeguard against duplicate unfreeze() calls
	}

	wxWindow& window = *_capturedWindow;

	_capturedWindow = NULL;

	_motionFunction = MotionFunction();
	_endMoveFunction = EndMoveFunction();

    if (_freezePointer)
    {
        window.WarpPointer(_freezePosX, _freezePosY);
    }

    if (_hidePointer)
    {
        window.SetCursor(wxCursor(wxCURSOR_DEFAULT));
    }

	if (window.HasCapture())
	{
		window.ReleaseMouse();
	}

	window.Disconnect(wxEVT_MOUSE_CAPTURE_LOST, wxMouseCaptureLostEventHandler(FreezePointer::onMouseCaptureLost), NULL, this);
	window.Disconnect(wxEVT_MOTION, wxMouseEventHandler(FreezePointer::onMouseMotion), NULL, this);

	window.Disconnect(wxEVT_LEFT_UP, wxMouseEventHandler(FreezePointer::onMouseUp), NULL, this);
	window.Disconnect(wxEVT_RIGHT_UP, wxMouseEventHandler(FreezePointer::onMouseUp), NULL, this);
	window.Disconnect(wxEVT_MIDDLE_UP, wxMouseEventHandler(FreezePointer::onMouseUp), NULL, this);
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
		_onMouseDown(ev);
	}
}

void FreezePointer::onMouseUp(wxMouseEvent& ev)
{
	if (_onMouseUp)
	{
		_onMouseUp(ev);
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
                _motionFunction(ev.GetX(), ev.GetY(), MouseButton::GetStateForMouseEvent(ev));
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
}

} // namespace
