#pragma once

#include "imodule.h"
#include "iradiant.h"
#include "iuimanager.h"
#include "idialogmanager.h"

#include "menu/MenuManager.h"
#include "ToolbarManager.h"
#include "DialogManager.h"

#include <memory>

namespace ui
{

class LocalBitmapArtProvider;

class UIManager :
	public IUIManager,
	public std::enable_shared_from_this<UIManager>
{
    // Sub-manager classes, constructed in initialiseModule to avoid being
    // called before the main window is ready.
    std::shared_ptr<MenuManager> _menuManager;
    std::shared_ptr<ToolbarManager> _toolbarManager;
	DialogManagerPtr _dialogManager;

	LocalBitmapArtProvider* _bitmapArtProvider;

public:
	UIManager() :
		_bitmapArtProvider(nullptr)
	{}

	/** greebo: Retrieves the helper class to manipulate the menu.
	 */
	IMenuManager& getMenuManager() override;

	IToolbarManager& getToolbarManager() override;

	IGroupDialog& getGroupDialog() override;

	IDialogManager& getDialogManager() override;

	const std::string& ArtIdPrefix() const override;

	// Called on radiant shutdown
	void clear();

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const IApplicationContext& ctx);
	void shutdownModule();
};

} // namespace ui
