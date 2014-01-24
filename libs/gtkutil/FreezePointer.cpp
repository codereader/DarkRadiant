#include "FreezePointer.h"

#include "Cursor.h"
#include "debugging/debugging.h"
#include "MouseButton.h"

namespace gtkutil
{

void FreezePointer::freeze(const Glib::RefPtr<Gtk::Window>& window, const MotionDeltaFunction& function)
{
	ASSERT_MESSAGE(!_function, "can't freeze pointer");
	
	const Gdk::EventMask mask = 
		Gdk::POINTER_MOTION_MASK | Gdk::POINTER_MOTION_HINT_MASK | Gdk::BUTTON_MOTION_MASK | 
		Gdk::BUTTON1_MOTION_MASK | Gdk::BUTTON2_MOTION_MASK | Gdk::BUTTON3_MOTION_MASK |
		Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::VISIBILITY_NOTIFY_MASK;

    // Hide cursor and grab the pointer
    Gdk::Cursor blank(Gdk::BLANK_CURSOR);
    // undone for wx window->get_window()->pointer_grab(true, mask, blank, GDK_CURRENT_TIME);

	Cursor::ReadPosition(window, _freezePosX, _freezePosY);
	Cursor::SetPosition(window, _freezePosX,_freezePosY);
		
	_function = function;

	// undone for wx _motionHandler = window->signal_motion_notify_event().connect(sigc::bind(sigc::mem_fun(*this, &FreezePointer::_onMouseMotion), window));
}

void FreezePointer::unfreeze(const Glib::RefPtr<Gtk::Window>& window)
{
	_motionHandler.disconnect();
	_function = MotionDeltaFunction();

	Cursor::SetPosition(window, _freezePosX,_freezePosY);
	
	// undone for wx Gdk::Window::pointer_ungrab(GDK_CURRENT_TIME);
}

bool FreezePointer::_onMouseMotion(GdkEventMotion* ev, const Glib::RefPtr<Gtk::Window>& window)
{
	int current_x, current_y;
	Cursor::ReadPosition(window, current_x, current_y);
		
	int dx = current_x - _freezePosX;
	int dy = current_y - _freezePosY;

	if (dx != 0 || dy != 0)
	{
		Cursor::SetPosition(window, _freezePosX, _freezePosY);

		if (_function)
		{
			_function(dx, dy, ev->state);
		}
	}

	return FALSE;
}

} // namespace

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

void FreezePointer::connectMouseEvents(const MouseEventFunction& onMouseDown, const MouseEventFunction& onMouseUp)
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
}

void FreezePointer::onMouseCaptureLost(wxMouseCaptureLostEvent& ev)
{
	if (_endMoveFunction)
	{
		_endMoveFunction();
	}
}

} // namespace
