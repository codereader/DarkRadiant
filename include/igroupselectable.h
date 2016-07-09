#pragma once

#include "iselectable.h"

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
	virtual void setSelected(bool select, bool notifyGroup) = 0;
};
