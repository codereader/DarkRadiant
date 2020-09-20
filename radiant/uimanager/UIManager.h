#pragma once

#include "imodule.h"
#include "iradiant.h"
#include "iuimanager.h"
#include "idialogmanager.h"

#include "menu/MenuManager.h"
#include "ToolbarManager.h"
#include "StatusBarManager.h"
#include "DialogManager.h"
#include "wxutil/event/SingleIdleCallback.h"
#include <sigc++/connection.h>

#include <memory>

namespace ui
{

class LocalBitmapArtProvider;

class UIManager :
	public IUIManager,
	public std::enable_shared_from_this<UIManager>,
	protected wxutil::SingleIdleCallback
{
    // Sub-manager classes, constructed in initialiseModule to avoid being
    // called before the main window is ready.
    std::shared_ptr<MenuManager> _menuManager;
    std::shared_ptr<ToolbarManager> _toolbarManager;
    std::shared_ptr<StatusBarManager> _statusBarManager;
	DialogManagerPtr _dialogManager;

	LocalBitmapArtProvider* _bitmapArtProvider;

	sigc::connection _selectionChangedConn;
	sigc::connection _countersChangedConn;

public:
	UIManager() :
		_bitmapArtProvider(nullptr)
	{}

	/** greebo: Retrieves the helper class to manipulate the menu.
	 */
	IMenuManager& getMenuManager();

	IToolbarManager& getToolbarManager();

	IGroupDialog& getGroupDialog();

	IStatusBarManager& getStatusBarManager();

	IDialogManager& getDialogManager();

	IFilterMenuPtr createFilterMenu();

	const std::string& ArtIdPrefix() const;

	// Called on radiant shutdown
	void clear();

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const IApplicationContext& ctx);
	void shutdownModule();

protected:
	void onIdle() override;

private:
	void updateCounterStatusBar();
};

} // namespace ui
