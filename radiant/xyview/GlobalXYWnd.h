#ifndef GLOBALXYWND_H_
#define GLOBALXYWND_H_

#include <list>

#include "iclipper.h"
#include "iregistry.h"
#include "preferencesystem.h"

#include "XYWnd.h"

	namespace {
		const std::string RKEY_CHASE_MOUSE = "user/ui/xyview/chaseMouse";
		const std::string RKEY_CAMERA_XY_UPDATE = "user/ui/xyview/camXYUpdate";
		const std::string RKEY_SHOW_CROSSHAIRS = "user/ui/xyview/showCrossHairs";
		const std::string RKEY_SHOW_GRID = "user/ui/xyview/showGrid";

		typedef std::list<XYWnd*> XYWndList;
	}

class XYWndManager : 
	public RegistryKeyObserver,
	public PreferenceConstructor
{
	// The list containing the pointers to all the allocated views
	XYWndList _XYViews;

	XYWnd* _activeXY;
	
	// True, if the view is moved when the mouse cursor exceeds the view window borders
	bool _chaseMouse;
	
	bool _camXYUpdate;
	bool _showCrossHairs;
	bool _showGrid;

public:

	// Constructor
	XYWndManager();
	
	// Destructor, calls destroy to free all remaining views
	~XYWndManager();
	
	// The callback that gets called on registry key changes
	void keyChanged();
	
	// Returns the state of the xy view preferences
	bool chaseMouse() const;
	bool camXYUpdate() const;
	bool showCrossHairs() const;
	bool showGrid() const;
	
	void toggleCrossHairs(); 
	void toggleGrid();
	
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
	
	void toggleActiveView();
	
	// Retrieves the pointer to the first view matching the given view type
	// @returns: NULL if no matching window could be found, the according pointer otherwise 
	XYWnd* getView(EViewType viewType);

	// Allocates a new XY view on the heap and returns its pointer
	XYWnd* createXY();
	
	// Construct the orthoview preference page and add it to the given group
	void constructPreferencePage(PreferenceGroup& group);

}; // class XYWndManager

// Use this method to access the global XYWnd manager class
XYWndManager& GlobalXYWnd();

#endif /*GLOBALXYWND_H_*/
