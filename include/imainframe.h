#pragma once

#include "imodule.h"

const std::string MODULE_MAINFRAME("MainFrame");

// Forward declaration
namespace Gtk
{
	class Widget;
	class Toolbar;
}

#include <gtkmm/window.h>

/**
 * Scoped object to block screen updates and display a modal message,
 * used while reloading shaders, for instance.
 */
class IScopedScreenUpdateBlocker
{
public:
	virtual ~IScopedScreenUpdateBlocker() {}
};
typedef boost::shared_ptr<IScopedScreenUpdateBlocker> IScopedScreenUpdateBlockerPtr;

/**
 * The MainFrame represents the top-level application window.
 */
class IMainFrame :
	public RegisterableModule
{
public:
	// Constructs the toplevel mainframe window and issues the "radiant startup" signal
	virtual void construct() = 0;

	// Destroys the toplevel mainframe window and issues the "radiant shutdown" signal
	virtual void destroy() = 0;

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
	virtual const Glib::RefPtr<Gtk::Window>& getTopLevelWindow() = 0;

	/**
	 * Returns TRUE if DarkRadiant is currently "in focus", i.e. the app in the foreground.
	 */
	virtual bool isActiveApp() = 0;

	/**
	 * greebo: Returns the main container widget (a vbox), where layouts
	 * can start packing widgets into. This resembles the large grey area
	 * in the main window.
	 * May return NULL if mainframe is not constructed yet.
	 */
	virtual Gtk::Container* getMainContainer() = 0;

	enum Toolbar
	{
		TOOLBAR_HORIZONTAL,	// the "view" toolbar (on the top)
		TOOLBAR_VERTICAL,	// the "edit" toolbar (on the left)
	};

	/**
	 * greebo: Returns a toolbar widget, as specified by the
	 * passed enum value.
	 */
	virtual Gtk::Toolbar* getToolbar(Toolbar type) = 0;

	/**
	 * Updates all viewports which are child of the toplevel window.
	 */
	virtual void updateAllWindows() = 0;

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
};

// This is the accessor for the mainframe module
inline IMainFrame& GlobalMainFrame() {
	// Cache the reference locally
	static IMainFrame& _mainFrame(
		*boost::static_pointer_cast<IMainFrame>(
			module::GlobalModuleRegistry().getModule(MODULE_MAINFRAME)
		)
	);
	return _mainFrame;
}
