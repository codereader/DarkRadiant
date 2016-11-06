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
#include "iundo.h"
#include "iorthocontextmenu.h"
#include "modulesystem/StaticModule.h"
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

const std::string& SelectionGroupManager::getName() const
{
	static std::string _name(MODULE_SELECTIONGROUP);
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
		_dependencies.insert(MODULE_MAPINFOFILEMANAGER);
	}

	return _dependencies;
}

void SelectionGroupManager::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	GlobalCommandSystem().addCommand("GroupSelected",
		std::bind(&SelectionGroupManager::groupSelectedCmd, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("UngroupSelected",
		std::bind(&SelectionGroupManager::ungroupSelectedCmd, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("DeleteAllSelectionGroups",
		std::bind(&SelectionGroupManager::deleteAllSelectionGroupsCmd, this, std::placeholders::_1));

	GlobalEventManager().addCommand("GroupSelected", "GroupSelected");
	GlobalEventManager().addCommand("UngroupSelected", "UngroupSelected");
	GlobalEventManager().addCommand("DeleteAllSelectionGroups", "DeleteAllSelectionGroups");

	GlobalMapModule().signal_mapEvent().connect(
		sigc::mem_fun(*this, &SelectionGroupManager::onMapEvent)
	);

	GlobalMapInfoFileManager().registerInfoFileModule(
		std::make_shared<SelectionGroupInfoFileModule>()
	);

	GlobalRadiant().signal_radiantStarted().connect([this] ()
	{
		GlobalUIManager().getMenuManager().insert(
			"main/edit/parent", "ungroupSelected", ui::eMenuItemType::menuItem, _("Ungroup Selection"), "ungroup_selection.png", "UngroupSelected");

		GlobalUIManager().getMenuManager().insert(
			"main/edit/ungroupSelected", "groupSelected", ui::eMenuItemType::menuItem, _("Group Selection"), "group_selection.png", "GroupSelected");

		GlobalUIManager().getMenuManager().insert(
			"main/edit/parent", "groupSelectedSeparator", ui::eMenuItemType::menuSeparator, "", "", "");
	});

	GlobalOrthoContextMenu().addItem(std::make_shared<wxutil::MenuItem>(
		new wxutil::IconTextMenuItem(_("Group Selection"), "group_selection.png"),
		[]() { algorithm::groupSelected(); },
		[]() { return algorithm::CommandNotAvailableException::ToBool(algorithm::checkGroupSelectedAvailable); }),
		ui::IOrthoContextMenu::SECTION_SELECTION_GROUPS);

	GlobalOrthoContextMenu().addItem(std::make_shared<wxutil::MenuItem>(
		new wxutil::IconTextMenuItem(_("Ungroup Selection"), "ungroup_selection.png"),
		[]() { algorithm::ungroupSelected(); },
		[]() { return algorithm::CommandNotAvailableException::ToBool(algorithm::checkUngroupSelectedAvailable); }), 
		ui::IOrthoContextMenu::SECTION_SELECTION_GROUPS);
}

void SelectionGroupManager::onMapEvent(IMap::MapEvent ev)
{
	if (ev == IMap::MapUnloaded)
	{
		deleteAllSelectionGroups();
		resetNextGroupId();
	}
}

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

	return found != _groups.end() ? found->second : createSelectionGroupInternal(id);
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
	UndoableCommand cmd("DeleteSelectionGroup");

	doDeleteSelectionGroup(id);
}

void SelectionGroupManager::doDeleteSelectionGroup(std::size_t id)
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
	UndoableCommand cmd("DeleteAllSelectionGroups");

	for (SelectionGroupMap::iterator g = _groups.begin(); g != _groups.end(); )
	{
		doDeleteSelectionGroup((g++)->first);
	}

	assert(_groups.empty());
}

void SelectionGroupManager::foreachSelectionGroup(const std::function<void(ISelectionGroup&)>& func)
{
	for (SelectionGroupMap::value_type& pair : _groups)
	{
		func(*pair.second);
	}
}

ISelectionGroupPtr SelectionGroupManager::createSelectionGroupInternal(std::size_t id)
{
	if (_groups.find(id) != _groups.end())
	{
		rWarning() << "Cannot create group with ID " << id << ", as it's already taken." << std::endl;
		throw std::runtime_error("Group ID already taken");
	}

	SelectionGroupPtr group = std::make_shared<SelectionGroup>(id);
	_groups[id] = group;

	// Adjust the next group ID 
	resetNextGroupId();

	return group;
}

void SelectionGroupManager::deleteAllSelectionGroupsCmd(const cmd::ArgumentList& args)
{
	deleteAllSelectionGroups();
}

void SelectionGroupManager::groupSelectedCmd(const cmd::ArgumentList& args)
{
	try
	{
		algorithm::groupSelected();
	}
	catch (selection::algorithm::CommandNotAvailableException& ex)
	{
		rError() << ex.what() << std::endl;
		wxutil::Messagebox::ShowError(ex.what());
	}
}

void SelectionGroupManager::ungroupSelectedCmd(const cmd::ArgumentList& args)
{
	try
	{
		algorithm::ungroupSelected();
	}
	catch (selection::algorithm::CommandNotAvailableException& ex)
	{
		rError() << ex.what() << std::endl;
		wxutil::Messagebox::ShowError(ex.what());
	}
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
#if 0
	for (std::size_t i = 0; i < std::numeric_limits<std::size_t>::max(); ++i)
	{
		if (_groups.find(i) == _groups.end())
		{
			// Found a free ID
			return i;
		}
	}
#endif

	if (_nextGroupId + 1 == std::numeric_limits<std::size_t>::max())
	{
		throw std::runtime_error("Out of group IDs.");
	}

	return _nextGroupId++;
}

module::StaticModule<SelectionGroupManager> staticSelectionGroupManagerModule;

SelectionGroupManager& getSelectionGroupManagerInternal()
{
	return *staticSelectionGroupManagerModule.getModule();
}

}
