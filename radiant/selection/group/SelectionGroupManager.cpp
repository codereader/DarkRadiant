#include "SelectionGroupManager.h"

#include "i18n.h"
#include "imap.h"
#include "iradiant.h"
#include "itextstream.h"
#include "imapinfofile.h"
#include "icommandsystem.h"
#include "iselection.h"
#include "ieventmanager.h"
#include "imainframe.h"
#include "iorthocontextmenu.h"
#include "module/StaticModule.h"
#include "selection/algorithm/Group.h"

#include "wxutil/dialog/MessageBox.h"
#include "selectionlib.h"
#include "SelectionGroup.h"
#include "scene/SelectableNode.h"
#include "SelectionGroupInfoFileModule.h"
#include "wxutil/menu/MenuItem.h"
#include "wxutil/menu/IconTextMenuItem.h"

namespace selection
{

SelectionGroupManager::SelectionGroupManager() :
	_nextGroupId(1)
{}

ISelectionGroupPtr SelectionGroupManager::createSelectionGroup()
{
	// Reserve a new group ID
	std::size_t id = generateGroupId();

	SelectionGroupPtr group = std::make_shared<SelectionGroup>(id);
	_groups[id] = group;

	return group;
}

ISelectionGroupPtr SelectionGroupManager::getSelectionGroup(std::size_t id)
{
	SelectionGroupMap::iterator found = _groups.find(id);

	return found != _groups.end() ? found->second : ISelectionGroupPtr();
}

ISelectionGroupPtr SelectionGroupManager::findOrCreateSelectionGroup(std::size_t id)
{
	SelectionGroupMap::iterator found = _groups.find(id);

	return found != _groups.end() ? found->second : createSelectionGroup(id);
}

void SelectionGroupManager::setGroupSelected(std::size_t id, bool selected)
{
	SelectionGroupMap::iterator found = _groups.find(id);

	if (found == _groups.end())
	{
		rError() << "Cannot find the group with ID " << id << std::endl;
		return;
	}

	found->second->setSelected(selected);
}

void SelectionGroupManager::deleteSelectionGroup(std::size_t id)
{
	SelectionGroupMap::iterator found = _groups.find(id);

	if (found == _groups.end())
	{
		rError() << "Cannot delete the group with ID " << id << " as it doesn't exist." << std::endl;
		return;
	}

	found->second->removeAllNodes();

	_groups.erase(found);
}

void SelectionGroupManager::deleteAllSelectionGroups()
{
	for (SelectionGroupMap::iterator g = _groups.begin(); g != _groups.end(); )
	{
		deleteSelectionGroup((g++)->first);
	}

	assert(_groups.empty());

	resetNextGroupId();
}

void SelectionGroupManager::foreachSelectionGroup(const std::function<void(ISelectionGroup&)>& func)
{
	for (SelectionGroupMap::value_type& pair : _groups)
	{
		func(*pair.second);
	}
}

ISelectionGroupPtr SelectionGroupManager::createSelectionGroup(std::size_t id)
{
	if (_groups.find(id) != _groups.end())
	{
		rWarning() << "Cannot create group with ID " << id << ", as it's already taken." << std::endl;
		throw std::runtime_error("Group ID already taken");
	}

	auto group = std::make_shared<SelectionGroup>(id);
	_groups[id] = group;

	// Adjust the next group ID 
	resetNextGroupId();

	return group;
}

void SelectionGroupManager::resetNextGroupId()
{
	if (_groups.empty())
	{
		_nextGroupId = 0;
	}
	else
	{
		_nextGroupId = _groups.rbegin()->first + 1;
	}
}

std::size_t SelectionGroupManager::generateGroupId()
{
	if (_nextGroupId + 1 == std::numeric_limits<std::size_t>::max())
	{
		throw std::runtime_error("Out of group IDs.");
	}

	return _nextGroupId++;
}

}
