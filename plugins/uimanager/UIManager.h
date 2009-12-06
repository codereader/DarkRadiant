#ifndef UIMANAGER_H_
#define UIMANAGER_H_

#include "imodule.h"
#include "iuimanager.h"
#include "idialogmanager.h"

#include "MenuManager.h"
#include "ToolbarManager.h"
#include "StatusBarManager.h"
#include "DialogManager.h"
#include "colourscheme/ColourSchemeManager.h"
#include <iostream>

namespace ui {

class UIManagerShutdownListener;
typedef boost::shared_ptr<UIManagerShutdownListener> UIManagerShutdownListenerPtr;

class UIManager :
	public IUIManager
{
	// Local helper class taking care of the menu
	MenuManager _menuManager;
	
	ToolbarManager _toolbarManager;

	StatusBarManager _statusBarManager;

	UIManagerShutdownListenerPtr _shutdownListener;

	DialogManagerPtr _dialogManager;

public:

	/** greebo: Retrieves the helper class to manipulate the menu.
	 */
	IMenuManager& getMenuManager();
	
	IToolbarManager& getToolbarManager();
	
	IColourSchemeManager& getColourSchemeManager();

	IGroupDialog& getGroupDialog();

	IStatusBarManager& getStatusBarManager();

	IDialogManager& getDialogManager();

	// Called on radiant shutdown
	void clear();

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
}; // class UIManager
typedef boost::shared_ptr<ui::UIManager> UIManagerPtr;

} // namespace ui

#endif /*UIMANAGER_H_*/
