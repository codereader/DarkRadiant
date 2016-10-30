#pragma once

#include "iselectiongroup.h"
#include "imap.h"
#include "icommandsystem.h"
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

	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const ApplicationContext& ctx) override;

	ISelectionGroupPtr createSelectionGroup() override;
	void setGroupSelected(std::size_t id, bool selected) override;
	void deleteAllSelectionGroups() override;
	void deleteSelectionGroup(std::size_t id) override;
	ISelectionGroupPtr getSelectionGroup(std::size_t id) override;
	ISelectionGroupPtr findOrCreateSelectionGroup(std::size_t id) override;

	// Internal methods only accessible through getSelectionGroupManagerInternal()

	void foreachSelectionGroup(const std::function<void(ISelectionGroup&)>& func);
	
	// Internal method allowing to create groups by ID
	ISelectionGroupPtr createSelectionGroupInternal(std::size_t id);

private:
	void deleteAllSelectionGroupsCmd(const cmd::ArgumentList& args);
	void groupSelectedCmd(const cmd::ArgumentList& args);
	void ungroupSelectedCmd(const cmd::ArgumentList& args);

	void onMapEvent(IMap::MapEvent ev);

	std::size_t generateGroupId();
	void resetNextGroupId();

	void doDeleteSelectionGroup(std::size_t id);
};

// Internal accessor method to get hold of the implementing subclass
SelectionGroupManager& getSelectionGroupManagerInternal();

}
