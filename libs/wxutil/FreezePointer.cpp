#include "FreezePointer.h"

#include <wx/window.h>
#include "debugging/debugging.h"
#include "MouseButton.h"

namespace wxutil
{

void FreezePointer::startCapture(wxWindow* window,
                                 const MotionFunction& motionDelta,
                                 const CaptureLostFunction& endMove,
                                 bool freezePointer,
                                 bool hidePointer,
                                 bool motionReceivesDeltas)
{
    assert(window);
	ASSERT_MESSAGE(motionDelta, "can't capture pointer");
	ASSERT_MESSAGE(endMove, "can't capture pointer");

    // Store the flags before going ahead
    _freezePointer = freezePointer;
    _hidePointer = hidePointer;
    _motionReceivesDeltas = motionReceivesDeltas;

	// Find the toplevel window
	wxWindow* topLevel = wxGetTopLevelParent(window);

    if (_hidePointer)
    {
        window->SetCursor(wxCursor(wxCURSOR_BLANK));
    }

    // We capture the mouse on the toplevel app, coordinates
    // are measured relative to the child window
    if (!topLevel->HasCapture())
    {
        topLevel->CaptureMouse();
    }

    _capturedWindow = window;

    wxPoint windowMousePos = _capturedWindow->ScreenToClient(wxGetMousePosition());

	_freezePosX = windowMousePos.x;
	_freezePosY = windowMousePos.y;

    if (_freezePointer)
    {
        _capturedWindow->WarpPointer(_freezePosX, _freezePosY);
    }

	_motionFunction = motionDelta;
	_captureLostFunction = endMove;

    topLevel->Connect(wxEVT_MOTION, wxMouseEventHandler(FreezePointer::onMouseMotion), NULL, this);

	topLevel->Connect(wxEVT_LEFT_UP, wxMouseEventHandler(FreezePointer::onMouseUp), NULL, this);
	topLevel->Connect(wxEVT_RIGHT_UP, wxMouseEventHandler(FreezePointer::onMouseUp), NULL, this);
	topLevel->Connect(wxEVT_MIDDLE_UP, wxMouseEventHandler(FreezePointer::onMouseUp), NULL, this);
	topLevel->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(FreezePointer::onMouseDown), NULL, this);
	topLevel->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(FreezePointer::onMouseDown), NULL, this);
	topLevel->Connect(wxEVT_MIDDLE_DOWN, wxMouseEventHandler(FreezePointer::onMouseDown), NULL, this);

    topLevel->Connect(wxEVT_MOUSE_CAPTURE_LOST, wxMouseCaptureLostEventHandler(FreezePointer::onMouseCaptureLost), NULL, this);
}

bool FreezePointer::isCapturing(wxWindow* window) const
{
    return _capturedWindow != nullptr;
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
	_captureLostFunction = CaptureLostFunction();

    if (_freezePointer)
    {
        window->WarpPointer(_freezePosX, _freezePosY);
    }

    if (_hidePointer)
    {
        window->SetCursor(wxCursor(wxCURSOR_DEFAULT));
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
    topLevel->Disconnect(wxEVT_LEFT_DOWN, wxMouseEventHandler(FreezePointer::onMouseDown), NULL, this);
	topLevel->Disconnect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(FreezePointer::onMouseDown), NULL, this);
	topLevel->Disconnect(wxEVT_MIDDLE_DOWN, wxMouseEventHandler(FreezePointer::onMouseDown), NULL, this);
}

void FreezePointer::connectMouseEvents(const MouseEventFunction& onMouseDown,
									   const MouseEventFunction& onMouseUp)
{
	_onMouseUp = onMouseUp;
	_onMouseDown = onMouseDown;
}

void FreezePointer::disconnectMouseEvents()
{
	_onMouseUp = MouseEventFunction();
	_onMouseDown = MouseEventFunction();
}

void FreezePointer::onMouseDown(wxMouseEvent& ev)
{
	if (_onMouseDown && _capturedWindow)
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
}

void FreezePointer::onMouseMotion(wxMouseEvent& ev)
{
    if (!_capturedWindow) return;

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

	ev.Skip();
}

void FreezePointer::onMouseCaptureLost(wxMouseCaptureLostEvent& ev)
{
    if (_captureLostFunction)
	{
		_captureLostFunction();
	}

    // Regardless of what the client does, we need to end capture now
    endCapture();
}

} // namespace
