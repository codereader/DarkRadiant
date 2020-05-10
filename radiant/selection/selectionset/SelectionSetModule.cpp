#include "iselectionset.h"
#include "itextstream.h"
#include "imapinfofile.h"
#include "ieventmanager.h"
#include "iselection.h"
#include "icommandsystem.h"
#include "imap.h"
#include "i18n.h"
#include "iradiant.h"

#include <sigc++/sigc++.h>
#include "module/StaticModule.h"

#include "SelectionSetInfoFileModule.h"
#include "SelectionSetToolmenu.h"
#include "SelectionSetManager.h"

namespace selection
{

class SelectionSetModule :
	public ISelectionSetModule
{
private:
	std::unique_ptr<SelectionSetToolmenu> _toolMenu;

public:
	ISelectionSetManager::Ptr createSelectionSetManager() override
	{
		return std::make_shared<SelectionSetManager>();
	}

	const std::string& getName() const override
	{
		static std::string _name(MODULE_SELECTIONSETS);
		return _name;
	}

	const StringSet& getDependencies() const override
	{
		static StringSet _dependencies;

		if (_dependencies.empty())
		{
			_dependencies.insert(MODULE_EVENTMANAGER);
			_dependencies.insert(MODULE_COMMANDSYSTEM);
			_dependencies.insert(MODULE_RADIANT_APP);
			_dependencies.insert(MODULE_MAPINFOFILEMANAGER);
		}

		return _dependencies;
	}

	void initialiseModule(const ApplicationContext& ctx) override
	{
		rMessage() << getName() << "::initialiseModule called." << std::endl;

		// Register for the startup event
		GlobalRadiant().signal_radiantStarted().connect(
			sigc::mem_fun(this, &SelectionSetModule::onRadiantStartup)
		);

		GlobalCommandSystem().addCommand("DeleteAllSelectionSets",
			std::bind(&SelectionSetModule::deleteAllSelectionSetsCmd, this, std::placeholders::_1));

		GlobalMapInfoFileManager().registerInfoFileModule(
			std::make_shared<SelectionSetInfoFileModule>()
		);
	}

private:
	void onRadiantStartup()
	{
		// Construct a new tool menu object
		_toolMenu.reset(new SelectionSetToolmenu);
	}

	void deleteAllSelectionSetsCmd(const cmd::ArgumentList& args)
	{
		if (!GlobalMapModule().getRoot())
		{
			rError() << "No map loaded, can't delete any sets" << std::endl;
			return;
		}

		GlobalMapModule().getRoot()->getSelectionSetManager().deleteAllSelectionSets();
	}
};

module::StaticModule<SelectionSetModule> selectionSetModule;

}
