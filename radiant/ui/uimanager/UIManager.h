#ifndef UIMANAGER_H_
#define UIMANAGER_H_

#include "iuimanager.h"

#include "MenuManager.h"

namespace ui {

class UIManager :
	public IUIManager
{
	// Local helper class taking care of the menu
	MenuManager _menuManager;
public:
	UIManager();
	
	/** greebo: Retrieves the menu bar with the given name.
	 */
	GtkWidget* getMenu(const std::string& name);

	/** greebo: Adds the given menuitem as child to the given path.
	 * 			<caption> is used as display string (can contain a mnemonic).
	 */
	void addMenuItem(const std::string& menuPath, 
					 const std::string& caption, 
					 const std::string& eventName);
}; // class UIManager

} // namespace ui

#endif /*UIMANAGER_H_*/
