#ifndef GLOBALXYWND_H_
#define GLOBALXYWND_H_

#include "iclipper.h"
#include <list>
#include "XYWnd.h"

	namespace {
		typedef std::list<XYWnd*> XYWndList;
	}

class XYWndManager
{
	// The list containing the pointers to all the allocated views
	XYWndList _XYViews;

	XYWnd* _activeXY;

public:

	// Constructor
	XYWndManager();
	
	// Destructor, calls destroy to free all remaining views
	~XYWndManager();
	
	// Passes a queueDraw() call to each allocated view
	void updateAllViews();
	
	// Free all the allocated views from the heap
	void destroy();
	
	XYWnd* getActiveXY() const;
	void setActiveXY(XYWnd* wnd);
	
	// Sets the origin of all available views
	void setOrigin(const Vector3& origin);
	
	// Sets the scale of all available views
	void setScale(float scale);
	
	// Positions the view of all available views / the active view
	void positionAllViews(const Vector3& origin);
	void positionView(const Vector3& origin);
	
	// Returns the view type of the currently active view
	EViewType getActiveViewType() const;
	void setActiveViewType(EViewType viewType);
	
	// Retrieves the pointer to the first view matching the given view type
	// @returns: NULL if no matching window could be found, the according pointer otherwise 
	XYWnd* getView(EViewType viewType);

	// Allocates a new XY view on the heap and returns its pointer
	XYWnd* createXY();

}; // class XYWndManager

// Use this method to access the global XYWnd manager class
XYWndManager& GlobalXYWnd();

#endif /*GLOBALXYWND_H_*/
