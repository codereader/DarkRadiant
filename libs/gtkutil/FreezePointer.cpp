#include "FreezePointer.h"

#include <wx/window.h>
#include "debugging/debugging.h"
#include "MouseButton.h"

namespace wxutil
{

void FreezePointer::freeze(wxWindow& window, const MotionDeltaFunction& motionDelta, 
						   const EndMoveFunction& endMove)
{
	ASSERT_MESSAGE(motionDelta, "can't freeze pointer");
	ASSERT_MESSAGE(endMove, "can't freeze pointer");
	
	// Find the toplevel window 
	wxWindow* topLevel = &window;

	while (topLevel->GetParent() != NULL)
	{
		topLevel = topLevel->GetParent();
	}

    // Hide cursor and grab the pointer	
	topLevel->SetCursor(wxCursor(wxCURSOR_BLANK)); 

	topLevel->CaptureMouse();

	_capturedWindow = topLevel;

	wxPoint windowMousePos = topLevel->ScreenToClient(wxGetMousePosition());

	_freezePosX = windowMousePos.x;
	_freezePosY = windowMousePos.y;

	topLevel->WarpPointer(_freezePosX, _freezePosY);

	_motionDeltaFunction = motionDelta;
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

void FreezePointer::unfreeze()
{
	if (_capturedWindow == NULL)
	{
		return; // safeguard against duplicate unfreeze() calls
	}

	wxWindow& window = *_capturedWindow;

	_capturedWindow = NULL;

	_motionDeltaFunction = MotionDeltaFunction();
	_endMoveFunction = EndMoveFunction();

	window.WarpPointer(_freezePosX, _freezePosY);

	window.SetCursor(wxCursor(wxCURSOR_DEFAULT));

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
		_capturedWindow->WarpPointer(_freezePosX, _freezePosY);

		if (_motionDeltaFunction)
		{
			_motionDeltaFunction(dx, dy, MouseButton::GetStateForMouseEvent(ev));
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
