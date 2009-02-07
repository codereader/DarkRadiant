#ifndef IMAINFRAME_H_
#define IMAINFRAME_H_

#include "imodule.h"

const std::string MODULE_MAINFRAME("MainFrame");

// Forward declaration
typedef struct _GtkWindow GtkWindow;
typedef struct _GtkWidget GtkWidget;

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
	virtual GtkWindow* getTopLevelWindow() = 0;

	/**
	 * greebo: Returns the main container widget (a vbox), where layouts
	 * can start packing widgets into. This resembles the large grey area
	 * in the main window.
	 * May return NULL if mainframe is not constructed yet.
	 */
	virtual GtkWidget* getMainContainer() = 0;

	/**
	 * Updates all viewports which are child of the toplevel window.
	 */
	virtual void updateAllWindows() = 0;

	/**
	 * Applies the named layout to the MainFrame window. See MainFrameLayout class.
	 */
	virtual void applyLayout(const std::string& name) = 0;

	/**
	 * Returns the name of the currently activated layout or
	 * an empty string if no layout is applied.
	 */
	virtual std::string getCurrentLayout() = 0;
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

#endif /* IMAINFRAME_H_ */
