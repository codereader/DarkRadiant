#include "GameSetupDialog.h"

#include "imainframe.h"
#include "i18n.h"
#include "igame.h"
#include "modulesystem/ModuleRegistry.h"

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

		container->GetSizer()->Add(page, 1, wxEXPAND | wxALL, 12);

		_book->AddPage(container, game->getKeyValue("name"));
	}
}

void GameSetupDialog::Show(const cmd::ArgumentList& args)
{
	// greebo: Check if the mainframe module is already "existing". It might be
	// uninitialised if this dialog is shown during DarkRadiant startup
	wxWindow* parent = module::GlobalModuleRegistry().moduleExists(MODULE_MAINFRAME) ?
		GlobalMainFrame().getWxTopLevelWindow() : nullptr;

	GameSetupDialog* dialog = new GameSetupDialog(parent);

	dialog->ShowModal();
	
	dialog->Destroy();
}

}
