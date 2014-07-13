#pragma once

#include "ieventmanager.h"
#include "render/View.h"

#include "windowobserver.h"
#include "Device.h"
#include "SelectObserver.h"
#include "ManipulateObserver.h"
#include "SelectionTest.h"
#include "Rectangle.h"

#include <wx/event.h>

class wxWindow;

// Abstract base class of the SelectionSystem Observer extending the WindowObserver interface
class SelectionSystemWindowObserver :
	public WindowObserver
{
public:
	virtual void setView(const render::View& view) = 0;
	virtual void setRectangleDrawCallback(const selection::Rectangle::Callback& callback) = 0;

	virtual void addObservedWidget(wxWindow& observed) = 0;
	virtual void removeObservedWidget(wxWindow& observed) = 0;
};

// ====================================================================================

/* greebo: This is the hub class that observes a view, the implementation of the abstract base class above.
 * It basically checks all "incoming" mouse clicks and passes them to the according
 * subclasses like Selector_ and ManipulateObserver, these in turn pass them to the RadiantSelectionSystem
 *
 * Note that some calls for button/modifiers could be catched in the XYView / Camview callback methods, so that
 * they never reach the WindowObserver (examples may be a Clipper command).
 */
class RadiantWindowObserver :
	public SelectionSystemWindowObserver,
	public wxEvtHandler
{
private:
	// The tolerance when it comes to the construction of selection boxes
	float _selectEpsilon;

	// The window dimensions
	int _width;
  	int _height;

	// This is true, if the "select" mouse button is currently pressed (important to know for drag operations)
  	bool _mouseDown;

	// Whether the key handler should listen for cancel events
	bool _listenForCancelEvents;

  	// A "third-party" event to be called when the mouse moved and/or button is released
	// Usually points to the Manipulate or Select Observer classes.
	MouseEventCallback _mouseMotionCallback;
	MouseEventCallback _mouseUpCallback;

public:
	// These are the classes that handle the selection- and manipulate-specific mouse actions
	// Note (greebo): Don't know if these should really be public
	SelectObserver _selectObserver;
  	ManipulateObserver _manipulateObserver;

	// Constructor
  	RadiantWindowObserver();

  	// Release this window observer, as this class is usually instanced with "new" on the heap
	void release() {
		delete this;
	}

	// Pass the view reference to the handler subclasses
	void setView(const render::View& view);

	// Tells the observer which widget it is actually observing
	void addObservedWidget(wxWindow& observed);
	void removeObservedWidget(wxWindow& observed);

	// Pass the rectangle callback function to the selector subclass
	void setRectangleDrawCallback(const selection::Rectangle::Callback& callback);

	// greebo: This is called if the window size changes (camera, orthoview)
	void onSizeChanged(int width, int height);

	// Handles the mouseDown event, basically determines which action should be performed (select or manipulate)
	void onMouseDown(const WindowVector& position, wxMouseEvent& ev);
	
  	/* greebo: Handle the mouse movement. This notifies the registered mouseMove callback
  	 * and resets the cycle selection counter
  	 */
	void onMouseMotion(const WindowVector& position, unsigned int state);

	/* greebo: Handle the mouseUp event. Usually, this means the end of an operation, so
	 * this has to check, if there are any callbacks connected, and call them if this is the case
	 */
	void onMouseUp(const WindowVector& position, wxMouseEvent& ev);

	// Cancels the current operation and disconnects the mouse handlers
	void cancelOperation();

private:
	// The callback for catching the cancel-event (ESC-key)
	void onWxKeyPress(wxKeyEvent& ev);

	void handleMouseDown(const WindowVector& position, ui::ObserverEvent observerEvent);
	void handleMouseUp(const WindowVector& position, ui::ObserverEvent observerEvent, wxMouseEvent* wxEvent);

}; // class RadiantWindowObserver

// Allocates a new Observer on the heap and returns the pointer
SelectionSystemWindowObserver* NewWindowObserver();
