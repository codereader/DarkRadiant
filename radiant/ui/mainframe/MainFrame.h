#ifndef _MAINFRAME_H_
#define _MAINFRAME_H_

#include "imainframe.h"
#include "imainframelayout.h"
#include "gtkutil/WindowPosition.h"

namespace ui {

class MainFrame :
	public IMainFrame
{
	GtkWindow* _window;

	// The main container (where layouts can start packing stuff into)
	GtkWidget* _mainContainer;

	bool _screenUpdatesEnabled;

	gtkutil::WindowPosition _windowPosition;

	// The current layout object (NULL if no layout active)
	IMainFrameLayoutPtr _currentLayout;

public:
	MainFrame();

	void construct();
	void destroy();

	// IMainFrame implementation
	bool screenUpdatesEnabled();
	void enableScreenUpdates();
	void disableScreenUpdates();

	GtkWindow* getTopLevelWindow();
	GtkWidget* getMainContainer();

	void updateAllWindows();

	// Apply the named viewstyle
	void applyLayout(const std::string& name);
	std::string getCurrentLayout();

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();

private:
	void create();

	void removeLayout();

	// Save/Restore the window position as saved to the registry
	void saveWindowPosition();
	void restoreWindowPosition();

	void shutdown();
	
	// Creates and returns the topmost application window
	GtkWindow* createTopLevelWindow();
	GtkWidget* createMenuBar();
	
	static gboolean onDelete(GtkWidget* widget, GdkEvent* ev, MainFrame* self);
};

} // namespace ui

#endif /* _MAINFRAME_H_ */
