#include <functional>

#include "iselectiongroup.h"
#include "i18n.h"
#include "icommandsystem.h"
#include "ieventmanager.h"
#include "imap.h"
#include "iradiant.h"
#include "iorthocontextmenu.h"

#include "SelectionGroupManager.h"

#include "selection/algorithm/Group.h"
#include "SelectionGroupInfoFileModule.h"
#include "wxutil/menu/MenuItem.h"
#include "wxutil/menu/IconTextMenuItem.h"
#include "module/StaticModule.h"

namespace selection
{

class SelectionGroupModule :
	public ISelectionGroupModule
{
public:
	const std::string & getName() const override
	{
		static std::string _name(MODULE_SELECTIONGROUPMODULE);
		return _name;
	}

	const StringSet& getDependencies() const override
	{
		static StringSet _dependencies;

		if (_dependencies.empty())
		{
			_dependencies.insert(MODULE_SELECTIONSYSTEM);
			_dependencies.insert(MODULE_EVENTMANAGER);
			_dependencies.insert(MODULE_COMMANDSYSTEM);
			_dependencies.insert(MODULE_RADIANT_APP);
			_dependencies.insert(MODULE_MAP);
			_dependencies.insert(MODULE_MAPINFOFILEMANAGER);
		}

		return _dependencies;
	}

	void initialiseModule(const ApplicationContext& ctx) override
	{
		rMessage() << getName() << "::initialiseModule called." << std::endl;

		GlobalCommandSystem().addCommand("GroupSelected", algorithm::groupSelectedCmd);
		GlobalCommandSystem().addCommand("UngroupSelected", algorithm::ungroupSelectedCmd);
		GlobalCommandSystem().addCommand("DeleteAllSelectionGroups", algorithm::deleteAllSelectionGroupsCmd);

		GlobalEventManager().addCommand("GroupSelected", "GroupSelected");
		GlobalEventManager().addCommand("UngroupSelected", "UngroupSelected");
		GlobalEventManager().addCommand("DeleteAllSelectionGroups", "DeleteAllSelectionGroups");

		GlobalMapModule().signal_mapEvent().connect(
			sigc::mem_fun(*this, &SelectionGroupModule::onMapEvent)
		);

		GlobalMapInfoFileManager().registerInfoFileModule(
			std::make_shared<SelectionGroupInfoFileModule>()
		);

		GlobalRadiant().signal_radiantStarted().connect([this]()
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

	ISelectionGroupManager::Ptr createSelectionGroupManager() override
	{
		return std::make_shared<SelectionGroupManager>();
	}

private:
	void onMapEvent(IMap::MapEvent ev)
	{
		if (ev == IMap::MapUnloaded && GlobalMapModule().getRoot())
		{
			algorithm::getMapSelectionGroupManager().deleteAllSelectionGroups();
		}
	}
};

module::StaticModule<SelectionGroupModule> selGroupModule;

}

