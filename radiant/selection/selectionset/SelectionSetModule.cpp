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

#include <wx/event.h>
#include <wx/toolbar.h>

namespace selection
{

namespace
{
	// Tool items created by the ToolBarManager carry ID >= 100
	const int CLEAR_TOOL_ID = 1;
}

class SelectionSetModule :
	public ISelectionSetModule
{
private:
	SelectionSetToolmenu* _toolMenu;
	wxToolBarToolBase* _clearAllButton;

public:
	SelectionSetModule() :
		_toolMenu(nullptr),
		_clearAllButton(nullptr)
	{}

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
		_toolMenu = new SelectionSetToolmenu(toolbar);
		toolbar->AddControl(_toolMenu);

		_clearAllButton = toolbar->AddTool(CLEAR_TOOL_ID, "",
			wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + "delete.png"), _("Clear Selection Sets"));
#if 0 // TODO
		_clearAllButton->GetToolBar()->EnableTool(_clearAllButton->GetId(), !_selectionSets.empty());
#endif

		toolbar->Bind(wxEVT_TOOL, &SelectionSetModule::onDeleteAllSetsClicked, this, _clearAllButton->GetId());
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

	void onDeleteAllSetsClicked(wxCommandEvent& ev)
	{
		if (ev.GetId() != _clearAllButton->GetId())
		{
			ev.Skip();
			return; // not our business
		}

		if (!GlobalMapModule().getRoot())
		{
			rError() << "No map loaded, can't delete any sets" << std::endl;
			return;
		}

		ui::IDialogPtr dialog = GlobalDialogManager().createMessageBox(
			_("Delete all selection sets?"),
			_("This will delete all set definitions. The actual map objects will not be affected by this step.\n\nContinue with that operation?"),
			ui::IDialog::MESSAGE_ASK);

		if (dialog->run() == ui::IDialog::RESULT_YES)
		{
			GlobalMapModule().getRoot()->getSelectionSetManager().deleteAllSelectionSets();
		}
	}
};

module::StaticModule<SelectionSetModule> selectionSetModule;

}
