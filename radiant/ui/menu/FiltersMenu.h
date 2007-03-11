#ifndef FILTERSMENU_H_
#define FILTERSMENU_H_

namespace ui {

/** Utility class for generating the Filters top-level menu. This class
 * registers the relevant menuitems on demand.
 * 
 * This class has been stripped down to the bones after the UIManager had been 
 * introduced, this code can probably be incorporated into something else.
 */
class FiltersMenu
{
public:
	/** Adds the menuitems to the UIManager
	 */
	static void addItems();
};

}

#endif /*FILTERSMENU_H_*/
