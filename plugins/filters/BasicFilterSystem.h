#ifndef BASICFILTERSYSTEM_H_
#define BASICFILTERSYSTEM_H_

#include "XMLFilter.h"
#include "imodule.h"
#include "ifilter.h"

#include <map>
#include <vector>
#include <string>
#include <iostream>

namespace filters
{

/** FilterSystem implementation class.
 */

class BasicFilterSystem 
: public FilterSystem
{
	// Hashtable of available filters, indexed by name
	typedef std::map<std::string, filters::XMLFilter> FilterTable;
	FilterTable _availableFilters;
	
	// Second table containing just the active filters
	FilterTable _activeFilters;

	// Cache of visibility flags for item names, to avoid having to
	// traverse the active filter list for each lookup
	typedef std::map<std::string, bool> StringFlagCache;
	StringFlagCache _visibilityCache;

private:
	
	// Perform a traversal of the scenegraph, setting or clearing the filtered
	// flag on Nodes depending on their entity class
	void updateScene();

	void updateShaders();
	
public:
	
	// Invoke the InstanceUpateWalker to update the filtered status.
	void update();

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
	
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
	virtual void shutdownModule();
};
typedef boost::shared_ptr<BasicFilterSystem> BasicFilterSystemPtr;

} // namespace filters

#endif /*BASICFILTERSYSTEM_H_*/
