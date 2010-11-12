#ifndef FILTERSMENU_H_
#define FILTERSMENU_H_

namespace ui
{

/**
 * Utility class for generating the Filters top-level menu. This class
 * registers the relevant menuitems on demand.
 */
class FiltersMenu
{
public:
	/** Public service method. Adds the menuitems to the global Menu.
	 *  Should be called by the Mainframe window only (and only once).
	 */
	static void addItemsToMainMenu();

	/**
	 * Removes all filter menu items from the menu.
	 */
	static void removeItemsFromMainMenu();
};

} // namespace

#endif /*FILTERSMENU_H_*/
