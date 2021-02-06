#pragma once

#include <map>
#include "ifilter.h"
#include "wxutil/menu/PopupMenu.h"

namespace wxutil
{

/** Utility class for generating the Filters top-level menu. This class
 * registers the relevant menuitems on demand.
 *
 * Construct a FiltersMenu instance to generate a new Filter Menu which
 * can be packed into a parent container widget using the getMenuWidget().
 */
class FilterPopupMenu
{
private:
	std::map<std::string, wxMenuItem*> _filterItems;

	wxutil::PopupMenu* _menu;

public:
	// Constructs the filter items
    FilterPopupMenu();

	~FilterPopupMenu();

	// Returns a wxMenu* with a fabricated filters submenu,
	// ready for packing into a menu bar.
	wxMenu* getMenuWidget();

private:
	void visitFilter(const std::string& filterName);
};

} // namespace
