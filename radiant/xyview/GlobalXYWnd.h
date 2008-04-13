#ifndef GLOBALXYWND_H_
#define GLOBALXYWND_H_

#include <list>

#include "iclipper.h"
#include "iregistry.h"

#include "XYWnd.h"

	namespace {
		const std::string RKEY_XYVIEW_ROOT = "user/ui/xyview";
		
		const std::string RKEY_CHASE_MOUSE = RKEY_XYVIEW_ROOT + "/chaseMouse";
		const std::string RKEY_CAMERA_XY_UPDATE = RKEY_XYVIEW_ROOT + "/camXYUpdate";
		const std::string RKEY_SHOW_CROSSHAIRS = RKEY_XYVIEW_ROOT + "/showCrossHairs";
		const std::string RKEY_SHOW_GRID = RKEY_XYVIEW_ROOT + "/showGrid";
		const std::string RKEY_SHOW_SIZE_INFO = RKEY_XYVIEW_ROOT + "/showSizeInfo";
		const std::string RKEY_SHOW_ENTITY_ANGLES = RKEY_XYVIEW_ROOT + "/showEntityAngles";
		const std::string RKEY_SHOW_ENTITY_NAMES = RKEY_XYVIEW_ROOT + "/showEntityNames";
		const std::string RKEY_SHOW_BLOCKS = RKEY_XYVIEW_ROOT + "/showBlocks";
		const std::string RKEY_SHOW_COORDINATES = RKEY_XYVIEW_ROOT + "/showCoordinates";
		const std::string RKEY_SHOW_OUTLINE = RKEY_XYVIEW_ROOT + "/showOutline";
		const std::string RKEY_SHOW_AXES = RKEY_XYVIEW_ROOT + "/showAxes";
		const std::string RKEY_SHOW_WORKZONE = RKEY_XYVIEW_ROOT + "/showWorkzone";
		const std::string RKEY_DEFAULT_BLOCKSIZE = "user/ui/xyview/defaultBlockSize";
		const std::string RKEY_TRANSLATE_CONSTRAINED = "user/ui/xyview/translateConstrained";
		const std::string RKEY_HIGHER_ENTITY_PRIORITY = "user/ui/xyview/higherEntitySelectionPriority";
		
	}

class XYWndManager : 
	public RegistryKeyObserver
{
	// Store an indexed map of XYWnds. When one is deleted, it will notify
	// the XYWndManager of its index so that it can be removed from the map
	typedef std::map<int, XYWndPtr> XYWndMap;
	XYWndMap _xyWnds;

	// The active XYWnd
	XYWndPtr _activeXY;
	
	// True, if the view is moved when the mouse cursor exceeds the view window borders
	bool _chaseMouse;
	
	bool _camXYUpdate;
	
	// The various display settings for xyviews 
	bool _showCrossHairs;
	bool _showGrid;
	bool _showSizeInfo;
	bool _showBlocks;
	bool _showCoordinates;
	bool _showOutline;
	bool _showAxes;
	bool _showWorkzone;
	
	unsigned int _defaultBlockSize;
	
	GtkWindow* _globalParentWindow;
	
private:
	
	// Get a unique ID for the XYWnd map
	int getUniqueID() const;
	
public:

	// Constructor
	XYWndManager();
	
	// The callback that gets called on registry key changes
	void keyChanged(const std::string& key, const std::string& val);
	
	// Returns the state of the xy view preferences
	bool chaseMouse() const;
	bool camXYUpdate() const;
	bool showCrossHairs() const;
	bool showGrid() const;
	bool showBlocks() const;
	bool showCoordinates() const;
	bool showOutline() const;
	bool showAxes() const;
	bool showWorkzone() const;
	bool showSizeInfo() const;
	bool higherEntitySelectionPriority() const;
	
	unsigned int defaultBlockSize() const;
	
	// Passes a queueDraw() call to each allocated view
	void updateAllViews();
	
	// Register the commands and capture the renderer states
	void construct();
	
	// Release the shader states
	void destroy();
	
	// Free all the allocated views from the heap
	void destroyViews();
	
	// Saves the current state of all open views to the registry
	void saveState();
	// Restores the xy windows according to the state saved in the XMLRegistry 
	void restoreState();
	
	XYWndPtr getActiveXY() const;
	
	/**
	 * Set the given XYWnd to active state.
	 * 
	 * @param id
	 * Unique ID of the XYWnd to set as active.
	 */
	void setActiveXY(int id);
	
	// Shortcut commands for connect view the EventManager
	void setActiveViewXY(); // top view
	void setActiveViewXZ(); // side view
	void setActiveViewYZ(); // front view
	void splitViewFocus(); // Re-position all available views
	void zoom100(); // Sets the scale of all windows to 1
	void focusActiveView(); // sets the focus of the active view
	
	// Sets the origin of all available views
	void setOrigin(const Vector3& origin);
	
	// Sets the scale of all available views
	void setScale(float scale);
	
	// Zooms the currently active view in/out
	void zoomIn();
	void zoomOut();
	
	// Positions the view of all available views / the active view
	void positionAllViews(const Vector3& origin);
	void positionView(const Vector3& origin);
	
	// Returns the view type of the currently active view
	EViewType getActiveViewType() const;
	void setActiveViewType(EViewType viewType);
	
	void toggleActiveView();
	
	// Retrieves the pointer to the first view matching the given view type
	// @returns: NULL if no matching window could be found, the according pointer otherwise 
	XYWndPtr getView(EViewType viewType);

	/**
	 * Create a non-floating (embedded) ortho view.
	 */
	XYWndPtr createEmbeddedOrthoView();
	
	/**
	 * Create a new floating ortho view, as a child of the main window.
	 */
	XYWndPtr createFloatingOrthoView(EViewType viewType);
	
	/**
	 * Parameter-less wrapper for createFloatingOrthoView(), for use by the
	 * event manager. The default orientation of XY is used.
	 */
	void createXYFloatingOrthoView();
	
	/**
	 * Invoked by the PersistentTransientWindow when a floating XY window is
	 * destroyed, so that the XYWnd can be removed from the map.
	 * 
	 * @param index
	 * The unique ID of the destroyed window.
	 */
	void notifyXYWndDestroy(int index);
	
	// Determines the global parent the xyviews are children of
	void setGlobalParentWindow(GtkWindow* globalParentWindow);
	
	/* greebo: This function determines the point currently being "looked" at, it is used for toggling the ortho views
	 * If something is selected the center of the selection is taken as new origin, otherwise the camera
	 * position is considered to be the new origin of the toggled orthoview. */
	Vector3 getFocusPosition();
	
	// Construct the orthoview preference page and add it to the given group
	void constructPreferences();
	
	// Registers all the XY commands in the EventManager 
	void registerCommands();
	
}; // class XYWndManager

// Use this method to access the global XYWnd manager class
XYWndManager& GlobalXYWnd();

#endif /*GLOBALXYWND_H_*/
