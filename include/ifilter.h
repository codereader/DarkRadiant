#pragma once

#include "imodule.h"
#include "inode.h"

#include <boost/shared_ptr.hpp>
#include <vector>
#include <cassert>

#include <sigc++/signal.h>

/**
 * This structure defines a simple filtercriterion as used by the Filtersystem
 */
class FilterRule
{
public:
	enum Type
	{
		TYPE_TEXTURE,
		TYPE_ENTITYCLASS,
		TYPE_OBJECT,
		TYPE_ENTITYKEYVALUE,
	};

	// The rule type
	Type type;

	// The entity key, only applies for type "entitykeyvalue"
	std::string entityKey;

	// the match expression regex
	std::string match;

	// true for action="show", false for action="hide"
	bool show;

private:
	// Private Constructor, use the named constructors below
	FilterRule(const Type type_, const std::string& match_, bool show_) :
		type(type_),
		match(match_),
		show(show_)
	{}

	// Alternative private constructor for the entityKeyValue type
	FilterRule(const Type type_, const std::string& entityKey_, const std::string& match_, bool show_) :
		type(type_),
		entityKey(entityKey_),
		match(match_),
		show(show_)
	{}

public:
	// Named constructors

	// Regular constructor for the non-entitykeyvalue types
	static FilterRule Create(const Type type, const std::string& match, bool show)
	{
		assert(type != TYPE_ENTITYKEYVALUE); 

		return FilterRule(type, match, show);
	}

	// Constructor for the entity key value type
	static FilterRule CreateEntityKeyValueRule(const std::string& key, const std::string& match, bool show)
	{
		return FilterRule(TYPE_ENTITYKEYVALUE, key, match, show);
	}
};
typedef std::vector<FilterRule> FilterRules;

/** Visitor interface for evaluating the available filters in the
 * FilterSystem.
 */
struct IFilterVisitor
{
    virtual ~IFilterVisitor() {}

	// Visit function
	virtual void visit(const std::string& filterName) = 0;
};

const char* const MODULE_FILTERSYSTEM = "FilterSystem";

// Forward declaration
class Entity;

/**
 * \brief
 * Interface for the FilterSystem
 *
 * The filter system provides a mechanism by which certain objects or materials
 * can be hidden from rendered views.
 */
class FilterSystem :
	public RegisterableModule
{
public:

    /// Signal emitted when the state of filters has changed
    virtual sigc::signal<void> filtersChangedSignal() const = 0;

	/**
	 * greebo: Updates all the "Filtered" status of all Instances
	 *         in the scenegraph based on the current filter settings.
	 */
	virtual void update() = 0;

	/**
	 * greebo: Lets the filtersystem update the specified subgraph only,
	 * which includes the given node and all children.
	 */
	virtual void updateSubgraph(const scene::INodePtr& root) = 0;

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

	/** 
	 * Activates or deactivates all known filters.
	 */
	virtual void setAllFilterStates(bool state) = 0;

	/** greebo: Returns the event name of the given filter. This is needed
	 * 			to create the toggle event to menus/etc.
	 */
	virtual std::string getFilterEventName(const std::string& filter) = 0;

	/** Test if a given item should be visible or not, based on the currently-
	 * active filters.
	 *
	 * @param item
	 * The filter type to query
	 *
	 * @param name
	 * String name of the item to query.
	 *
	 * @returns
	 * true if the item is visible, false otherwise.
	 */
	virtual bool isVisible(const FilterRule::Type type, const std::string& name) = 0;

	/**
	 * Test if a given entity should be visible or not, based on the currently active filters.
	 *
	 * @param type
	 * The filter type to query
	 *
	 * @param entity
	 * The Entity to test
	 *
	 * @returns
	 * true if the entity is visible, false otherwise.
	 */
	virtual bool isEntityVisible(const FilterRule::Type type, const Entity& entity) = 0;	

	// =====  API for Filter management and editing =====

	/**
	 * greebo: Returns TRUE if the filter is read-only and can't be deleted.
	 */
	virtual bool filterIsReadOnly(const std::string& filter) = 0;

	/**
	 * greebo: Adds a new filter to the system with the given ruleset. The new filter
	 * is not set to read-only.
	 *
	 * @returns: TRUE on success, FALSE if the filter name already exists.
	 */
	virtual bool addFilter(const std::string& filterName, const FilterRules& ruleSet) = 0;

	/**
	 * greebo: Removes the filter, returns TRUE on success.
	 */
	virtual bool removeFilter(const std::string& filter) = 0;

	/**
	 * greebo: Renames the specified filter. This also takes care of renaming the corresponding command in the
	 * EventManager class.
	 *
	 * @returns: TRUE on success, FALSE if the filter hasn't been found or is read-only.
	 */
	virtual bool renameFilter(const std::string& oldFilterName, const std::string& newFilterName) = 0;

	/**
	 * greebo: Returns the ruleset of this filter, order is important.
	 */
	virtual FilterRules getRuleSet(const std::string& filter) = 0;

	/**
	 * greebo: Applies the given criteria set to the named filter, replacing the existing set.
	 * This applies to non-read-only filters only.
	 *
	 * @returns: TRUE on success, FALSE if filter not found or read-only.
 	 */
	virtual bool setFilterRules(const std::string& filter, const FilterRules& ruleSet) = 0;
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
