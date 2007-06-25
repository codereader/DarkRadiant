#ifndef BASICFILTERSYSTEM_H_
#define BASICFILTERSYSTEM_H_

#include "XMLFilter.h"
#include "ifilter.h"

#include <map>
#include <vector>
#include <string>

namespace filters
{

/** FilterSystem implementation class.
 */

class BasicFilterSystem 
: public FilterSystem
{
public:

	// Radiant Module stuff
	typedef FilterSystem Type;
	STRING_CONSTANT(Name, "*");

	// Return the static instance
	FilterSystem* getTable() {
		return this;
	}

private:

	// Flag to indicate initialisation status
	bool _initialised;

	// Hashtable of available filters, indexed by name
	typedef std::map<std::string, filters::XMLFilter> FilterTable;
	FilterTable _availableFilters;
	
	// Second table containing just the active filters
	FilterTable _activeFilters;

	// Cache of visibility flags for item names, to avoid having to
	// traverse the active filter list for each lookup
	typedef std::map<std::string, bool> StringFlagCache;
	StringFlagCache _visibilityCache;

public:
	
	// Constructor
	BasicFilterSystem();

	// Initialise the filter system. This must be done after the main
	// Radiant module, hence cannot be done in the constructor.
	void initialise();

	// Filter system visit function
	void forEachFilter(IFilterVisitor& visitor);
	
	std::string getFilterEventName(const std::string& filter);
	
	bool getFilterState(const std::string& filter) {
		return (_activeFilters.find(filter) != _activeFilters.end());
	}
	
	// Set the state of a filter
	void setFilterState(const std::string& filter, bool state);

	// Query whether an item is visible or filtered out
	bool isVisible(const std::string& item, const std::string& name);
};


}

#endif /*BASICFILTERSYSTEM_H_*/
