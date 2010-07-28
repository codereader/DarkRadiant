#ifndef UIMANAGER_H_
#define UIMANAGER_H_

#include "imodule.h"
#include "iradiant.h"
#include "iuimanager.h"
#include "idialogmanager.h"

#include "MenuManager.h"
#include "ToolbarManager.h"
#include "StatusBarManager.h"
#include "DialogManager.h"
#include "colourscheme/ColourSchemeManager.h"
#include <iostream>
#include <map>
#include <boost/enable_shared_from_this.hpp>

namespace ui {

class UIManager :
	public IUIManager,
	public RadiantEventListener,
	public boost::enable_shared_from_this<UIManager>
{
private:
	// Local helper class taking care of the menu
	MenuManager _menuManager;
	
	ToolbarManager _toolbarManager;

	StatusBarManager _statusBarManager;

	DialogManagerPtr _dialogManager;

	typedef std::map<std::string, Glib::RefPtr<Gdk::Pixbuf>> PixBufMap;
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

	Glib::RefPtr<Gdk::Pixbuf> getLocalPixbuf(const std::string& fileName);
	Glib::RefPtr<Gdk::Pixbuf> getLocalPixbufWithMask(const std::string& fileName);

	IFilterMenuPtr createFilterMenu();
	IModelPreviewPtr createModelPreview();

	// Called on radiant shutdown
	void clear();

	// RadiantEventListener
	void onRadiantShutdown();

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();
}; // class UIManager
typedef boost::shared_ptr<ui::UIManager> UIManagerPtr;

} // namespace ui

#endif /*UIMANAGER_H_*/
