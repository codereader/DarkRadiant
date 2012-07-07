#ifndef _GTKUTIL_MULTIMON_H_
#define _GTKUTIL_MULTIMON_H_

#include <gdkmm/display.h>
#include <gdkmm/screen.h>
#include <gtkmm/window.h>

#include "itextstream.h"

namespace gtkutil
{

/**
 * greebo: This class acts as container for several
 * multi-monitor-related functions. Use the getMonitor() method
 * to acquire the screen dimensions of the given screen.
 */
class MultiMonitor
{
public:
	/**
	 * Returns the number of monitors of the default screen.
	 */
	static int getNumMonitors()
	{
		// Acquire the default screen reference
		Glib::RefPtr<Gdk::Screen> screen = Gdk::Display::get_default()->get_default_screen();

		// Get and return the number of monitors
		return screen->get_n_monitors();;
	}

	/**
	 * Returns the screen rectangle of the screen with the given index.
	 * The first screen is always present and has the index 0.
	 */
	static Gdk::Rectangle getMonitor(int monitorNum)
	{
		Glib::RefPtr<Gdk::Screen> screen = Gdk::Display::get_default()->get_default_screen();

		Gdk::Rectangle geom;
		screen->get_monitor_geometry(monitorNum, geom);

		return geom;
	}

	/**
	 * greebo: Returns the rectangle (width/height) for the monitor
	 * which the given window is displayed on.
	 */
	static Gdk::Rectangle getMonitorForWindow(const Glib::RefPtr<Gtk::Window>& window)
	{
		// Retrieve the screen
		Glib::RefPtr<Gdk::Screen> scr = window->get_screen();

		// Get the monitor which the GtkWindow is displayed on
		int monitorNum = scr->get_monitor_at_window(window->get_window());

		return getMonitor(monitorNum);
	}

	/**
	 * greebo: Returns the rectangle (width/height) for the monitor
	 * which the given window is displayed on.
	 */
	static Gdk::Rectangle getMonitorForWindow(Gtk::Window& window)
	{
		// Retrieve the screen
		Glib::RefPtr<Gdk::Screen> scr = window.get_screen();

		// Get the monitor which the GtkWindow is displayed on
		int monitorNum = scr->get_monitor_at_window(window.get_window());

		return getMonitor(monitorNum);
	}

	static void printMonitorInfo()
	{
		rMessage() << "Default screen has " << getNumMonitors() << " monitors." << std::endl;

		// detect multiple monitors
		for (int j = 0; j < getNumMonitors(); j++)
		{
			Gdk::Rectangle geom = getMonitor(j);

			rMessage() << "Monitor " << j << " geometry: "
				<< geom.get_width() << "x" << geom.get_height() << " at "
				<< geom.get_x() << ", " << geom.get_y() << std::endl;
		}
	}
};

} // namespace gtkutil

#endif /* _GTKUTIL_MULTIMON_H_ */
