#pragma once

#include <map>
#include "icommandsystem.h"
#include "imainframe.h"
#include "iregistry.h"
#include "imainframelayout.h"
#include "gtkutil/WindowPosition.h"
#include <gtkmm/window.h>
#include <gtkmm/box.h>
#include <gtkmm/toolbar.h>

namespace ui
{

class MainFrame : public IMainFrame
{
	// The top-level window
	Glib::RefPtr<Gtk::Window> _window;

	// The main container (where layouts can start packing stuff into)
	Gtk::VBox* _mainContainer;

	bool _screenUpdatesEnabled;

	gtkutil::WindowPosition _windowPosition;

	// The current layout object (NULL if no layout active)
	IMainFrameLayoutPtr _currentLayout;

	typedef std::map<Toolbar, Gtk::Toolbar*> ToolbarMap;
	ToolbarMap _toolbars;

private:
	void keyChanged();

public:
	MainFrame();

	void construct();
	void destroy();

	// IMainFrame implementation
	bool screenUpdatesEnabled();
	void enableScreenUpdates();
	void disableScreenUpdates();

	const Glib::RefPtr<Gtk::Window>& getTopLevelWindow();
	bool isActiveApp();
	Gtk::Container* getMainContainer();
	Gtk::Toolbar* getToolbar(Toolbar type);

	void updateAllWindows();

	// Apply the named viewstyle
	void applyLayout(const std::string& name);
    void setActiveLayoutName(const std::string& name);
	std::string getCurrentLayout();

	IScopedScreenUpdateBlockerPtr getScopedScreenUpdateBlocker(const std::string& title, 
		const std::string& message, bool forceDisplay = false);

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

	// Creates the topmost application window
	void createTopLevelWindow();
	Gtk::Widget* createMenuBar();

	// Signal callback
	bool onDeleteEvent(GdkEventAny* ev);

#ifdef WIN32
	// Enables or disabled desktop composition, Windows-specific
	void setDesktopCompositionEnabled(bool enabled);
#endif
};

} // namespace ui
