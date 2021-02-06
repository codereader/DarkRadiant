#pragma once

#include <map>
#include "ifilter.h"
#include "wxutil/menu/PopupMenu.h"

namespace wxutil
{

/** 
 * Utility class for generating a Filters fly-out menu. 
 * Provides a menu with a check button for each of the registered filters.
 */
class FilterPopupMenu :
    public wxutil::PopupMenu
{
private:
	std::map<std::string, wxMenuItem*> _filterItems;

public:
	// Constructs the filter items
    FilterPopupMenu();

	virtual ~FilterPopupMenu();

private:
	void visitFilter(const std::string& filterName);
};

} // namespace
