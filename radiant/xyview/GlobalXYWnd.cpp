#include "GlobalXYWnd.h"

// Constructor
XYWndManager::XYWndManager() :
	_activeXY(NULL)
{
	
}

// Destructor
XYWndManager::~XYWndManager() {
	destroy();
}

void XYWndManager::updateAllViews() {
	for (XYWndList::iterator i = _XYViews.begin(); i != _XYViews.end(); i++) {
		XYWnd* xyview = *i;
		
		// Pass the call
		xyview->queueDraw();
	}
}

// Free the allocated XYViews from the heap
void XYWndManager::destroy() {
	for (XYWndList::iterator i = _XYViews.begin(); i != _XYViews.end(); i++) {
		// Free the view from the heap
		XYWnd* xyView = *i;
		delete xyView;
		
		// Delete the xyview from the list
		_XYViews.erase(i);
	}
}

XYWnd* XYWndManager::getActiveXY() const {
	return _activeXY;
}

void XYWndManager::setOrigin(const Vector3& origin) {
	// Cycle through the list of views and set the origin 
	for (XYWndList::iterator i = _XYViews.begin(); i != _XYViews.end(); i++) {
		XYWnd* xyView = *i;
		
		if (xyView != NULL) {
			// Pass the call
			xyView->setOrigin(origin);
		}
	}
}

void XYWndManager::setScale(float scale) {
	// Cycle through the list of views and set the origin 
	for (XYWndList::iterator i = _XYViews.begin(); i != _XYViews.end(); i++) {
		XYWnd* xyView = *i;
		
		if (xyView != NULL) {
			// Pass the call
			xyView->setScale(scale);
		}
	}
}

void XYWndManager::positionAllViews(const Vector3& origin) {
	// Cycle through the list of views and set the origin 
	for (XYWndList::iterator i = _XYViews.begin(); i != _XYViews.end(); i++) {
		XYWnd* xyView = *i;
		
		if (xyView != NULL) {
			// Pass the call
			xyView->positionView(origin);
		}
	}
}

void XYWndManager::positionView(const Vector3& origin) {
	if (_activeXY != NULL) {
		return _activeXY->positionView(origin);
	}
}

EViewType XYWndManager::getActiveViewType() const {
	if (_activeXY != NULL) {
		return _activeXY->getViewType();
	}
	// Return at least anything
	return XY;
}

void XYWndManager::setActiveViewType(EViewType viewType) {
	if (_activeXY != NULL) {
		return _activeXY->setViewType(viewType);
	}
}

XYWnd* XYWndManager::getView(EViewType viewType) {
	// Cycle through the list of views and get the one matching the type 
	for (XYWndList::iterator i = _XYViews.begin(); i != _XYViews.end(); i++) {
		XYWnd* xyView = *i;
		
		if (xyView != NULL) {
			// If the view matches, return the pointer
			if (xyView->getViewType() == viewType) {
				return xyView;
			}
		}
	}

	return NULL;
}

void XYWndManager::setActiveXY(XYWnd* wnd) {
	// Notify the currently active XYView that is has been deactivated
	if (_activeXY != NULL) {
		_activeXY->setActive(false);
	}
	
	// Update the pointer
	_activeXY = wnd;
	
	// Notify the new active XYView about its activation
	if (_activeXY != NULL) {
		_activeXY->setActive(true);
	}
}

XYWnd* XYWndManager::createXY() {
	// Allocate a new window
	XYWnd* newWnd = new XYWnd();
	
	// Add it to the internal list and return the pointer
	_XYViews.push_back(newWnd);
	
	return newWnd;
}

// Accessor function returning a reference to the static instance
XYWndManager& GlobalXYWnd() {
	static XYWndManager _xyWndManager;
	return _xyWndManager;
}
