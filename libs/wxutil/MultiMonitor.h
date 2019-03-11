#pragma once

#include <wx/display.h>

#include "itextstream.h"

namespace wxutil
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
	static unsigned int getNumMonitors()
	{
		// Get and return the number of monitors
		return wxDisplay::GetCount();
	}

	/**
	 * Returns the screen rectangle of the screen with the given index.
	 * The first screen is always present and has the index 0.
	 */
	static wxRect getMonitor(int monitorNum)
	{
		wxDisplay display(monitorNum);

		return display.GetGeometry();
	}

	/**
	 * Returns the index of the monitor
	 * the given window is currently displayed on.
	 * If the window cannot be found, the first monitor number 0 is returned.
	 */
	static unsigned int getMonitorNumForWindow(wxWindow* window)
	{
		int num = wxDisplay::GetFromWindow(window);
		return num != wxNOT_FOUND && num > 0 ? static_cast<unsigned int>(num) : 0;
	}

	/**
	 * Returns the rectangle (width/height) for the monitor
	 * which the given window is displayed on.
	 */
	static wxRect getMonitorForWindow(wxWindow* window)
	{
		// Retrieve the screen
		wxDisplay display(wxDisplay::GetFromWindow(window));
		
		return display.GetGeometry();
	}

	static void printMonitorInfo()
	{
		rMessage() << "Default screen has " << getNumMonitors() << " monitors." << std::endl;

		// detect multiple monitors
		for (unsigned int j = 0; j < getNumMonitors(); j++)
		{
			wxRect geom = getMonitor(j);

			rMessage() << "Monitor " << j << " geometry: "
				<< geom.GetWidth() << "x" << geom.GetHeight() << " at "
				<< geom.GetX() << ", " << geom.GetY() << std::endl;
		}
	}
};

} // namespace
