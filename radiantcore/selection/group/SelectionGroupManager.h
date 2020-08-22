#pragma once

#include "iselectiongroup.h"
#include <map>

namespace selection
{

class SelectionGroup;
typedef std::shared_ptr<SelectionGroup> SelectionGroupPtr;

class SelectionGroupManager :
	public ISelectionGroupManager
{
private:
	typedef std::map<std::size_t, SelectionGroupPtr> SelectionGroupMap;
	SelectionGroupMap _groups;

	// Group IDs are never re-used during the same mapping session
	// to support undo/redo operations.
	std::size_t _nextGroupId;

public:
	SelectionGroupManager();

	ISelectionGroupPtr createSelectionGroup() override;
	ISelectionGroupPtr createSelectionGroup(std::size_t id) override;
	void setGroupSelected(std::size_t id, bool selected) override;
	void deleteAllSelectionGroups() override;
	void deleteSelectionGroup(std::size_t id) override;
	ISelectionGroupPtr getSelectionGroup(std::size_t id) override;
	ISelectionGroupPtr findOrCreateSelectionGroup(std::size_t id) override;
	void foreachSelectionGroup(const std::function<void(ISelectionGroup&)>& func) override;

private:
	std::size_t generateGroupId();
	void resetNextGroupId();
};

}
