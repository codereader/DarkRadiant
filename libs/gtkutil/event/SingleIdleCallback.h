#pragma once

#include <glib.h>

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
private:
	// ID of the registered callback (for subsequent removal)
	guint _id;

private:

	// Actual GTK idle callback static function, which invokes the child class'
	// implementing method
	static gboolean _onIdle(gpointer self)
	{
		SingleIdleCallback* cb = reinterpret_cast<SingleIdleCallback*>(self);

		// Call the virtual function
		cb->onGtkIdle();

		cb->_id = 0;

		return FALSE; // automatically deregister callback
	}

	// Remove the callback
	void deregisterCallback()
	{
		if (_id != 0)
		{
			g_source_remove(_id);
			_id = 0;
		}
	}

protected:

	/**
	 * Request an idle callback. The onGtkIdle() method will be invoked during
	 * the next GTK idle period.
	 */
	void requestIdleCallback()
	{
		if (_id != 0) return; // avoid duplicate registrations

		_id = g_idle_add(_onIdle, this);
	}

	// TRUE if an idle callback is pending
	bool callbacksPending() const
	{
		return _id != 0;
	}

	// Flushes any pending events, forces a call to onGtkIdle, if necessary
	void flushIdleCallback()
	{
		if (_id != 0)
		{
			// Cancel the event and force an onGtkIdle() call
			deregisterCallback();

			// Invoke the idle callback
			onGtkIdle();
		}
	}

	/**
	 * Implementing method for the idle callback. Code in this method will
	 * be executed only when GTK is idle.
	 */
	virtual void onGtkIdle() { }

public:

	SingleIdleCallback() :
		_id(0)
	{}

	/**
	 * Destructor. De-registers the callback from GTK, so that the method will
	 * not be invoked after the object is destroyed.
	 */
	virtual ~SingleIdleCallback()
	{
		deregisterCallback();
	}
};

}
