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
#include <map>

namespace ui {

class UIManagerShutdownListener;
typedef boost::shared_ptr<UIManagerShutdownListener> UIManagerShutdownListenerPtr;

class UIManager :
	public IUIManager
{
private:
	// Local helper class taking care of the menu
	MenuManager _menuManager;
	
	ToolbarManager _toolbarManager;

	StatusBarManager _statusBarManager;

	UIManagerShutdownListenerPtr _shutdownListener;

	DialogManagerPtr _dialogManager;

	typedef std::map<std::string, GdkPixbuf*> PixBufMap;
	PixBufMap _localPixBufs;
	PixBufMap _localPixBufsWithMask;

public:

	/** greebo: Retrieves the helper class to manipulate the menu.
	 */
	IMenuManager& getMenuManager();
	
	IToolbarManager& getToolbarManager();
	
	IColourSchemeManager& getColourSchemeManager();

	IGroupDialog& getGroupDialog();

	IStatusBarManager& getStatusBarManager();

	IDialogManager& getDialogManager();

	GdkPixbuf* getLocalPixbuf(const std::string& fileName);
	GdkPixbuf* getLocalPixbufWithMask(const std::string& fileName);

	IFilterMenuPtr createFilterMenu();
	IModelPreviewPtr createModelPreview();

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

#endif /*UIMANAGER_H_*/
