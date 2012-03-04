#ifndef GLOBALXYWND_H_
#define GLOBALXYWND_H_

#include <list>

#include "iorthoview.h"
#include "iclipper.h"
#include "iregistry.h"
#include "icommandsystem.h"

#include "XYWnd.h"

class XYWndManager : public IXWndManager
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

	Glib::RefPtr<Gtk::Window> _globalParentWindow;

private:

	// Get a unique ID for the XYWnd map
	int getUniqueID() const;
    void observeKey(const std::string&);
	void refreshFromRegistry();

public:

	// Constructor
	XYWndManager();

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
	void setActiveViewXY(const cmd::ArgumentList& args); // top view
	void setActiveViewXZ(const cmd::ArgumentList& args); // side view
	void setActiveViewYZ(const cmd::ArgumentList& args); // front view
	void splitViewFocus(const cmd::ArgumentList& args); // Re-position all available views
	void zoom100(const cmd::ArgumentList& args); // Sets the scale of all windows to 1
	void focusActiveView(const cmd::ArgumentList& args); // sets the focus of the active view

	// Sets the origin of all available views
	void setOrigin(const Vector3& origin);

	// Sets the scale of all available views
	void setScale(float scale);

	// Zooms the currently active view in/out
	void zoomIn(const cmd::ArgumentList& args);
	void zoomOut(const cmd::ArgumentList& args);

	// Positions the view of all available views / the active view
	void positionAllViews(const Vector3& origin);
	void positionActiveView(const Vector3& origin);

	// Returns the view type of the currently active view
	EViewType getActiveViewType() const;
	void setActiveViewType(EViewType viewType);

	void toggleActiveView(const cmd::ArgumentList& args);

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
	void createXYFloatingOrthoView(const cmd::ArgumentList& args);

	/**
	 * greebo: This removes a certain orthoview ID, usually initiating
	 * destruction of the XYWnd/FloatingOrthoView object.
	 */
	void destroyXYWnd(int id);

	// Determines the global parent the xyviews are children of
	void setGlobalParentWindow(const Glib::RefPtr<Gtk::Window>& globalParentWindow);

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();

private:
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
