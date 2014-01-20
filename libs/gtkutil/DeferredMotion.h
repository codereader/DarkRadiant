#pragma once

#include <gdk/gdkevents.h>
#include <boost/function.hpp>

#include <wx/wxprec.h>
#include "event/SingleIdleCallback.h"
#include "gtkutil/MouseButton.h"

namespace gtkutil
{

/** 
 * greebo: this class is used by the Cam- and Orthoviews as "onMouseMotion" buffer
 * It is buffering the motion calls until GTK is idle, in which case the 
 * attached callback is invoked with the buffered x,y and state parameters.
 */
class DeferredMotion :
	protected SingleIdleCallback,
	public wxEvtHandler
{
public:
	// The motion function to invoke when GTK is idle
	// Signature: void myFunction(int x, int y, unsigned int state);
	typedef boost::function<void(int, int, unsigned int)> MotionCallback;

private:
	MotionCallback _motionCallback;

	int _x;
	int _y;
	unsigned int _state;

public:
	DeferredMotion(const MotionCallback& motionCallback) :
		_motionCallback(motionCallback)
	{}

	// greebo: This is the actual callback method that gets connected via to the "motion_notify_event"
	bool onMouseMotion(GdkEventMotion* ev)
	{
		_x = static_cast<int>(ev->x);
		_y = static_cast<int>(ev->y);
		_state = ev->state;

		requestIdleCallback();
		
		return false;
	}

	void wxOnMouseMotion(wxMouseEvent& ev)
	{
		_x = ev.GetX();
		_y = ev.GetY();
		_state = wxutil::MouseButton::GetStateForMouseEvent(ev);

		requestIdleCallback();
	}

protected:
	// GTK idle callback
	void onGtkIdle()
	{
		_motionCallback(_x, _y, _state);
	}
};

} // namespace
