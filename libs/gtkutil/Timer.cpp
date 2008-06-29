#include "Timer.h"

namespace gtkutil
{

// Constructor (instantiate with timeout in millisec. and a static callback function)
Timer::Timer(const unsigned long timeout, TimerCallback callback, gpointer data) :
	_timerID(0),
	_timeout(timeout),
	_callback(callback),
	_data(data)
{}

// Sets the timeout to the given value (in ms). The changes get 
// activate when enable() is called the next time.
void Timer::setTimeout(const unsigned long timeout) {
	// Stop the current timer, if there is one running
	disable();
	
	// Store the new timeout value
	_timeout = timeout;
}

// Sets the callback to the given <callback>
void Timer::setCallback(TimerCallback callback) {
	// Stop the current timer, if there is one running
	disable();
	
	// Store the new callback function internally
	_callback = callback;
}

// Sets the callback to the given <callback>
void Timer::setCallbackData(gpointer data) {
	// Stop the current timer, if there is one running
	disable();
	
	// Store the new data pointer internally
	_data = data;
}

// Starts the timer and saves the handler id locally
void Timer::enable() {
	if (_timerID == 0) {
		_timerID = gtk_timeout_add(_timeout, _callback, _data);
	}
}

// Disables the timer and resets the handler ID
void Timer::disable() {
	if (_timerID != 0) {
		gtk_timeout_remove(_timerID);
		_timerID = 0;
	}
}

bool Timer::isEnabled() const {
	return (_timerID != 0);
}

} // namespace gtkutil
