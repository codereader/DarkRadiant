#ifndef SINGLEIDLECALLBACK_H_
#define SINGLEIDLECALLBACK_H_

#include <glib/gmain.h>

namespace gtkutil
{

/**
 * Base class for classes which wish to receive a single callback when GTK is
 * idle, to perform processing that does not block the GUI. A class which
 * derives from this will have its onGtkIdle() method invoked once during the
 * GTK idle period. An empty default implementation is provided.
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
		return FALSE; // automatically deregister callback 
	}
	
protected:
	
	/**
	 * Implementing method for the idle callback. Code in this method will
	 * be executed once when GTK is idle.
	 */
	virtual void onGtkIdle() { }
	
public:
	
	/**
	 * Constructor. Registers the callback.
	 */
	SingleIdleCallback() 
	: _id(g_idle_add(_onIdle, this))
	{ }
	
	/**
	 * Destructor. De-registers the callback from GTK, so that the method will
	 * not be invoked after the object is destroyed.
	 */
	~SingleIdleCallback() {
		g_source_remove(_id);
	}
};

}

#endif /*SINGLEIDLECALLBACK_H_*/
