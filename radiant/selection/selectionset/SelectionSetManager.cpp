#include "SelectionSetManager.h"

#include "itextstream.h"
#include "i18n.h"
#include "iselection.h"
#include "idialogmanager.h"
#include "imapinfofile.h"
#include "iuimanager.h"
#include "ieventmanager.h"
#include "imainframe.h"
#include "modulesystem/StaticModule.h"
#include "SelectionSetToolmenu.h"
#include "SelectionSetInfoFileModule.h"

#include <wx/toolbar.h>
#include <wx/frame.h>
#include <wx/artprov.h>
#include <wx/stattext.h>

#include <functional>

namespace selection
{

namespace
{
	// Tool items created by the ToolBarManager carry ID >= 100
	const int CLEAR_TOOL_ID = 1;
}

SelectionSetManager::SelectionSetManager() :
    _toolMenu(NULL),
    _clearAllButton(NULL)
{}

const std::string& SelectionSetManager::getName() const
{
	static std::string _name("SelectionSetManager");
	return _name;
}

const StringSet& SelectionSetManager::getDependencies() const
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

void SelectionSetManager::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	// Register for the startup event
	GlobalRadiant().signal_radiantStarted().connect(
        sigc::mem_fun(this, &SelectionSetManager::onRadiantStartup)
    );

	GlobalCommandSystem().addCommand("DeleteAllSelectionSets",
		std::bind(&SelectionSetManager::deleteAllSelectionSetsCmd, this, std::placeholders::_1));

	GlobalEventManager().addCommand("DeleteAllSelectionSets", "DeleteAllSelectionSets");

	GlobalMapModule().signal_mapEvent().connect(
		sigc::mem_fun(*this, &SelectionSetManager::onMapEvent)
	);

	GlobalMapInfoFileManager().registerInfoFileModule(
		std::make_shared<SelectionSetInfoFileModule>()
	);
}

void SelectionSetManager::shutdownModule()
{
	_sigSelectionSetsChanged.clear();
	_selectionSets.clear();
}

void SelectionSetManager::onMapEvent(IMap::MapEvent ev)
{
	if (ev == IMap::MapUnloaded)
	{
		deleteAllSelectionSets();
	}
}

void SelectionSetManager::onRadiantStartup()
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
	_clearAllButton->GetToolBar()->EnableTool(_clearAllButton->GetId(), !_selectionSets.empty());

	toolbar->Connect(_clearAllButton->GetId(), wxEVT_TOOL, 
		wxCommandEventHandler(SelectionSetManager::onDeleteAllSetsClicked), NULL, this);

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

sigc::signal<void> SelectionSetManager::signal_selectionSetsChanged() const
{
    return _sigSelectionSetsChanged;
}

void SelectionSetManager::foreachSelectionSet(const VisitorFunc& functor)
{
	for (SelectionSets::const_iterator i = _selectionSets.begin(); i != _selectionSets.end(); )
	{
		functor((i++)->second);
	}
}

void SelectionSetManager::foreachSelectionSet(Visitor& visitor)
{
	foreachSelectionSet([&] (const ISelectionSetPtr& set)
	{
		visitor.visit(set);
	});
}

ISelectionSetPtr SelectionSetManager::createSelectionSet(const std::string& name)
{
	SelectionSets::iterator i = _selectionSets.find(name);

	if (i == _selectionSets.end())
	{
		// Create new set
		std::pair<SelectionSets::iterator, bool> result = _selectionSets.insert(
			SelectionSets::value_type(name, SelectionSetPtr(new SelectionSet(name))));

		i = result.first;

		_sigSelectionSetsChanged();

        if (_clearAllButton)
        {
            _clearAllButton->GetToolBar()->EnableTool(_clearAllButton->GetId(), !_selectionSets.empty());
        }
	}

	return i->second;
}

void SelectionSetManager::deleteSelectionSet(const std::string& name)
{
	SelectionSets::iterator i = _selectionSets.find(name);

    if (i != _selectionSets.end())
    {
        _selectionSets.erase(i);

        _sigSelectionSetsChanged();

        if (_clearAllButton)
        {
            _clearAllButton->GetToolBar()->EnableTool(_clearAllButton->GetId(), !_selectionSets.empty());
        }
    }
}

void SelectionSetManager::deleteAllSelectionSets()
{
	_selectionSets.clear();
	_sigSelectionSetsChanged();

    if (_clearAllButton)
    {
        _clearAllButton->GetToolBar()->EnableTool(_clearAllButton->GetId(), false);
    }
}

void SelectionSetManager::deleteAllSelectionSetsCmd(const cmd::ArgumentList& args)
{
	deleteAllSelectionSets();
}

ISelectionSetPtr SelectionSetManager::findSelectionSet(const std::string& name)
{
	SelectionSets::iterator i = _selectionSets.find(name);

	return (i != _selectionSets.end()) ? i->second : ISelectionSetPtr();
}

void SelectionSetManager::onDeleteAllSetsClicked(wxCommandEvent& ev)
{
	if (ev.GetId() != _clearAllButton->GetId())
	{
		ev.Skip();
		return; // not our business
	}

	ui::IDialogPtr dialog = GlobalDialogManager().createMessageBox(
		_("Delete all selection sets?"),
		_("This will delete all set definitions. The actual map objects will not be affected by this step.\n\nContinue with that operation?"),
		ui::IDialog::MESSAGE_ASK);

	ui::IDialog::Result result = dialog->run();

	if (result == ui::IDialog::RESULT_YES)
	{
		deleteAllSelectionSets();
	}
}

// Define the static SelectionSetManager module
module::StaticModule<SelectionSetManager> selectionSetManager;

} // namespace
