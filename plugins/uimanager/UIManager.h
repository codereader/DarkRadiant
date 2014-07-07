#pragma once

#include "imodule.h"
#include "iradiant.h"
#include "iuimanager.h"
#include "idialogmanager.h"

#include "MenuManager.h"
#include "ToolbarManager.h"
#include "StatusBarManager.h"
#include "DialogManager.h"
#include "colourscheme/ColourSchemeManager.h"
#include <boost/enable_shared_from_this.hpp>

namespace ui
{

class LocalBitmapArtProvider;

class UIManager :
	public IUIManager,
	public boost::enable_shared_from_this<UIManager>
{
private:
	// Local helper class taking care of the menu
	MenuManager _menuManager;

	ToolbarManager _toolbarManager;

	StatusBarManager _statusBarManager;

	DialogManagerPtr _dialogManager;

	LocalBitmapArtProvider* _bitmapArtProvider;

public:
	UIManager() :
		_bitmapArtProvider(NULL)
	{}

	/** greebo: Retrieves the helper class to manipulate the menu.
	 */
	IMenuManager& getMenuManager();

	IToolbarManager& getToolbarManager();

	IColourSchemeManager& getColourSchemeManager();

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
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();
}; // class UIManager
typedef boost::shared_ptr<ui::UIManager> UIManagerPtr;

} // namespace ui
