#ifndef GTKUTIL_TIMER_H_
#define GTKUTIL_TIMER_H_

#include "gtk/gtkmain.h"

/* greebo: This is an encapsulation of the gtk_timeout_ methods that
 * periodically call a certain function in the given intervals (resolution: 1 ms).
 * 
 * Instantiate this Timer with a timeout in ms and a GTK-compatible callback.
 * (e.g. Cast a static member function using G_CALLBACK() and pass it).
 */

namespace gtkutil {

class Timer
{
	// The handler ID for the connected timer
	guint _timerID;

	// The timeout interval in milliseconds
	unsigned long _timeout;

	typedef gboolean (*TimerCallback)(gpointer data);

	// The timer callback function (GTK compatible)
	TimerCallback _callback;
	
	// The pointer to pass (e.g. a pointer to a class instance)
	gpointer _data;

public:
	// Constructor (instantiate with timeout in millisec. and a static callback function)
	Timer(const unsigned long timeout, TimerCallback callback, gpointer data) :
		_timeout(timeout),
		_callback(callback),
		_data(data)
	{}
	
	// Sets the timeout to the given value (in ms). The changes get 
	// activate when enable() is called the next time.
	void setTimeout(const unsigned long timeout) {
		// Stop the current timer, if there is one running
		disable();
		
		// Store the new timeout value
		_timeout = timeout;
	}
	
	// Sets the callback to the given <callback>
	void setCallback(TimerCallback callback) {
		// Stop the current timer, if there is one running
		disable();
		
		// Store the new callback function internally
		_callback = callback;
	}
	
	// Sets the callback to the given <callback>
	void setCallbackData(gpointer data) {
		// Stop the current timer, if there is one running
		disable();
		
		// Store the new data pointer internally
		_data = data;
	}
	
	// Starts the timer and saves the handler id locally
	void enable() {
		if (_timerID == 0) {
			_timerID = gtk_timeout_add(_timeout, _callback, _data);
		}
	}
	
	// Disables the timer and resets the handler ID
	void disable() {
		if (_timerID != 0) {
			gtk_timeout_remove(_timerID);
			_timerID = 0;
		}
	}
	
	bool isEnabled() const {
		return (_timerID != 0);
	}
}; // class Timer

} // namespace gtkutil

#endif /*GTKUTIL_TIMER_H_*/
