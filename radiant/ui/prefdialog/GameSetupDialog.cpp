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
#include <wx/button.h>
#include "settings/GameManager.h"

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
	_book->Connect(wxEVT_CHOICEBOOK_PAGE_CHANGED, wxBookCtrlEventHandler(GameSetupDialog::onPageChanged), nullptr, this);

	wxStaticText* label = new wxStaticText(this, wxID_ANY, _("Game Type:"));
	label->SetFont(label->GetFont().Bold());

	mainVbox->Add(label, 0, wxBOTTOM, 6);
	mainVbox->Add(_book, 1, wxEXPAND);

	wxBoxSizer* buttonHBox = new wxBoxSizer(wxHORIZONTAL);

	// Create the Save button
	wxButton* saveButton = new wxButton(this, wxID_SAVE);
	saveButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(GameSetupDialog::onSave), nullptr, this);

	// Create the assign shortcut button
	wxButton* cancelButton = new wxButton(this, wxID_CANCEL);
	cancelButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(GameSetupDialog::onCancel), nullptr, this);

	buttonHBox->Add(saveButton, 0, wxRIGHT, 6);
	buttonHBox->Add(cancelButton, 0, wxRIGHT, 6);

	mainVbox->Add(buttonHBox, 0, wxALIGN_RIGHT | wxTOP | wxBOTTOM, 12);

	initialiseControls();

	Layout();
	Fit();
	CenterOnParent();
}

void GameSetupDialog::initialiseControls()
{
	const game::IGameManager::GameList& games = GlobalGameManager().getSortedGameList();

	for (const game::IGamePtr& game : games)
	{
		wxPanel* container = new wxPanel(_book, wxID_ANY);
		container->SetSizer(new wxBoxSizer(wxVERTICAL));

		// For each game type create a separate page in the choice book
		GameSetupPage* page = GameSetupPage::CreatePageForGame(game, container);
		page->SetName("GameSetupPage");

		// Store the game value as client data into the page object
		page->SetClientData(new wxStringClientData(game->getKeyValue("name")));

		container->GetSizer()->Add(page, 1, wxEXPAND | wxALL, 12);

		_book->AddPage(container, game->getKeyValue("name"));
	}

	// Select the currently active game type
	setSelectedPage(registry::getValue<std::string>(RKEY_GAME_TYPE));
}

void GameSetupDialog::setSelectedPage(const std::string& name)
{
	for (std::size_t i = 0; i < _book->GetPageCount(); ++i)
	{
		wxWindow* container = _book->GetPage(i);

		GameSetupPage* page = dynamic_cast<GameSetupPage*>(wxWindow::FindWindowByName("GameSetupPage", container));

		assert(page != nullptr);

		wxStringClientData* data = static_cast<wxStringClientData*>(page->GetClientData());

		if (data->GetData() == name)
		{
			_book->SetSelection(i);
			page->onPageShown();
			return;
		}
	}
}

GameSetupPage* GameSetupDialog::getSelectedPage()
{
	return getPage(_book->GetSelection());
}

GameSetupPage* GameSetupDialog::getPage(int num)
{
	// Find the actual GameSetupPage in the window hierarchy
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

void GameSetupDialog::tryEndModal(wxStandardID result)
{
	if (getSelectedGameType().empty())
	{
		// Ask the user to select a game type
		wxutil::Messagebox::Show(_("Invalid Settings"),
			_("Please select a game type"), IDialog::MESSAGE_CONFIRM, nullptr);
		return;
	}

	GameSetupPage* page = getSelectedPage();
	assert(page != nullptr);

	try
	{
		page->validateSettings();
	}
	catch (GameSettingsInvalidException& ex)
	{
		std::string msg = fmt::format(_("Warning:\n{0}\nDo you want to correct these settings?"), ex.what());

		if (wxutil::Messagebox::Show(_("Invalid Settings"),
			msg, IDialog::MESSAGE_ASK, nullptr) == wxutil::Messagebox::RESULT_YES)
		{
			// User wants to correct the settings, don't exit
			return;
		}
	}

	// Fire the close event
	page->onClose();

	EndModal(result);
}

void GameSetupDialog::onSave(wxCommandEvent& ev)
{
	GameSetupPage* page = getSelectedPage();
	assert(page != nullptr);

	if (!page->onPreSave())
	{
		return; // pre-save action returned false, prevent dialog close
	}

	// Confirm valid or invalid settings and end the dialog
	tryEndModal(wxID_OK);
}

void GameSetupDialog::onCancel(wxCommandEvent& ev)
{
	// Confirm valid or invalid settings and end the dialog
	tryEndModal(wxID_CANCEL);
}

void GameSetupDialog::onPageChanged(wxBookCtrlEvent& ev)
{
	if (ev.GetSelection() != wxNOT_FOUND)
	{
		GameSetupPage* page = getPage(ev.GetSelection());

		if (page != nullptr)
		{
			page->onPageShown();
		}
	}
}

void GameSetupDialog::Show(const cmd::ArgumentList& args)
{
	// greebo: Check if the mainframe module is already "existing". It might be
	// uninitialised if this dialog is shown during DarkRadiant startup
	wxWindow* parent = module::GlobalModuleRegistry().moduleExists(MODULE_MAINFRAME) ?
		GlobalMainFrame().getWxTopLevelWindow() : nullptr;

	GameSetupDialog* dialog = new GameSetupDialog(parent);

	if (dialog->ShowModal() == wxID_OK)
	{
		GameSetupPage* page = dialog->getSelectedPage();

		assert(page != nullptr);

		// Copy the values from the page instance to our result
		const game::GameConfiguration& config = page->getConfiguration();

		// Apply the configuration (don't use the GlobalGameManager accessor yet)
		game::Manager::Instance().applyConfig(config);
	}
	
	dialog->Destroy();
}

}
