#pragma once

#include "imodule.h"
#include "iselectable.h"
#include <sigc++/signal.h>

// GroupSelectables are regular selectables which can be part of 
// one or more SelectionGroups.
class IGroupSelectable :
	public ISelectable
{
public:
	typedef std::vector<std::size_t> GroupIds;

	virtual ~IGroupSelectable() {}

	// Adds this item to the group specified by its ID
	virtual void addToGroup(std::size_t groupId) = 0;

	// Removes this item from the group specified by its ID
	virtual void removeFromGroup(std::size_t groupId) = 0;

	// Returns true if this node is member of any group
	virtual bool isGroupMember() = 0;

	// Returns the group this node has been added to last
	// This represents the currently "active" group ID
	// Will throw an exception if this node is not a member of any group
	virtual std::size_t getMostRecentGroupId() = 0;

	// Returns all group assignments of this node
	// The most recently added group is at the back of the list
	virtual const GroupIds& getGroupIds() = 0;

	// Special overload to control whether this selectable should propagate
	// the status change to the group it belongs to.
	virtual void setSelected(bool select, bool changeGroupStatus) = 0;
};

namespace selection
{

class ISelectionGroupManager :
	public RegisterableModule
{
public:
	virtual ~ISelectionGroupManager() {}

	// Sets the selection status of all members of the given group
	virtual void setGroupSelected(std::size_t id, bool selected) = 0;

	virtual void deleteAllSelectionGroups() = 0;

	virtual void deleteSelectionGroup(std::size_t id) = 0;
};

} // namespace

const char* const MODULE_SELECTIONGROUP = "SelectionGroupManager";

inline selection::ISelectionGroupManager& GlobalSelectionGroupManager()
{
	// Cache the reference locally
	static selection::ISelectionGroupManager& _manager(
		*std::static_pointer_cast<selection::ISelectionGroupManager>(
			module::GlobalModuleRegistry().getModule(MODULE_SELECTIONGROUP)
		)
	);
	return _manager;
}
