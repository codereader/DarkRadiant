#ifndef UIMANAGER_H_
#define UIMANAGER_H_

#include "imodule.h"
#include "iuimanager.h"

#include "MenuManager.h"
#include "ToolbarManager.h"
#include "StatusBarManager.h"
#include "colourscheme/ColourSchemeManager.h"
#include <iostream>

namespace ui {

class UIManager :
	public IUIManager
{
	// Local helper class taking care of the menu
	MenuManager _menuManager;
	
	ToolbarManager _toolbarManager;

	StatusBarManager _statusBarManager;
public:

	/** greebo: Retrieves the helper class to manipulate the menu.
	 */
	IMenuManager& getMenuManager();
	
	IToolbarManager& getToolbarManager();
	
	IColourSchemeManager& getColourSchemeManager();

	IGroupDialog& getGroupDialog();

	IStatusBarManager& getStatusBarManager();

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
}; // class UIManager
typedef boost::shared_ptr<ui::UIManager> UIManagerPtr;

} // namespace ui

#endif /*UIMANAGER_H_*/
