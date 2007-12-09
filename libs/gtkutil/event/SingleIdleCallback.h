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
 * A class which derives from this must invoke the enableIdleCallback() method
 * when it is ready to receive an idle callback. Subsequently, it will have its 
 * onGtkIdle() method invoked once during the GTK idle period. 
 */
class SingleIdleCallback
{
	// ID of the registered callback (for subsequent removal)
	guint _id;

private:
	
	// Actual GTK idle callback static function, which invokes the child class'
	// implementing method
	static gboolean _onIdle(gpointer self) {
		SingleIdleCallback* cb = reinterpret_cast<SingleIdleCallback*>(self);
		cb->onGtkIdle();
		cb->deregisterCallback();
		return FALSE; // automatically deregister callback 
	}
	
	// Remove the callback
	void deregisterCallback() {
		g_source_remove(_id);
	}
	
protected:
	
	/**
	 * Enable the callback. Until this method is invoked, no GTK idle callback
	 * will take place.
	 */
	void enableIdleCallback() {
		_id = g_idle_add(_onIdle, this);
	}
	
	/**
	 * Implementing method for the idle callback. Code in this method will
	 * be executed once when GTK is idle.
	 */
	virtual void onGtkIdle() { }
	
public:
	
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
