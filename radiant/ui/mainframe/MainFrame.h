#pragma once

#include <map>
#include "icommandsystem.h"
#include "imainframe.h"
#include "iregistry.h"
#include "imainframelayout.h"
#include "wxutil/WindowPosition.h"

namespace ui
{

class TopLevelFrame;

class MainFrame : 
	public IMainFrame
{
private:
	TopLevelFrame* _topLevelWindow;

	bool _screenUpdatesEnabled;

	wxutil::WindowPosition _windowPosition;

	// The current layout object (NULL if no layout active)
	IMainFrameLayoutPtr _currentLayout;

private:
	void keyChanged();
	void preDestructionCleanup();
    void onTopLevelFrameClose(wxCloseEvent& ev);

public:
	MainFrame();

	void construct();

	// IMainFrame implementation
	bool screenUpdatesEnabled();
	void enableScreenUpdates();
	void disableScreenUpdates();

	wxFrame* getWxTopLevelWindow();
	bool isActiveApp();
	wxBoxSizer* getWxMainContainer();
	wxToolBar* getToolbar(Toolbar type);

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

	// Creates the topmost application window
	void createTopLevelWindow();

#ifdef WIN32
	// Enables or disabled desktop composition, Windows-specific
	void setDesktopCompositionEnabled(bool enabled);
#endif
};

} // namespace ui
