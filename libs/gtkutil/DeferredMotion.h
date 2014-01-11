#pragma once

#include <gdk/gdkevents.h>
#include <boost/function.hpp>

#include "event/SingleIdleCallback.h"

namespace gtkutil
{

/** 
 * greebo: this class is used by the Cam- and Orthoviews as "onMouseMotion" buffer
 * It is buffering the motion calls until GTK is idle, in which case the 
 * attached callback is invoked with the buffered x,y and state parameters.
 */
class DeferredMotion :
	protected SingleIdleCallback
{
public:
	// The motion function to invoke when GTK is idle
	// Signature: void myFunction(gdouble x, gdouble y, guint state);
	typedef boost::function<void(gdouble, gdouble, guint)> MotionCallback;

private:
	MotionCallback _motionCallback;

	gdouble _x;
	gdouble _y;
	guint _state;

public:
	DeferredMotion(const MotionCallback& motionCallback) :
		_motionCallback(motionCallback)
	{}

	// greebo: This is the actual callback method that gets connected via to the "motion_notify_event"
	bool onMouseMotion(GdkEventMotion* ev)
	{
		_x = ev->x;
		_y = ev->y;
		_state = ev->state;

		requestIdleCallback();
		
		return false;
	}

protected:
	// GTK idle callback
	void onGtkIdle()
	{
		_motionCallback(_x, _y, _state);
	}
};

} // namespace
