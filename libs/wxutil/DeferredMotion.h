#pragma once

#include <boost/function.hpp>

#include <wx/wxprec.h>
#include "event/SingleIdleCallback.h"
#include "wxutil/MouseButton.h"

namespace wxutil
{

/** 
 * greebo: this class is used by the Cam- and Orthoviews as "onMouseMotion" buffer
 * It is buffering the motion calls until the app is idle, in which case the 
 * attached callback is invoked with the buffered x,y and state parameters.
 */
class DeferredMotion :
	public wxutil::SingleIdleCallback
{
public:
	// The motion function to invoke when the application is idle
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

	void wxOnMouseMotion(wxMouseEvent& ev)
	{
		_x = ev.GetX();
		_y = ev.GetY();
		_state = wxutil::MouseButton::GetStateForMouseEvent(ev);

		wxutil::SingleIdleCallback::requestIdleCallback();
	}

protected:
	// wxWidgets idle callback
	void onIdle()
	{
		_motionCallback(_x, _y, _state);
	}
};

} // namespace
