#ifndef _GTKUTIL_MULTIMON_H_
#define _GTKUTIL_MULTIMON_H_

#include <gdk/gdkscreen.h>
#include <gdk/gdkdisplay.h>

namespace gtkutil {

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
	static int getNumMonitors() {
		// Acquire the default screen reference
		GdkScreen* screen = gdk_display_get_default_screen(gdk_display_get_default());
		
		// Get and return the number of monitors
		return gdk_screen_get_n_monitors(screen);
	}

	/** 
	 * Returns the screen rectangle of the screen with the given index.
	 * The first screen is always present and has the index 0.
	 */
	static GdkRectangle getMonitor(int screenNum) {
		GdkScreen* screen = gdk_display_get_default_screen(gdk_display_get_default());

		GdkRectangle geom;
		gdk_screen_get_monitor_geometry(screen, screenNum, &geom);

		return geom;
	}
};

} // namespace gtkutil

#endif /* _GTKUTIL_MULTIMON_H_ */
