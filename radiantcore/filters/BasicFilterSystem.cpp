#include "BasicFilterSystem.h"

#include <functional>

#include "iradiant.h"
#include "itextstream.h"
#include "iscenegraph.h"
#include "iregistry.h"
#include "igame.h"
#include "ishaders.h"

#include "module/StaticModule.h"
#include "InstanceUpdateWalker.h"
#include "SetObjectSelectionByFilterWalker.h"

namespace filters
{

namespace
{
	// Registry key for .game-defined filters
	const std::string RKEY_GAME_FILTERS = "/filtersystem//filter";

	const std::string RKEY_USER_FILTER_BASE = "user/ui/filtersystem";

	// Registry key for user-defined filters
	const std::string RKEY_USER_FILTERS = RKEY_USER_FILTER_BASE + "/filters//filter";

	// Registry key for persistent filter setting
	const std::string RKEY_USER_ACTIVE_FILTERS = RKEY_USER_FILTER_BASE + "//activeFilter";
}

void BasicFilterSystem::setAllFilterStates(bool state)
{
	if (state)
	{
		_activeFilters = _availableFilters;
	}
	else
	{
		_activeFilters.clear();
	}

	// Invalidate the visibility cache to force new values to be
	// loaded from the filters themselves
	_visibilityCache.clear();

	// Update the scenegraph instances
	update();

	_filterConfigChangedSignal.emit();

	// Trigger an immediate scene redraw
	GlobalSceneGraph().sceneChanged();
}

void BasicFilterSystem::setAllFilterStatesCmd(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rMessage() << "Usage: SetAllFilterStates 1|0" << std::endl;
		rMessage() << " an argument value of 1 activates all filters, 0 deactivates them." << std::endl;
		return;
	}

	setAllFilterStates(args.front().getInt() != 0);
}

void BasicFilterSystem::setFilterStateCmd(const cmd::ArgumentList& args)
{
	if (args.size() != 2)
	{
		rMessage() << "Usage: SetFilterState <FilterName> <1|0>" << std::endl;
		rMessage() << " an argument value of 1 activates the named filters, 0 deactivates it." << std::endl;
		return;
	}

	std::string filterName = args[0].getString();

	if (_availableFilters.find(filterName) == _availableFilters.end())
	{
		rError() << "Unknown filter: " << filterName << std::endl;
		return;
	}

	setFilterState(args[0].getString(), args[1].getInt() != 0);
}

void BasicFilterSystem::toggleFilterStateCmd(const cmd::ArgumentList& args)
{
	if (args.size() != 2)
	{
		rMessage() << "Usage: ToggleFilterState <FilterName>" << std::endl;
		return;
	}

	std::string filterName = args[0].getString();

	if (_availableFilters.find(filterName) == _availableFilters.end())
	{
		rError() << "Unknown filter: " << filterName << std::endl;
		return;
	}

	setFilterState(filterName, !getFilterState(filterName));
}

void BasicFilterSystem::selectObjectsByFilterCmd(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rMessage() << "Usage: SelectObjectsByFilter \"FilterName\"" << std::endl;
		return;
	}

	setObjectSelectionByFilter(args[0].getString(), true);
}

void BasicFilterSystem::deselectObjectsByFilterCmd(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rMessage() << "Usage: DeselectObjectsByFilter \"FilterName\"" << std::endl;
		return;
	}

	setObjectSelectionByFilter(args[0].getString(), false);
}

void BasicFilterSystem::setObjectSelectionByFilter(const std::string& filterName, bool select)
{
	if (!GlobalSceneGraph().root())
	{
		rError() << "No map loaded." << std::endl;
		return;
	}

	auto f = _availableFilters.find(filterName);

	if (f == _availableFilters.end())
	{
		rError() << "Cannot find the filter named " << filterName << std::endl;
		return;
	}

	SetObjectSelectionByFilterWalker walker(*f->second, select);
	GlobalSceneGraph().root()->traverse(walker);
}

void BasicFilterSystem::initialiseModule(const IApplicationContext& ctx)
{
	game::IGamePtr game = GlobalGameManager().currentGame();
	assert(game);

	// Ask the XML Registry for filter nodes (from .game file and from user's filters.xml)
	xml::NodeList filters = game->getLocalXPath(RKEY_GAME_FILTERS);
	xml::NodeList userFilters = GlobalRegistry().findXPath(RKEY_USER_FILTERS);

    rMessage() << "[filters] Loaded " << (filters.size() + userFilters.size())
			  << " filters from registry." << std::endl;

	// Read-only filters
	addFiltersFromXML(filters, true);

	// user-defined filters
	addFiltersFromXML(userFilters, false);

	// Add the (de-)activate all commands
	GlobalCommandSystem().addCommand("SetAllFilterStates",
		std::bind(&BasicFilterSystem::setAllFilterStatesCmd, this, std::placeholders::_1), { cmd::ARGTYPE_INT });

	// Command to activate/deactivate a named filter
	GlobalCommandSystem().addCommand("SetFilterState",
		std::bind(&BasicFilterSystem::setFilterStateCmd, this, std::placeholders::_1),
		{ cmd::ARGTYPE_STRING, cmd::ARGTYPE_INT });

	// Command to toggle a named filter's state
	GlobalCommandSystem().addCommand("ToggleFilterState",
		std::bind(&BasicFilterSystem::toggleFilterStateCmd, this, std::placeholders::_1),
		{ cmd::ARGTYPE_STRING });

	// Register two shortcuts
	GlobalCommandSystem().addStatement("ActivateAllFilters", "SetAllFilterStates 1", false);
	GlobalCommandSystem().addStatement("DeactivateAllFilters", "SetAllFilterStates 0", false);

	GlobalCommandSystem().addCommand(SELECT_OBJECTS_BY_FILTER_CMD,
		std::bind(&BasicFilterSystem::selectObjectsByFilterCmd, this, std::placeholders::_1), { cmd::ARGTYPE_STRING });

	GlobalCommandSystem().addCommand(DESELECT_OBJECTS_BY_FILTER_CMD,
		std::bind(&BasicFilterSystem::deselectObjectsByFilterCmd, this, std::placeholders::_1), { cmd::ARGTYPE_STRING });
}

void BasicFilterSystem::addFiltersFromXML(const xml::NodeList& nodes, bool readOnly)
{
	// Load the list of active filter names from the user tree. There is no
	// guarantee that these are actually valid filters in the .game file
	std::set<std::string> activeFilterNames;
	xml::NodeList activeFilters = GlobalRegistry().findXPath(RKEY_USER_ACTIVE_FILTERS);

	for (const auto& node : activeFilters)
	{
		// Add the name of this filter to the set
		activeFilterNames.insert(node.getAttributeValue("name"));
	}

	// Iterate over the list of nodes, adding filter objects onto the list
	for (const auto& node : nodes)
	{
		// Initialise the XMLFilter object
		std::string filterName = node.getAttributeValue("name");
		auto filter = std::make_shared<XMLFilter>(filterName, readOnly);

		// Get all of the filterCriterion children of this node
		xml::NodeList critNodes = node.getNamedChildren("filterCriterion");

		// Create XMLFilterRule objects for each criterion
		for (const auto& critNode : critNodes)
		{
			std::string typeStr = critNode.getAttributeValue("type");
			bool show = critNode.getAttributeValue("action") == "show";
			std::string match = critNode.getAttributeValue("match");

			if (typeStr == "texture")
			{
				filter->addRule(FilterRule::TYPE_TEXTURE, match, show);
			}
			else if (typeStr == "entityclass")
			{
				filter->addRule(FilterRule::TYPE_ENTITYCLASS, match, show);
			}
			else if (typeStr == "object")
			{
				filter->addRule(FilterRule::TYPE_OBJECT, match, show);
			}
			else if (typeStr == "entitykeyvalue")
			{
				filter->addEntityKeyValueRule(critNode.getAttributeValue("key"), match, show);
			}
		}

		// Add this XMLFilter to the list of available filters
		XMLFilter::Ptr inserted = _availableFilters.emplace(filterName, filter).first->second;

		bool filterShouldBeActive = activeFilterNames.find(filterName) != activeFilterNames.end();

		auto adapter = ensureEventAdapter(*inserted);

		// If this filter is in our active set, enable it
		if (filterShouldBeActive)
		{
			_activeFilters.emplace(filterName, inserted);
		}
	}
}

XmlFilterEventAdapter::Ptr BasicFilterSystem::ensureEventAdapter(XMLFilter& filter)
{
	auto existing = _eventAdapters.find(filter.getName());

	if (existing != _eventAdapters.end())
	{
		return existing->second;
	}

	auto result = _eventAdapters.emplace(filter.getName(),
		std::make_shared<XmlFilterEventAdapter>(filter));

	return result.first->second;
}

// Shut down the Filters module, saving active filters to registry
void BasicFilterSystem::shutdownModule()
{
	// Remove the existing set of active filter nodes
	GlobalRegistry().deleteXPath(RKEY_USER_ACTIVE_FILTERS);

	// Add a node for each active filter
	for (const auto& pair : _activeFilters)
	{
		GlobalRegistry().createKeyWithName(RKEY_USER_FILTER_BASE, "activeFilter", pair.first);
	}

	// Save user-defined filters too (delete all first)
	GlobalRegistry().deleteXPath(RKEY_USER_FILTER_BASE + "/filters");

	// Create the new top-level node
	auto filterParent = GlobalRegistry().createKey(RKEY_USER_FILTER_BASE + "/filters");

	for (const auto& pair : _availableFilters)
	{
		// Don't save stock filters
		if (pair.second->isReadOnly()) continue;

		// Create a new filter node with a name
		xml::Node filter = filterParent.createChild("filter");
		filter.setAttributeValue("name", pair.first);

		// Save all the rules as children to that node
		const auto& ruleSet = pair.second->getRuleSet();

		for (const auto& rule : ruleSet)
		{
			// Create a new criterion tag
			xml::Node criterion = filter.createChild("filterCriterion");

			std::string typeStr;

			switch (rule.type)
			{
			case FilterRule::TYPE_TEXTURE: typeStr = "texture"; break;
			case FilterRule::TYPE_OBJECT: typeStr = "object"; break;
			case FilterRule::TYPE_ENTITYCLASS: typeStr = "entityclass"; break;
			case FilterRule::TYPE_ENTITYKEYVALUE:
				typeStr = "entitykeyvalue";
				criterion.setAttributeValue("key", rule.entityKey);
				break;
			default: continue;
			};

			criterion.setAttributeValue("type", typeStr);
			criterion.setAttributeValue("match", rule.match);
			criterion.setAttributeValue("action", rule.show ? "show" : "hide");
		}
	}

	_visibilityCache.clear();
	_eventAdapters.clear();
	_activeFilters.clear();
	_availableFilters.clear();

	_filterCollectionChangedSignal.clear();
	_filterConfigChangedSignal.clear();
}

sigc::signal<void> BasicFilterSystem::filterConfigChangedSignal() const
{
    return _filterConfigChangedSignal;
}

sigc::signal<void> BasicFilterSystem::filterCollectionChangedSignal() const
{
	return _filterCollectionChangedSignal;
}

void BasicFilterSystem::update()
{
	// Update shaders first, so that nodes can judge whether they're hidden on basis of their texture
	updateShaders();

	// Now update the scene
	updateScene();
}

void BasicFilterSystem::forEachFilter(const std::function<void(const std::string & name)>& func)
{
	// Visit each filter on the list, passing the name to the visitor
	for (const auto& pair : _availableFilters)
	{
		func(pair.first);
	}
}

std::string BasicFilterSystem::getFilterEventName(const std::string& filter)
{
	auto f = _availableFilters.find(filter);

	return f != _availableFilters.end() ? f->second->getEventName() : std::string();
}

bool BasicFilterSystem::getFilterState(const std::string& filter)
{
	return _activeFilters.find(filter) != _activeFilters.end();
}

// Change the state of a named filter
void BasicFilterSystem::setFilterState(const std::string& filter, bool state)
{
	assert(!_availableFilters.empty());

	if (state)
	{
		// Copy the filter to the active filters list
		_activeFilters.emplace(filter, _availableFilters.find(filter)->second);
	}
	else
	{
		assert(!_activeFilters.empty());
		// Remove filter from active filters list
		_activeFilters.erase(filter);
	}

	// Invalidate the visibility cache to force new values to be
	// loaded from the filters themselves
	_visibilityCache.clear();

	// Update the scenegraph instances
	update();

	_filterConfigChangedSignal.emit();

	// Trigger an immediate scene redraw
	GlobalSceneGraph().sceneChanged();
}

bool BasicFilterSystem::filterIsReadOnly(const std::string& filter)
{
	auto f = _availableFilters.find(filter);

	// Return "read-only" if filter is not found
	return f != _availableFilters.end() ? f->second->isReadOnly() : true;
}

bool BasicFilterSystem::addFilter(const std::string& filterName, const FilterRules& ruleSet)
{
	auto f = _availableFilters.find(filterName);

	if (f != _availableFilters.end())
	{
		return false; // already exists
	}

	auto filter = std::make_shared<XMLFilter>(filterName, false);
	_availableFilters.emplace(filterName, filter);

	// Apply the ruleset
	filter->setRules(ruleSet);

	// Create the event adapter
	ensureEventAdapter(*filter);

	_filterCollectionChangedSignal.emit();

	return true;
}

bool BasicFilterSystem::removeFilter(const std::string& filter)
{
	auto f = _availableFilters.find(filter);

	// Refuse to delete if filter is not found or is read only
	if (f == _availableFilters.end() || f->second->isReadOnly())
	{
		return false;
	}

	_eventAdapters.erase(f->second->getName());

	// Check if the filter was active
	auto found = _activeFilters.find(f->first);
	bool wasActive = found != _activeFilters.end();

	if (wasActive)
	{
		_activeFilters.erase(found);
	}

	// Now remove the object from the available filters too
	_availableFilters.erase(f);

	_filterCollectionChangedSignal.emit();

	if (wasActive)
	{
		// Clear the cache, the rules have changed
		_visibilityCache.clear();

		_filterConfigChangedSignal.emit();

		update();
	}

	return true;
}

bool BasicFilterSystem::renameFilter(const std::string& oldFilterName, const std::string& newFilterName)
{
	// Check if the new name is already used
	auto c = _availableFilters.find(newFilterName);

	if (c != _availableFilters.end())
	{
		// Can't rename, name is already in use
		return false;
	}

	auto f = _availableFilters.find(oldFilterName);

	// Refuse to rename non-existent or read-only filters
	if (f == _availableFilters.end() || f->second->isReadOnly())
	{
		// Filter not found
		return false;
	}

	// Check if the filter was active
	auto found = _activeFilters.find(f->first);

	bool wasActive = found != _activeFilters.end();

	if (wasActive)
	{
		_activeFilters.erase(found);
	}

	// Perform the actual rename procedure
	f->second->setName(newFilterName);

	// Find the adapter to update the event bindings
	auto adapter = _eventAdapters.find(oldFilterName);

	if (adapter != _eventAdapters.end())
	{
		adapter->second->onEventNameChanged();

		// Re-insert the event adapter using a new key
		auto adapterPtr = adapter->second;
		_eventAdapters.erase(adapter);
		adapter = _eventAdapters.emplace(newFilterName, adapterPtr).first;
	}

	// Insert the new filter into the table
	_availableFilters.emplace(newFilterName, f->second);

	// If this filter is in our active set, enable it
	if (wasActive)
	{
		_activeFilters.emplace(newFilterName, f->second);
	}

	// Remove the old filter from the filtertable
	_availableFilters.erase(oldFilterName);

	_filterCollectionChangedSignal.emit();

	return true;
}

// Query whether an item is visible or filtered out
bool BasicFilterSystem::isVisible(const FilterRule::Type type, const std::string& name)
{
	// Check if this item is in the visibility cache, returning
	// its cached value if found
	auto cacheIter = _visibilityCache.find(name);

	if (cacheIter != _visibilityCache.end())
	{
		return cacheIter->second;
	}

	// Otherwise, walk the list of active filters to find a value for
	// this item.
	bool visFlag = true; // default if no filters modify it

	for (const auto& active : _activeFilters)
	{
		// Delegate the check to the filter object. If a filter returns
		// false for the visibility check, then the item is filtered
		// and we don't need any more checks.
		if (!active.second->isVisible(type, name))
		{
			visFlag = false;
			break;
		}
	}

	// Cache the result and return to caller
	_visibilityCache.emplace(name, visFlag);

	return visFlag;
}

bool BasicFilterSystem::isEntityVisible(const FilterRule::Type type, const Entity& entity)
{
	// Otherwise, walk the list of active filters to find a value for
	// this item.
	bool visFlag = true; // default if no filters modify it

	for (const auto& active : _activeFilters)
	{
		// Delegate the check to the filter object. If a filter returns
		// false for the visibility check, then the item is filtered
		// and we don't need any more checks.
		if (!active.second->isEntityVisible(type, entity))
		{
			visFlag = false;
			break;
		}
	}

	return visFlag;
}

FilterRules BasicFilterSystem::getRuleSet(const std::string& filter)
{
	auto f = _availableFilters.find(filter);

	return f != _availableFilters.end() ? f->second->getRuleSet() : FilterRules();
}

bool BasicFilterSystem::setFilterRules(const std::string& filter, const FilterRules& ruleSet)
{
	auto f = _availableFilters.find(filter);

	if (f != _availableFilters.end() && !f->second->isReadOnly())
	{
		// Apply the ruleset
		f->second->setRules(ruleSet);

		// Clear the cache, the ruleset has changed
		_visibilityCache.clear();

		_filterConfigChangedSignal.emit();

		update();

		return true;
	}

	return false; // not found or readonly
}

void BasicFilterSystem::updateSubgraph(const scene::INodePtr& root)
{
	// Construct an InstanceUpdateWalker and traverse the scenegraph to update
	// all instances
	InstanceUpdateWalker walker(*this);
	root->traverse(walker);
}

// Update scenegraph instances with filtered status
void BasicFilterSystem::updateScene()
{
    auto rootNode = GlobalSceneGraph().root();

    if (!rootNode) return;

	// pass scenegraph root to specialised routine
	updateSubgraph(rootNode);

    // Invoke onFiltersChanged on the root node
    rootNode->onFiltersChanged();
}

// Update scenegraph instances with filtered status
void BasicFilterSystem::updateShaders()
{
	// Construct a ShaderVisitor to traverse the shaders
    GlobalMaterialManager().foreachMaterial([this] (const MaterialPtr& material)
    {
        // Set the shader's visibility based on the current filter settings
        material->setVisible(
            isVisible(FilterRule::TYPE_TEXTURE, material->getName())
        );
    });
}

// RegisterableModule implementation
const std::string& BasicFilterSystem::getName() const
{
	static std::string _name(MODULE_FILTERSYSTEM);
	return _name;
}

const StringSet& BasicFilterSystem::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_GAMEMANAGER);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
	}

	return _dependencies;
}

// Module instance
module::StaticModuleRegistration<BasicFilterSystem> filterSystemModule;

}
