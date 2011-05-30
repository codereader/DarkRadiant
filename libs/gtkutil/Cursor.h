#pragma once

#include <gdkmm/cursor.h>

namespace gtkutil
{

class Cursor
{
public:
	static Gdk::Cursor createBlank()
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
};

} // namespace
