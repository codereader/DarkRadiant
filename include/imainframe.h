#pragma once

#include "imodule.h"
#include <sigc++/signal.h>

const char* const MODULE_MAINFRAME("MainFrame");

class wxFrame;
class wxToolBar;
class wxBoxSizer;

/**
 * Scoped object to block screen updates and display a modal message,
 * used while reloading shaders, for instance.
 */
class IScopedScreenUpdateBlocker
{
public:
	virtual ~IScopedScreenUpdateBlocker() {}

	// For operations without calculatable duration, call pulse() regularly to
	// provide some visual feedback
	virtual void pulse() = 0;

	// Update the progress fraction [0..1]
	virtual void setProgress(float progress) = 0;

	// Set the status message that might be displayed to the user
	virtual void setMessage(const std::string& message) = 0;
};
typedef std::shared_ptr<IScopedScreenUpdateBlocker> IScopedScreenUpdateBlockerPtr;

/**
 * The MainFrame represents the top-level application window.
 */
class IMainFrame :
	public RegisterableModule
{
public:
	// Returns TRUE if screen updates are enabled
	virtual bool screenUpdatesEnabled() = 0;

	// Use this to (re-)enable camera and xyview draw updates
	virtual void enableScreenUpdates() = 0;

	// Use this to disable camera and xyview draw updates until enableScreenUpdates is called.
	virtual void disableScreenUpdates() = 0;

	/**
	 * Returns the main application window widget. Returns NULL if no window
	 * has been constructed yet.
	 */
	virtual wxFrame* getWxTopLevelWindow() = 0;

	/**
	 * Returns TRUE if DarkRadiant is currently "in focus", i.e. the app in the foreground.
	 */
	virtual bool isActiveApp() = 0;

	/**
	 * greebo: Returns the main container widget (a box sizer), where layouts
	 * can start packing widgets into. This resembles the large grey area
	 * in the main window.
	 * May return NULL if mainframe is not constructed yet.
	 */
	virtual wxBoxSizer* getWxMainContainer() = 0;

    /// Identifiers for application toolbars
    enum class Toolbar
    {
        /// Top horizontal toolbar, containing mostly view-related options
        TOP,

        /// Left vertical toolbar, containing various edit options
        LEFT,

        /// Toolbar above the 3D camera view
        CAMERA
    };

    /// Obtain a pointer to an application toolbar
	virtual wxToolBar* getToolbar(Toolbar toolbarID) = 0;

	/**
	 * Updates all viewports which are child of the toplevel window.
     * Set the force flag to true to redraw immediately insteaf of queueing.
	 */
	virtual void updateAllWindows(bool force = false) = 0;

	/**
	 * Applies the named layout to the MainFrame window. See MainFrameLayout class.
	 */
	virtual void applyLayout(const std::string& name) = 0;

    /// Store the layout name, but do not immediately apply it
    virtual void setActiveLayoutName(const std::string& name) = 0;

	/**
	 * Returns the name of the currently activated layout or
	 * an empty string if no layout is applied.
	 */
	virtual std::string getCurrentLayout() = 0;

	/**
	 * Acquire a screen update blocker object that displays a modal message.
	 * As soon as the object is destroyed screen updates are allowed again.
	 *
	 * Pass the title and the message to display in the small modal window.
	 */
	virtual IScopedScreenUpdateBlockerPtr getScopedScreenUpdateBlocker(const std::string& title,
		const std::string& message, bool forceDisplay = false) = 0;

    /**
     * \brief
     * A signal emitted when the MainFrame window has been set up.
     *
     * Modules can subscribe to this to register any UI parts that require a
     * valid main window or sub component like the group dialog to be
     * constructed. This is a one-time signal, after emission the subscribers
     * will be automatically removed by this class.
     */
    virtual sigc::signal<void>& signal_MainFrameConstructed() = 0;

	/**
	 * Signal fired after the MainFrame window is shown the first time
	 * during application start up.
	 * This is a one-time signal, after emission the subscribers will be
	 * automatically removed by this class.
	 */
	virtual sigc::signal<void>& signal_MainFrameReady() = 0;

	/**
	 * Signal fired when the UI is shutting down, right before the MainFrame
	 * window will be destroyed. Dependant UI modules can listen to this
	 * event to get a chance to clean up and save their state.
	 * This is a one-time signal, after emission the subscribers will be
	 * automatically removed by this class.
	 */
	virtual sigc::signal<void>& signal_MainFrameShuttingDown() = 0;
};

// This is the accessor for the mainframe module
inline IMainFrame& GlobalMainFrame()
{
	static module::InstanceReference<IMainFrame> _reference(MODULE_MAINFRAME);
	return _reference;
}
