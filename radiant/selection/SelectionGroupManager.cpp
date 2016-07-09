#include "SelectionGroupManager.h"

#include "imap.h"
#include "itextstream.h"
#include "icommandsystem.h"
#include "iselection.h"
#include "ieventmanager.h"
#include "modulesystem/StaticModule.h"

#include "selection/SelectionGroup.h"
#include "SelectableNode.h"

namespace selection
{

const std::string& SelectionGroupManager::getName() const
{
	static std::string _name("SelectionGroupManager");
	return _name;
}

const StringSet& SelectionGroupManager::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_SELECTIONSYSTEM);
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_RADIANT);
		_dependencies.insert(MODULE_MAP);
	}

	return _dependencies;
}

void SelectionGroupManager::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	GlobalCommandSystem().addCommand("DeleteAllSelectionGroups",
		std::bind(&SelectionGroupManager::deleteAllSelectionGroupsCmd, this, std::placeholders::_1));

	GlobalEventManager().addCommand("DeleteAllSelectionGroups", "DeleteAllSelectionGroups");

	GlobalMapModule().signal_mapEvent().connect(
		sigc::mem_fun(*this, &SelectionGroupManager::onMapEvent)
	);
}

void SelectionGroupManager::onMapEvent(IMap::MapEvent ev)
{
	if (ev == IMap::MapUnloaded)
	{
		deleteAllSelectionGroups();
	}
}

void SelectionGroupManager::deleteSelectionGroup(std::size_t id)
{
	SelectionGroupMap::iterator found = _groups.find(id);

	if (found == _groups.end())
	{
		rError() << "Cannot delete the group with ID " << id << " as it doesn't exist." << std::endl;
		return;
	}

	found->second->foreachNode([&](const scene::INodePtr& node)
	{
		std::shared_ptr<scene::SelectableNode> selectable = std::dynamic_pointer_cast<scene::SelectableNode>(node);

		assert(selectable);

		selectable->removeFromGroup(found->first);
	});

	_groups.erase(found);
}

void SelectionGroupManager::deleteAllSelectionGroups()
{
	for (SelectionGroupMap::iterator g = _groups.begin(); g != _groups.end(); )
	{
		deleteSelectionGroup(g->first);
	}

	assert(_groups.empty());
}

void SelectionGroupManager::deleteAllSelectionGroupsCmd(const cmd::ArgumentList& args)
{
	deleteAllSelectionGroups();
}

void SelectionGroupManager::groupSelectedCmd(const cmd::ArgumentList& args)
{
	// Create a new group ID
	std::size_t id = generateGroupId();

	SelectionGroupPtr group = std::make_shared<SelectionGroup>(id);
	_groups[id] = group;

	GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
	{
		std::shared_ptr<scene::SelectableNode> selectable = std::dynamic_pointer_cast<scene::SelectableNode>(node);

		if (!selectable) return;

		selectable->addToGroup(id);
		group->addNode(node);
	});
}

std::size_t SelectionGroupManager::generateGroupId()
{
	for (std::size_t i = 0; i < std::numeric_limits<std::size_t>::max(); ++i)
	{
		if (_groups.find(i) == _groups.end())
		{
			// Found a free ID
			return i;
		}
	}

	throw std::runtime_error("Out of group IDs.");
}

}