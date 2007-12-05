/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if !defined(INCLUDED_IFILTER_H)
#define INCLUDED_IFILTER_H

#include "imodule.h"

/** Visitor interface for evaluating the available filters in the 
 * FilterSystem.
 */
 
struct IFilterVisitor {
	// Visit function
	virtual void visit(const std::string& filterName) = 0;
};

const std::string MODULE_FILTERSYSTEM("FilterSystem");

/** Interface for the FilterSystem.
 */
class FilterSystem :
	public RegisterableModule
{
public:

	/**
	 * greebo: Updates all the "Filtered" status of all Instances 
	 *         in the scenegraph based on the current filter settings.         
	 */ 
	virtual void update() = 0;

	/** Visit the available filters, passing each filter's text
	 * name to the visitor.
	 * 
	 * @param visitor
	 * Visitor class implementing the IFilterVisitor interface.
	 */
	virtual void forEachFilter(IFilterVisitor& visitor) = 0;
	
	/** Set the state of the named filter.
	 * 
	 * @param filter
	 * The filter to toggle.
	 * 
	 * @param state
	 * true if the filter should be active, false otherwise.
	 */
	virtual void setFilterState(const std::string& filter, bool state) = 0;

	/** greebo: Returns the state of the given filter.
	 * 
	 * @returns: true or false, depending on the filter state.
	 */
	virtual bool getFilterState(const std::string& filter) = 0;

	/** greebo: Returns the event name of the given filter. This is needed
	 * 			to create the toggle event to menus/etc.
	 */
	virtual std::string getFilterEventName(const std::string& filter) = 0;

	/** Test if a given item should be visible or not, based on the currently-
	 * active filters.
	 * 
	 * @param item
	 * The item to query - "texture", "entityclass" or "object"
	 * 
	 * @param text
	 * String name of the item to query.
	 * 
	 * @returns
	 * true if the item is visible, false otherwise.
	 */
	virtual bool isVisible(const std::string& item, const std::string& text) = 0;

};

inline FilterSystem& GlobalFilterSystem() {
	// Cache the reference locally
	static FilterSystem& _filterSystem(
		*boost::static_pointer_cast<FilterSystem>(
			module::GlobalModuleRegistry().getModule(MODULE_FILTERSYSTEM)
		)
	);
	return _filterSystem;
}

#endif
