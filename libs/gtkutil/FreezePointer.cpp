#include "FreezePointer.h"

#include "cursor_old.h"
#include "Cursor.h"
#include "debugging/debugging.h"

namespace gtkutil
{

void FreezePointer::freeze(const Glib::RefPtr<Gtk::Window>& window, const MotionDeltaFunction& function)
{
	ASSERT_MESSAGE(!_function, "can't freeze pointer");
	
	const Gdk::EventMask mask = 
		Gdk::POINTER_MOTION_MASK | Gdk::POINTER_MOTION_HINT_MASK | Gdk::BUTTON_MOTION_MASK | 
		Gdk::BUTTON1_MOTION_MASK | Gdk::BUTTON2_MOTION_MASK | Gdk::BUTTON3_MOTION_MASK |
		Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | Gdk::VISIBILITY_NOTIFY_MASK;

	Gdk::Cursor cursor = Cursor::createBlank();

	window->get_window()->pointer_grab(true, mask, cursor, GDK_CURRENT_TIME);

	Sys_GetCursorPos(window->gobj(), &_freezePosX, &_freezePosY);
	Sys_SetCursorPos(window->gobj(), _freezePosX, _freezePosY);
		
	_function = function;

	_motionHandler = window->signal_motion_notify_event().connect(
		sigc::bind(sigc::mem_fun(*this, &FreezePointer::_onMouseMotion), window));
}

void FreezePointer::unfreeze(const Glib::RefPtr<Gtk::Window>& window)
{
	_motionHandler.disconnect();
	_function = MotionDeltaFunction();

	Sys_SetCursorPos(window->gobj(), _freezePosX, _freezePosY);

	Gdk::Window::pointer_ungrab(GDK_CURRENT_TIME);
}

bool FreezePointer::_onMouseMotion(GdkEventMotion* ev, const Glib::RefPtr<Gtk::Window>& window)
{
	int current_x, current_y;
	Sys_GetCursorPos(GTK_WINDOW(window->gobj()), &current_x, &current_y);

	int dx = current_x - _freezePosX;
	int dy = current_y - _freezePosY;

	if (dx != 0 || dy != 0)
	{
		Sys_SetCursorPos(GTK_WINDOW(window->gobj()), _freezePosX, _freezePosY);

		if (_function)
		{
			_function(dx, dy, ev->state);
		}
	}

	return FALSE;
}

} // namespace
