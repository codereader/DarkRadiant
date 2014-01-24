#pragma once

#include <glib.h>
#include <wx/wxprec.h>

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

namespace wxutil
{

/**
 * Base class for classes which wish to receive a single callback when the GUI is
 * idle, to perform processing that does not block the GUI.
 *
 * A class which derives from this must invoke the requestIdleCallback() method
 * whenever it is ready to receive an idle callback. Subsequently, it will have
 * its onIdle() method invoked once during the idle period.
 */
class SingleIdleCallback :
	public wxEvtHandler
{
private:
	bool _callbackPending;

private:

	// Actual wxIdelEvent callback method, which invokes the child class'
	// implementing method
	void _onIdle(wxIdleEvent& ev)
	{
		wxTheApp->Disconnect(wxEVT_IDLE, wxIdleEventHandler(wxutil::SingleIdleCallback::_onIdle), NULL, this);
		
		// Call the virtual function
		onIdle();

		_callbackPending = false;
	}

	// Remove the callback
	void deregisterCallback()
	{
		if (_callbackPending)
		{
			wxTheApp->Disconnect(wxEVT_IDLE, wxIdleEventHandler(wxutil::SingleIdleCallback::_onIdle), NULL, this);
			_callbackPending = false;
		}
	}

protected:

	/**
	 * Request an idle callback. The onIdle() method will be invoked during
	 * the next idle period.
	 */
	void requestIdleCallback()
	{
		if (_callbackPending) return; // avoid duplicate registrations

		_callbackPending = true;
		wxTheApp->Connect(wxEVT_IDLE, wxIdleEventHandler(wxutil::SingleIdleCallback::_onIdle), NULL, this);
	}

	// TRUE if an idle callback is pending
	bool callbacksPending() const
	{
		return _callbackPending;
	}

	// Flushes any pending events, forces a call to onGtkIdle, if necessary
	void flushIdleCallback()
	{
		if (_callbackPending)
		{
			// Cancel the event and force an onIdle() call
			deregisterCallback();

			// Invoke the idle callback
			onIdle();
		}
	}

	/**
	 * Implementing method for the idle callback. Code in this method will
	 * be executed only when the app is idle.
	 */
	virtual void onIdle() { }

public:

	SingleIdleCallback() :
		_callbackPending(0)
	{}

	/**
	 * Destructor. De-registers the callback from wx, so that the method will
	 * not be invoked after the object is destroyed.
	 */
	virtual ~SingleIdleCallback()
	{
		deregisterCallback();
	}
};

}
