#include "Cursor.h"

#ifdef WIN32
#include <gdk/win32/gdkwin32.h>
#else
#include <gdk/gdkx.h>
#endif

namespace gtkutil
{

#ifdef WIN32

void Cursor::ReadPosition(const Glib::RefPtr<Gtk::Window>& window, int& x, int& y)
{
	POINT pos;
	GetCursorPos(&pos);

	ScreenToClient((HWND)GDK_WINDOW_HWND(window->get_window()->gobj()), &pos);

	x = pos.x;
	y = pos.y;
}

void Cursor::SetPosition(const Glib::RefPtr<Gtk::Window>& window, int x, int y)
{
	POINT pos;
	pos.x = x;
	pos.y = y;

	ClientToScreen((HWND)GDK_WINDOW_HWND(window->get_window()->gobj()), &pos);

	SetCursorPos(pos.x, pos.y);
}

#else

void Cursor::ReadPosition(const Glib::RefPtr<Gtk::Window>& window, int& x, int& y)
{
	gdk_display_get_pointer(gdk_display_get_default(), 0, &x, &y, 0);
}

void Cursor::SetPosition(const Glib::RefPtr<Gtk::Window>& window, int x, int y)
{
	XWarpPointer(GDK_DISPLAY(), None, GDK_ROOT_WINDOW(), 0, 0, 0, 0, x, y);
}

#endif

} // namespace
