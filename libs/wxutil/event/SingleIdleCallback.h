#pragma once

#include <wx/event.h>
#include <wx/app.h>

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
class SingleIdleCallback
{
private:
	bool _registered;

    // To avoid inheriting from wxEvtHandler, we use composition
    // maintaining an inner wxEvtHandler instance to be invoked on idle
    class InternalEventHandler :
        public wxEvtHandler
    {
    private:
        SingleIdleCallback& _owner;
    public:
        InternalEventHandler(SingleIdleCallback& owner) :
            _owner(owner)
        {}

        void _onIdle(wxIdleEvent&)
        {
            // Call the virtual function
            _owner.handleIdleCallback();
        }
    } _evtHandler;

private:

	void handleIdleCallback()
	{
        wxTheApp->Unbind(wxEVT_IDLE, &InternalEventHandler::_onIdle, &_evtHandler);
        _registered = false;
		
		// Call the virtual function
		onIdle();
	}

	// Remove the callback
	void deregisterCallback()
	{
        if (!_registered) return;

        if (wxTheApp)
        {
            _registered = false;
            wxTheApp->Unbind(wxEVT_IDLE, &InternalEventHandler::_onIdle, &_evtHandler);
        }
	}

protected:

	/**
	 * Request an idle callback. The onIdle() method will be invoked during
	 * the next idle period.
	 */
	void requestIdleCallback()
	{
		if (_registered) return; // avoid duplicate registrations

		if (wxTheApp)
		{
            _registered = true;
            wxTheApp->Bind(wxEVT_IDLE, &InternalEventHandler::_onIdle, &_evtHandler);
		}
	}

    void cancelCallbacks()
    {
        deregisterCallback();
    }

	// Flushes any pending events, forces a call to onGtkIdle, if necessary
	void flushIdleCallback()
	{
		if (_registered)
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
	virtual void onIdle() = 0;

public:

    SingleIdleCallback() :
        _registered(false),
        _evtHandler(*this)
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
