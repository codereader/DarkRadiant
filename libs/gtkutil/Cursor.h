#pragma once

#include <gdkmm/cursor.h>
#include <gtkmm/window.h>

namespace gtkutil
{

class Cursor
{
public:
	/**
	 * Reads the cursor position of the given window and writes the result to x,y.
	 */
	static void ReadPosition(const Glib::RefPtr<Gtk::Window>& window, int& x, int& y);

	/**
	 * Sets the cursor position of the given window to x,y.
	 */
	static void SetPosition(const Glib::RefPtr<Gtk::Window>& window, int x, int y);
};

} // namespace
