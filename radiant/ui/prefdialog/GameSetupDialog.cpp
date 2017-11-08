#include "GameSetupDialog.h"

#include "imainframe.h"
#include "i18n.h"
#include "igame.h"
#include "modulesystem/ModuleRegistry.h"

#include "registry/registry.h"
#include "GameSetupPageIdTech.h"
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/choicebk.h>
#include <wx/stattext.h>

namespace ui
{

GameSetupDialog::GameSetupDialog(wxWindow* parent) :
	DialogBase(_("Game Setup"), parent),
	_book(nullptr)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	// 12-pixel spacer
	wxBoxSizer* mainVbox = new wxBoxSizer(wxVERTICAL);
	GetSizer()->Add(mainVbox, 1, wxEXPAND | wxALL, 12);

	_book = new wxChoicebook(this, wxID_ANY);
	wxStaticText* label = new wxStaticText(this, wxID_ANY, _("Please select a Game Type:"));

	mainVbox->Add(label);
	mainVbox->Add(_book, 1, wxEXPAND);

	mainVbox->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT);

	initialiseControls();

	Layout();
	Fit();
}

void GameSetupDialog::initialiseControls()
{
	const game::IGameManager::GameList& games = GlobalGameManager().getSortedGameList();

	for (const game::IGamePtr& game : games)
	{
		wxPanel* container = new wxPanel(_book, wxID_ANY);
		container->SetSizer(new wxBoxSizer(wxVERTICAL));

		// Check the game setup dialog type
		std::string type = GameSetupPageIdTech::TYPE();

		xml::NodeList nodes = game->getLocalXPath("/gameSetup/dialog");

		if (!nodes.empty())
		{
			std::string value = nodes[0].getAttributeValue("type");

			if (!value.empty())
			{
				type = value;
			}
		}

		// For each game type create a separate page in the choice book
		GameSetupPage* page = GameSetupPage::CreatePageForType(type, container);
		page->SetName("GameSetupPage");

		// Store the game value as client data into the page object
		page->SetClientData(new wxStringClientData(game->getKeyValue("name")));

		container->GetSizer()->Add(page, 1, wxEXPAND | wxALL, 12);

		_book->AddPage(container, game->getKeyValue("name"));
	}
}

void GameSetupDialog::save()
{
	if (_book->GetSelection() == wxNOT_FOUND)
	{
		rError() << "Cannot save game type, nothing selected" << std::endl;
		return;
	}

	// Extract the game type value from the current page and save it to the registry
	wxWindow* container = _book->GetPage(_book->GetSelection());
	GameSetupPage* page = dynamic_cast<GameSetupPage*>(wxWindow::FindWindowByName("GameSetupPage", container));

	wxStringClientData* data = static_cast<wxStringClientData*>(page->GetClientData());

	std::string selectedGame = data->GetData().ToStdString();
	registry::setValue(RKEY_GAME_TYPE, selectedGame);

	// Ask the current page to set the paths to the registry
	page->saveSettings();
}

void GameSetupDialog::Show(const cmd::ArgumentList& args)
{
	// greebo: Check if the mainframe module is already "existing". It might be
	// uninitialised if this dialog is shown during DarkRadiant startup
	wxWindow* parent = module::GlobalModuleRegistry().moduleExists(MODULE_MAINFRAME) ?
		GlobalMainFrame().getWxTopLevelWindow() : nullptr;

	GameSetupDialog* dialog = new GameSetupDialog(parent);

	int result = dialog->ShowModal();

	if (result == wxID_OK)
	{
		dialog->save();
	}
	
	dialog->Destroy();
}

}
