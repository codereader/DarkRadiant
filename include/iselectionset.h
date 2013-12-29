#pragma once

#include "imodule.h"
#include "inode.h"
#include <set>

namespace selection
{

class ISelectionSet
{
public:
	// The name of this set
	virtual const std::string& getName() = 0;

	// Checks whether this set is empty
	virtual bool empty() = 0;

	// Selects all member nodes of this set
	virtual void select() = 0;

	// De-selects all member nodes of this set
	virtual void deselect() = 0;

	// Removes all members, leaving this set as empty
	virtual void clear() = 0;

	// Clears this set and loads the currently selected nodes in the
	// scene as new members into this set.
	virtual void assignFromCurrentScene() = 0;

	// Adds the given node to this set
	virtual void addNode(const scene::INodePtr& node) = 0;

	// Returns the nodes contained in this selection set.
	virtual std::set<scene::INodePtr> getNodes() = 0;
};
typedef boost::shared_ptr<ISelectionSet> ISelectionSetPtr;

class ISelectionSetManager :
	public RegisterableModule
{
public:
	class Observer
	{
	public:
		// Called when the list of selection sets has been changed,
		// by deletion or addition
		virtual void onSelectionSetsChanged() = 0;
	};

	virtual void addObserver(Observer& observer) = 0;
	virtual void removeObserver(Observer& observer) = 0;

	class Visitor
	{
	public:
		virtual void visit(const ISelectionSetPtr& set) = 0;
	};

	/**
	 * greebo: Traverses the list of selection sets using
	 * the given visitor class.
	 */
	virtual void foreachSelectionSet(Visitor& visitor) = 0;

	typedef std::function<void(const ISelectionSetPtr&)> VisitorFunc;

	/**
	 * greebo: Traverses the list of selection sets using the given functor.
	 */
	virtual void foreachSelectionSet(const VisitorFunc& functor) = 0;

	/**
	 * greebo: Creates a new selection set with the given name.
	 * If a selection with that name is already registered, the existing
	 * one is returned.
	 */
	virtual ISelectionSetPtr createSelectionSet(const std::string& name) = 0;

	/**
	 * Removes the named selection set. If the named set is
	 * not existing, nothing happens.
	 */
	virtual void deleteSelectionSet(const std::string& name) = 0;

	/**
	 * Deletes all sets.
	 */
	virtual void deleteAllSelectionSets() = 0;

	/**
	 * Finds the named selection set.
	 *
	 * @returns the found selection set or NULL if the set is not existent.
	 */
	virtual ISelectionSetPtr findSelectionSet(const std::string& name) = 0;
};

} // namespace

const char* const MODULE_SELECTIONSET = "SelectionSetManager";

inline selection::ISelectionSetManager& GlobalSelectionSetManager()
{
	// Cache the reference locally
	static selection::ISelectionSetManager& _manager(
		*boost::static_pointer_cast<selection::ISelectionSetManager>(
			module::GlobalModuleRegistry().getModule(MODULE_SELECTIONSET)
		)
	);
	return _manager;
}
