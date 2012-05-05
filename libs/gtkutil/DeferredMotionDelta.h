#pragma once

#include <glibmm.h>
#include <gdk/gdkevents.h>
#include <boost/function.hpp>

namespace gtkutil
{

/**
 * A class accumulating mouse motion delta calls. When GTK is idle, 
 * the attached function object is called with the stored x,y delta values.
 */
class DeferredMotionDelta
{
public:
	typedef boost::function<void(int, int)> MotionDeltaFunction;

private:
	int _deltaX;
	int _deltaY;

	sigc::connection _motionHandler;
	
	MotionDeltaFunction _function;

public:

    /// Initialise with function to call
	DeferredMotionDelta(const MotionDeltaFunction& function) : 
		_deltaX(0), 
		_deltaY(0),
		_function(function)
	{}

	void flush()
	{
		_function(_deltaX, _deltaY);

		_deltaX = 0;
		_deltaY = 0;
	}

    /// Supply motion delta and invoke the callback when idle
	void onMouseMotionDelta(int x, int y, guint state)
	{
		_deltaX += x;
		_deltaY += y;

        Glib::signal_idle().connect_once(
            sigc::mem_fun(this, &DeferredMotionDelta::flush)
        );
	}
};

} // namespace
