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
	virtual ~IGroupSelectable() {}

	// Adds this item to the group specified by its ID
	virtual void addToGroup(std::size_t groupId) = 0;

	// Removes this item from the group specified by its ID
	virtual void removeFromGroup(std::size_t groupId) = 0;

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
