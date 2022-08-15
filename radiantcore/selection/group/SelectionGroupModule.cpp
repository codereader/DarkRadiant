#include <functional>

#include "iselectiongroup.h"
#include "icommandsystem.h"
#include "imap.h"

#include "SelectionGroupManager.h"

#include "selection/algorithm/Group.h"
#include "SelectionGroupInfoFileModule.h"
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
			_dependencies.insert(MODULE_COMMANDSYSTEM);
			_dependencies.insert(MODULE_MAP);
			_dependencies.insert(MODULE_MAPINFOFILEMANAGER);
		}

		return _dependencies;
	}

	void initialiseModule(const IApplicationContext& ctx) override
	{
		GlobalCommandSystem().addCommand("GroupSelected", algorithm::groupSelectedCmd);
		GlobalCommandSystem().addCommand("UngroupSelected", algorithm::ungroupSelectedCmd);
		GlobalCommandSystem().addCommand("DeleteAllSelectionGroups", algorithm::deleteAllSelectionGroupsCmd);

		GlobalMapModule().signal_mapEvent().connect(
			sigc::mem_fun(*this, &SelectionGroupModule::onMapEvent)
		);

		GlobalMapInfoFileManager().registerInfoFileModule(
			std::make_shared<SelectionGroupInfoFileModule>()
		);
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

module::StaticModuleRegistration<SelectionGroupModule> selGroupModule;

}

