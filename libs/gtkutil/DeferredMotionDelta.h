#pragma once

#include <sigc++/connection.h>
#include <glib.h>
#include <gdk/gdkevents.h>
#include <boost/function.hpp>

#include "event/SingleIdleCallback.h"

namespace gtkutil
{

/**
 * A class accumulating mouse motion delta calls. When GTK is idle, 
 * the attached function object is called with the stored x,y delta values.
 */
class DeferredMotionDelta :
	private SingleIdleCallback
{
public:
	typedef boost::function<void(int, int)> MotionDeltaFunction;

private:
	int _deltaX;
	int _deltaY;

	sigc::connection _motionHandler;
	
	MotionDeltaFunction _function;

public:
	DeferredMotionDelta(const MotionDeltaFunction& function) : 
		_deltaX(0), 
		_deltaY(0),
		_function(function)
	{}

	void flush()
	{
		flushIdleCallback();
	}

	void onMouseMotionDelta(int x, int y, guint state)
	{
		_deltaX += x;
		_deltaY += y;

		requestIdleCallback();
	}

private:
	void onGtkIdle()
	{
		_function(_deltaX, _deltaY);

		_deltaX = 0;
		_deltaY = 0;
	}
};

} // namespace
