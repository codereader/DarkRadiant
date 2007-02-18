#ifndef GTKUTIL_TIMER_H_
#define GTKUTIL_TIMER_H_

#include "gtk/gtkmain.h"

/* greebo: This is an encapsulation of the gtk_timeout_ methods that
 * periodically call a certain function in the given intervals (resolution: 1 ms).
 * 
 * Instantiate this Timer with a timeout in ms and a GTK-compatible callback.
 * (e.g. a static member function that can be cast by using G_CALLBACK()).
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
	Timer(const unsigned long timeout, TimerCallback callback, gpointer data);
	
	// Sets the timeout to the given value (in ms). The changes get 
	// activated when enable() is called the next time.
	void setTimeout(const unsigned long timeout);
	
	// Sets the callback to the given <callback>
	void setCallback(TimerCallback callback);
	
	// Sets the callback to the given <callback>
	void setCallbackData(gpointer data);
	
	// Starts the timer and saves the handler id locally
	void enable();
	
	// Disables the timer and resets the handler ID
	void disable();
	
	bool isEnabled() const;

}; // class Timer

} // namespace gtkutil

#endif /*GTKUTIL_TIMER_H_*/
