#ifndef _ISELECTION_SET_H_
#define _ISELECTION_SET_H_

#include "imodule.h"

namespace selection
{

class ISelectionSet
{
public:
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
};
typedef boost::shared_ptr<ISelectionSet> ISelectionSetPtr;

class ISelectionSetManager :
	public RegisterableModule
{
public:
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

#endif /* _ISELECTION_SET_H_ */
