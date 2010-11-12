#ifndef _FILTER_MENU_H_
#define _FILTER_MENU_H_

#include "ifiltermenu.h"
#include "ifilter.h"

namespace ui
{

/** Utility class for generating the Filters top-level menu. This class
 * registers the relevant menuitems on demand.
 *
 * Construct a FiltersMenu instance to generate a new Filter Menu which
 * can be packed into a parent container widget using the GtkWidget* operator.
 */
class FilterMenu :
	public IFilterMenu,
	public IFilterVisitor
{
private:
	Gtk::Widget* _menu;

	// Static counter to create unique menu bar widgets
	static std::size_t _counter;

	// The path of this menu
	std::string _path;

	// The target path used for population
	std::string _targetPath;

public:
	// Constructs the filters submenu including menu bar
	FilterMenu();

	~FilterMenu();

	// Returns a GtkWidget* with a fabricated filters submenu,
	// ready for packing into a menu bar.
	Gtk::Widget* getMenuBarWidget();

	// IFilterVisitor implementation
	void visit(const std::string& filterName);
};

} // namespace

#endif /* _FILTER_MENU_H_ */
