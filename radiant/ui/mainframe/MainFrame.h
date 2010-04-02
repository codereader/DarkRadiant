#ifndef _MAINFRAME_H_
#define _MAINFRAME_H_

#include "icommandsystem.h"
#include "imainframe.h"
#include "iregistry.h"
#include "imainframelayout.h"
#include "gtkutil/WindowPosition.h"

namespace ui {

class MainFrame :
	public IMainFrame,
	public RegistryKeyObserver
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

	// RegistryKeyObserver implementation
	void keyChanged(const std::string& changedKey, const std::string& newValue);

	// Apply the named viewstyle
	void applyLayout(const std::string& name);
	std::string getCurrentLayout();

	// Command to toggle the current layout's camera fullscreen mode
	void toggleFullscreenCameraView(const cmd::ArgumentList& args);

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

#ifdef WIN32
	// Enables or disabled desktop composition, Windows-specific
	void setDesktopCompositionEnabled(bool enabled);
#endif
};

} // namespace ui

#endif /* _MAINFRAME_H_ */
