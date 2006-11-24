#ifndef RADIANTWINDOWOBSERVER_H_
#define RADIANTWINDOWOBSERVER_H_

#include "view.h"
#include "surfacedialog.h" // For apply/copy texture
#include "ButtonsModifiers.h"

#include "Device.h"
#include "SelectObserver.h"
#include "ManipulateObserver.h"
#include "SelectionTest.h"

// Abstract base class of the SelectionSystem Observer
class SelectionSystemWindowObserver : public WindowObserver {
public:
  virtual void setView(const View& view) = 0;
  virtual void setRectangleDrawCallback(const RectangleCallback& callback) = 0;
};

// ====================================================================================

/* greebo: This is the hub class that observes a view, the implementation of the abstract base class above. 
 * It basically checks all "incoming" mouse clicks and passes them to the according 
 * subclasses like Selector_ and ManipulateObserver, these in turn pass them to the RadiantSelectionSystem */
class RadiantWindowObserver : public SelectionSystemWindowObserver {
	// The tolerance when it comes to the construction of selection boxes
	enum {
		SELECT_EPSILON = 8,
  	};

	// The window dimensions
	int _width;
  	int _height;

	// This is true, if the "select" mouse button is currently pressed (important to know for drag operations)
  	bool _mouseDown;

public:
	// These are the classes that handle the selection- and manipulate-specific mouse actions
	// Note (greebo): Don't know if these should really be public
	SelectObserver _selectObserver;
  	ManipulateObserver _manipulateObserver;

	// Constructor
  	RadiantWindowObserver() : _mouseDown(false) {}

  	// Release this window observer, as this class is usually instanced with "new" on the heap
	void release() {
		delete this;
	}

	// Pass the view reference to the handler subclasses 
	void setView(const View& view);
	
	// Pass the rectangle callback function to the selector subclass
	void setRectangleDrawCallback(const RectangleCallback& callback);
	
	// greebo: This is called if the window size changes (camera, orthoview)
	void onSizeChanged(int width, int height);
  
	// Handles the mouseDown event, basically determines which action should be performed (select or manipulate)
	void onMouseDown(const WindowVector& position, ButtonIdentifier button, ModifierFlags modifiers);
	
  	/* greebo: Handle the mouse movement. This notifies the registered mouseMove callback
  	 * and resets the cycle selection counter 
  	 */
	void onMouseMotion(const WindowVector& position, ModifierFlags modifiers);
	
	/* greebo: Handle the mouseUp event. Usually, this means the end of an operation, so
	 * this has to check, if there are any callbacks connected, and call them if this is the case
	 */
  	void onMouseUp(const WindowVector& position, ButtonIdentifier button, ModifierFlags modifiers);
  	
  	// Called upon modifier key press/release, updates the interal bit mask accordingly
  	void onModifierDown(ModifierFlags type);
  	void onModifierUp(ModifierFlags type);
}; // class RadiantWindowObserver


// Allocates a new Observer on the heap and returns the pointer 
SelectionSystemWindowObserver* NewWindowObserver();

#endif /*RADIANTWINDOWOBSERVER_H_*/
