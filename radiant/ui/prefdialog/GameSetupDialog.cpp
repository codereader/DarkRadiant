#include "GameSetupDialog.h"

#include "imainframe.h"
#include "i18n.h"
#include "igame.h"
#include "modulesystem/ModuleRegistry.h"

#include "wxutil/dialog/MessageBox.h"
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

	wxBoxSizer* buttonHBox = new wxBoxSizer(wxHORIZONTAL);

	// Create the Save button
	wxButton* saveButton = new wxButton(this, wxID_SAVE);
	saveButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(GameSetupDialog::onSave), nullptr, this);

	// Create the assign shortcut button
	wxButton* cancelButton = new wxButton(this, wxID_CANCEL);
	cancelButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(GameSetupDialog::onCancel), NULL, this);

	buttonHBox->Add(saveButton, 0, wxRIGHT, 6);
	buttonHBox->Add(cancelButton, 0, wxRIGHT, 6);

	mainVbox->Add(buttonHBox, 0, wxALIGN_RIGHT | wxTOP | wxBOTTOM, 12);

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

GameSetupPage* GameSetupDialog::getSelectedPage()
{
	// Extract the game type value from the current page and save it to the registry
	wxWindow* container = _book->GetPage(_book->GetSelection());
	return dynamic_cast<GameSetupPage*>(wxWindow::FindWindowByName("GameSetupPage", container));
}

std::string GameSetupDialog::getSelectedGameType()
{
	// Extract the game type value from the current page and save it to the registry
	GameSetupPage* page = getSelectedPage();

	if (page == nullptr) return std::string();

	wxStringClientData* data = static_cast<wxStringClientData*>(page->GetClientData());

	return data->GetData().ToStdString();
}

void GameSetupDialog::onSave(wxCommandEvent& ev)
{
	if (getSelectedGameType().empty())
	{
		// Ask the user to select a game type
		wxutil::Messagebox::Show(_("Invalid Settings"), 
			_("Please select a game type"), ui::IDialog::MESSAGE_CONFIRM, nullptr);
		return;
	}

	GameSetupPage* page = GameSetupDialog::getSelectedPage();
	assert(page != nullptr);

	try
	{
		page->validateSettings();
	}
	catch (GameSettingsInvalidException& ex)
	{
		std::string msg = fmt::format(_("Warning:\n{0}\nDo you want to correct these settings?"), ex.what());

		if (wxutil::Messagebox::Show(_("Invalid Settings"), 
				msg, ui::IDialog::MESSAGE_ASK, nullptr) == wxutil::Messagebox::RESULT_YES)
		{
			// User wants to correct the settings, don't exit
			return;
		}
	}

	EndModal(wxID_OK);
}

void GameSetupDialog::onCancel(wxCommandEvent& ev)
{
	EndModal(wxID_CANCEL);
}

GameSetupDialog::Result GameSetupDialog::Show(const cmd::ArgumentList& args)
{
	GameSetupDialog::Result result;

	// greebo: Check if the mainframe module is already "existing". It might be
	// uninitialised if this dialog is shown during DarkRadiant startup
	wxWindow* parent = module::GlobalModuleRegistry().moduleExists(MODULE_MAINFRAME) ?
		GlobalMainFrame().getWxTopLevelWindow() : nullptr;

	GameSetupDialog* dialog = new GameSetupDialog(parent);

	if (dialog->ShowModal() == wxID_OK)
	{
		result.gameType = dialog->getSelectedGameType();

		if (result.gameType.empty())
		{
			rError() << "Cannot save game paths, nothing selected" << std::endl;
			return result;
		}

		GameSetupPage* page = dialog->getSelectedPage();

		result.enginePath = page->getEnginePath();
		result.modPath = page->getModPath();
		result.modBasePath = page->getModBasePath();
	}
	
	dialog->Destroy();

	return result;
}

}
