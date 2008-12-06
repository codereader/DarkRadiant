#ifndef SINGLEIDLECALLBACK_H_
#define SINGLEIDLECALLBACK_H_

#include <glib/gmain.h>

#include <iostream>

namespace gtkutil
{

/**
 * Base class for classes which wish to receive a single callback when GTK is
 * idle, to perform processing that does not block the GUI. 
 * 
 * A class which derives from this must invoke the requestIdleCallback() method
 * whenever it is ready to receive an idle callback. Subsequently, it will have 
 * its onGtkIdle() method invoked once during the GTK idle period. 
 */
class SingleIdleCallback
{
	// ID of the registered callback (for subsequent removal)
	guint _id;

	// some sort of "resource lock" to avoid duplicate callback registrations
	bool _callbacksPending;

private:
	
	// Actual GTK idle callback static function, which invokes the child class'
	// implementing method
	static gboolean _onIdle(gpointer self) {
		SingleIdleCallback* cb = reinterpret_cast<SingleIdleCallback*>(self);

		// Call the virtual function
		cb->onGtkIdle();

		cb->_callbacksPending = false;

		return FALSE; // automatically deregister callback 
	}
	
	// Remove the callback
	void deregisterCallback() {
		g_source_remove(_id);
		_callbacksPending = false;
	}
	
protected:
	
	/**
	 * Request an idle callback. The onGtkIdle() method will be invoked during
	 * the next GTK idle period.
	 */
	void requestIdleCallback() {
		if (_callbacksPending) return; // avoid duplicate registrations

		_callbacksPending = true;

		_id = g_idle_add(_onIdle, this);
	}
	
	/**
	 * Implementing method for the idle callback. Code in this method will
	 * be executed only when GTK is idle.
	 */
	virtual void onGtkIdle() { }
	
public:

	SingleIdleCallback() :
		_callbacksPending(false)
	{}

	/**
	 * Destructor. De-registers the callback from GTK, so that the method will
	 * not be invoked after the object is destroyed.
	 */
	~SingleIdleCallback() {
		deregisterCallback();
	}
};

}

#endif /*SINGLEIDLECALLBACK_H_*/
