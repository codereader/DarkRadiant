#pragma once

#include "XMLFilter.h"
#include "imodule.h"
#include "ifilter.h"
#include "icommandsystem.h"
#include "ieventmanager.h"
#include "xmlutil/Node.h"

#include <map>
#include <vector>
#include <string>
#include <iostream>

namespace filters
{

const char* const SELECT_OBJECTS_BY_FILTER_CMD = "SelectObjectsByFilter";
const char* const DESELECT_OBJECTS_BY_FILTER_CMD = "DeselectObjectsByFilter";

/** FilterSystem implementation class.
 */
class BasicFilterSystem : 
	public FilterSystem
{
private:
	// Hashtable of available filters, indexed by name
	typedef std::map<std::string, XMLFilter> FilterTable;
	FilterTable _availableFilters;

	// Second table containing just the active filters
	FilterTable _activeFilters;

	// Cache of visibility flags for item names, to avoid having to
	// traverse the active filter list for each lookup
	typedef std::map<std::string, bool> StringFlagCache;
	StringFlagCache _visibilityCache;

    sigc::signal<void> _filtersChangedSignal;

private:

	// Perform a traversal of the scenegraph, setting or clearing the filtered
	// flag on Nodes depending on their entity class
	void updateScene();

	void updateShaders();

	void updateEvents();

	void addFiltersFromXML(const xml::NodeList& nodes, bool readOnly);

	IEventPtr createEventToggle(XMLFilter& filter);

	void setAllFilterStatesCmd(const cmd::ArgumentList& args);

	void selectObjectsByFilterCmd(const cmd::ArgumentList& args);
	void deselectObjectsByFilterCmd(const cmd::ArgumentList& args);

	void setObjectSelectionByFilter(const std::string& filterName, bool select);

public:
    // FilterSystem implementation
    sigc::signal<void> filtersChangedSignal() const override;

	// Invoke the InstanceUpateWalker to update the filtered status.
	void update() override;

	// Updates the given subgraph
	void updateSubgraph(const scene::INodePtr& root) override;

	// Filter system visit function
	void forEachFilter(IFilterVisitor& visitor) override;

	std::string getFilterEventName(const std::string& filter) override;

	bool getFilterState(const std::string& filter) override;

	// Set the state of a filter
	void setFilterState(const std::string& filter, bool state) override;

	// Query whether an item is visible or filtered out
	bool isVisible(const FilterRule::Type type, const std::string& name) override;

	// Query whether an entity is visible or filtered out
	bool isEntityVisible(const FilterRule::Type type, const Entity& entity) override;

	// Whether this filter is read-only and can't be changed
	bool filterIsReadOnly(const std::string& filter) override;

	// Adds a new filter to the system
	bool addFilter(const std::string& filterName, const FilterRules& ruleSet) override;

	// Removes the filter and returns true on success
	bool removeFilter(const std::string& filter) override;

	// Renames the filter and updates eventmanager
	bool renameFilter(const std::string& oldFilterName, const std::string& newFilterName) override;

	// Returns the ruleset of the named filter
	FilterRules getRuleSet(const std::string& filter) override;

	// Applies the ruleset and replaces the previous one for a given filter.
	bool setFilterRules(const std::string& filter, const FilterRules& ruleSet) override;

	// Activates or deactivates all known filters.
	void setAllFilterStates(bool state) override;

	// RegisterableModule implementation
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;
	void shutdownModule() override;
};

} // namespace filters
