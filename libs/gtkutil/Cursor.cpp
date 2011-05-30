#include "Cursor.h"

#ifdef WIN32
#include <gdk/gdkwin32.h>
#else
#include <gdk/gdkx.h>
#endif

namespace gtkutil
{

Gdk::Cursor Cursor::createBlank()
{
	char buffer[(32 * 32)/8];
	memset(buffer, 0, (32 * 32)/8);
		
	Gdk::Color white; 
	white.set_rgb(0xffff, 0xffff, 0xffff);

	Gdk::Color black;
	black.set_rgb(0x0000, 0x0000, 0x0000);

	Glib::RefPtr<Gdk::Pixmap> pixmap = Gdk::Pixmap::create_from_data(
		Glib::RefPtr<Gdk::Drawable>(), buffer, 32, 32, 1, white, black);

	Glib::RefPtr<Gdk::Pixmap> mask = Gdk::Pixmap::create_from_data(
		Glib::RefPtr<Gdk::Drawable>(), buffer, 32, 32, 1, white, black);

	return Gdk::Cursor(pixmap, mask, white, black, 1, 1);
}

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
