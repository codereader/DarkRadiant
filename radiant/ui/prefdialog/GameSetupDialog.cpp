#include "GameSetupDialog.h"

#include "imainframe.h"
#include "i18n.h"
#include "igame.h"
#include "modulesystem/ModuleRegistry.h"

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
	SetMinClientSize(wxSize(640, -1));

	// 12-pixel spacer
	wxBoxSizer* mainVbox = new wxBoxSizer(wxVERTICAL);
	GetSizer()->Add(mainVbox, 1, wxEXPAND | wxALL, 12);

	_book = new wxChoicebook(this, wxID_ANY);
	wxStaticText* label = new wxStaticText(this, wxID_ANY, _("Please select a Game Type:"));

	mainVbox->Add(label);
	mainVbox->Add(_book, 1, wxEXPAND);

	mainVbox->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT);

	initialiseControls();
}

void GameSetupDialog::initialiseControls()
{
	wxChoice* choice = _book->GetChoiceCtrl();

	const game::IGameManager::GameList& games = GlobalGameManager().getSortedGameList();

	for (const game::IGamePtr& game : games)
	{
		choice->AppendString(game->getKeyValue("name"));

		wxPanel* container = new wxPanel(_book, wxID_ANY);
		container->SetSizer(new wxBoxSizer(wxVERTICAL));

		// For each game type create a separate page in the choice book
		GameSetupPage* page = GameSetupPage::CreatePageForType(game->getKeyValue("type"), _book);

		container->GetSizer()->Add(page, 1, wxEXPAND, 12);
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
