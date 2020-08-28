#pragma once

#include <map>
#include <sigc++/connection.h>
#include "icommandsystem.h"
#include "imainframe.h"
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

	sigc::connection _mapNameChangedConn;
	sigc::connection _mapModifiedChangedConn;

	sigc::signal<void> _sigMainFrameConstructed;
	sigc::signal<void> _sigMainFrameReady;
	sigc::signal<void> _sigMainFrameShuttingDown;

private:
	void keyChanged();
	void preDestructionCleanup();
	void updateTitle();
    void onTopLevelFrameClose(wxCloseEvent& ev);

public:
	MainFrame();

	void construct() override;

	// IMainFrame implementation
	bool screenUpdatesEnabled() override;
	void enableScreenUpdates() override;
	void disableScreenUpdates() override;

	wxFrame* getWxTopLevelWindow() override;
	bool isActiveApp() override;
	wxBoxSizer* getWxMainContainer() override;
	wxToolBar* getToolbar(Toolbar type) override;

	void updateAllWindows(bool force = false) override;

	// Apply the named viewstyle
	void applyLayout(const std::string& name) override;
    void setActiveLayoutName(const std::string& name) override;
	std::string getCurrentLayout() override;

	IScopedScreenUpdateBlockerPtr getScopedScreenUpdateBlocker(const std::string& title, 
		const std::string& message, bool forceDisplay = false) override;

	sigc::signal<void>& signal_MainFrameConstructed() override;
	sigc::signal<void>& signal_MainFrameReady() override;
	sigc::signal<void>& signal_MainFrameShuttingDown() override;

	// Command to toggle the current layout's camera fullscreen mode
	void toggleFullscreenCameraView(const cmd::ArgumentList& args);

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;
	void shutdownModule() override;

private:
	void create();

	void exitCmd(const cmd::ArgumentList& args);

	void removeLayout();

	// Save/Restore the window position as saved to the registry
	void saveWindowPosition();
	void restoreWindowPosition();

	// Creates the topmost application window
	void createTopLevelWindow();

	// Constructs the MainFrame and shows the Window
	void postModuleInitialisation();

#ifdef WIN32
	// Enables or disabled desktop composition, Windows-specific
	void setDesktopCompositionEnabled(bool enabled);
#endif
};

} // namespace ui
