#include "BasicFilterSystem.h"

#include "InstanceUpdateWalker.h"
#include "ShaderUpdateWalker.h"

#include "iradiant.h"
#include "iscenegraph.h"
#include "iregistry.h"
#include "ieventmanager.h"
#include "igame.h"
#include "ishaders.h"

#include "generic/callback.h"

namespace filters
{

namespace {
	
	// Registry key for .game-defined filters
	const std::string RKEY_GAME_FILTERS = "game/filtersystem//filter";
	
	// Registry key for user-defined persistent filters
	const std::string RKEY_USER_FILTERS = "user/ui/filtersystem//activeFilter";

}

// Initialise the filter system
void BasicFilterSystem::initialiseModule(const ApplicationContext& ctx) {

	// Load the list of active filter names from the user tree. There is no
	// guarantee that these are actually valid filters in the .game file
	std::set<std::string> activeFilterNames;
	xml::NodeList activeFilters = GlobalRegistry().findXPath(RKEY_USER_FILTERS);
	for (xml::NodeList::const_iterator i = activeFilters.begin();
		 i != activeFilters.end();
		 ++i)
	{
		// Add the name of this filter to the set
		activeFilterNames.insert(i->getAttributeValue("name"));
	}
	
	// Ask the XML Registry for the game filter nodes
	xml::NodeList filters = GlobalRegistry().findXPath(RKEY_GAME_FILTERS);
	std::cout << "[filters] Loaded " << filters.size() 
			  << " filters from registry." << std::endl;

	// Iterate over the list of nodes, adding filter objects onto the list
	for (xml::NodeList::iterator iter = filters.begin();
		 iter != filters.end();
		 ++iter)
	{
		// Initialise the XMLFilter object
		std::string filterName = iter->getAttributeValue("name");
		XMLFilter filter(filterName);

		// Get all of the filterCriterion children of this node
		xml::NodeList critNodes = iter->getNamedChildren("filterCriterion");

		// Create XMLFilterRule objects for each criterion
		for (xml::NodeList::iterator critIter = critNodes.begin();
			 critIter != critNodes.end();
			 ++critIter)
		{
			filter.addRule(critIter->getAttributeValue("type"),
						   critIter->getAttributeValue("match"),
						   critIter->getAttributeValue("action") == "show");
		}
		
		// Add this XMLFilter to the list of available filters
		XMLFilter& inserted = _availableFilters.insert(
			FilterTable::value_type(filterName, filter)
		).first->second;
		
		// Add the according toggle command to the eventmanager
		IEventPtr fEvent = GlobalEventManager().addToggle(
			filter.getEventName(),
			MemberCaller<XMLFilter, &XMLFilter::toggle>(inserted) 
		);
		
		// If this filter is in our active set, enable it
		if (activeFilterNames.find(filterName) != activeFilterNames.end()) {
			fEvent->setToggled(true);
			_activeFilters.insert(
				FilterTable::value_type(filterName, inserted)
			);
		}
	}
}

// Shut down the Filters module, saving active filters to registry
void BasicFilterSystem::shutdownModule() {
	
	// Remove the existing set of active filter nodes
	GlobalRegistry().deleteXPath("user/ui/filtersystem");
	
	// Add a node for each active filter
	for (FilterTable::const_iterator i = _activeFilters.begin();
		 i != _activeFilters.end();
		 ++i)
	{
		GlobalRegistry().createKeyWithName(
			"user/ui/filtersystem", "activeFilter", i->first
		);
	}
}

void BasicFilterSystem::update() {
	updateScene();
	updateShaders();
}

void BasicFilterSystem::forEachFilter(IFilterVisitor& visitor) {

	// Visit each filter on the list, passing the name to the visitor
	for (FilterTable::iterator iter = _availableFilters.begin();
		 iter != _availableFilters.end();
		 ++iter)
	{
		visitor.visit(iter->first);
	}
}

std::string BasicFilterSystem::getFilterEventName(const std::string& filter) {
	FilterTable::iterator f = _availableFilters.find(filter);
	
	if (f != _availableFilters.end()) {
		return f->second.getEventName();
	}
	else {
		return "";
	}
}

// Change the state of a named filter
void BasicFilterSystem::setFilterState(const std::string& filter, bool state) {

	assert(!_availableFilters.empty());
	if (state) {
		// Copy the filter to the active filters list
		_activeFilters.insert(
			FilterTable::value_type(
				filter, _availableFilters.find(filter)->second));
	}
	else {
		assert(!_activeFilters.empty());
		// Remove filter from active filters list
		_activeFilters.erase(filter);
	}

	// Invalidate the visibility cache to force new values to be
	// loaded from the filters themselves
	_visibilityCache.clear();
			
	// Update the scenegraph instances
	update();
	
	// Trigger an immediate scene redraw
	GlobalSceneGraph().sceneChanged();
}

// Query whether an item is visible or filtered out
bool BasicFilterSystem::isVisible(const std::string& item, 
								  const std::string& name) 
{
	// Check if this item is in the visibility cache, returning
	// its cached value if found
	StringFlagCache::iterator cacheIter = _visibilityCache.find(name);
	if (cacheIter != _visibilityCache.end())
		return cacheIter->second;
		
	// Otherwise, walk the list of active filters to find a value for
	// this item.
	bool visFlag = true; // default if no filters modify it
	
	for (FilterTable::iterator activeIter = _activeFilters.begin();
		 activeIter != _activeFilters.end();
		 ++activeIter)
	{
		// Delegate the check to the filter object. If a filter returns
		// false for the visibility check, then the item is filtered
		// and we don't need any more checks.			
		if (!activeIter->second.isVisible(item, name)) {
			visFlag = false;
			break;
		}
	}			

	// Cache the result and return to caller
	_visibilityCache.insert(StringFlagCache::value_type(name, visFlag));
	return visFlag;
}

// Update scenegraph instances with filtered status
void BasicFilterSystem::updateScene() {

	// Construct an InstanceUpdateWalker and traverse the scenegraph to update
	// all instances
	InstanceUpdateWalker walker;
	GlobalSceneGraph().traverse(walker);
}

// Update scenegraph instances with filtered status
void BasicFilterSystem::updateShaders() {
	// Construct a ShaderVisitor to traverse the shaders
	ShaderUpdateWalker walker;
	GlobalShaderSystem().foreachShader(walker);
}

// RegisterableModule implementation
const std::string& BasicFilterSystem::getName() const {
	static std::string _name(MODULE_FILTERSYSTEM);
	return _name;
}

const StringSet& BasicFilterSystem::getDependencies() const {
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_GAMEMANAGER);
		_dependencies.insert(MODULE_EVENTMANAGER);
	}

	return _dependencies;
}

}
