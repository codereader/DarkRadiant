#include "iselectionset.h"
#include "itextstream.h"
#include "imapinfofile.h"
#include "ieventmanager.h"
#include "iselection.h"
#include "icommandsystem.h"
#include "i18n.h"
#include "imap.h"
#include "iradiant.h"

#include <sigc++/sigc++.h>
#include "modulesystem/StaticModule.h"

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
			_dependencies.insert(MODULE_SELECTIONSYSTEM);
			_dependencies.insert(MODULE_EVENTMANAGER);
			_dependencies.insert(MODULE_COMMANDSYSTEM);
			_dependencies.insert(MODULE_RADIANT);
			_dependencies.insert(MODULE_MAP);
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

		GlobalEventManager().addCommand("DeleteAllSelectionSets", "DeleteAllSelectionSets");

		GlobalMapInfoFileManager().registerInfoFileModule(
			std::make_shared<SelectionSetInfoFileModule>()
		);
	}

private:
	void onRadiantStartup()
	{
		// Get the horizontal toolbar and add a custom widget
		wxToolBar* toolbar = GlobalMainFrame().getToolbar(IMainFrame::TOOLBAR_HORIZONTAL);

		// Insert a separator at the end of the toolbar
		toolbar->AddSeparator();

		wxStaticText* label = new wxStaticText(toolbar, wxID_ANY, _("Selection Set: "));
		toolbar->AddControl(label);

		// Construct a new tool menu object
		_toolMenu.reset(new SelectionSetToolmenu(toolbar));

		toolbar->Realize();

#ifdef __WXOSX__
		// Weird workaround to stop an empty area from being drawn
		// where the label and combobox are supposed to be
		label->Hide();
		label->Show();
		_toolMenu->Hide();
		_toolMenu->Show();
#endif
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
