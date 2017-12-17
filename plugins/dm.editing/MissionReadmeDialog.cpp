#include "MissionReadmeDialog.h"

#include "igui.h"
#include "itextstream.h"
#include "i18n.h"
#include <sigc++/functors/mem_fun.h>

#include <functional>
#include <fmt/format.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include "wxutil/preview/GuiView.h"
#include "wxutil/dialog/MessageBox.h"

#include "MissionInfoGuiView.h"

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Mission Readme Editor (readme.txt)");
}

MissionReadmeDialog::MissionReadmeDialog(wxWindow* parent) :
	DialogBase(_(WINDOW_TITLE), parent),
	_updateInProgress(false)
{
	populateWindow();

	try
	{
		_readmeFile = map::ReadmeTxt::LoadForCurrentMod();
	}
	catch (map::ReadmeTxt::ParseException& ex)
	{
		rError() << "Failed to parse readme.txt: " << ex.what() << std::endl;

		wxutil::Messagebox::ShowError(
			fmt::format(_("Failed to parse readme.txt:\n{0}"), ex.what()), this);

		// Reset the file to defaults
		_readmeFile.reset();
	}

	_guiView->setGui(GlobalGuiManager().getGui("guis/mainmenu.gui"));
	//_guiView->setMissionInfoFile(_readmeFile);

	updateValuesFromReadmeFile();
}

void MissionReadmeDialog::updateValuesFromReadmeFile()
{
	assert(_readmeFile); // this should be non-NULL at all times

	if (!_readmeFile) return;

	_updateInProgress = true;

	findNamedObject<wxTextCtrl>(this, "MissionInfoReadmeContentsEntry")->SetValue(_readmeFile->getContents());
	findNamedObject<wxStaticText>(this, "MissionInfoReadmeOutputPath")->SetLabelText(_readmeFile->getFullOutputPath());

	_guiView->update();

	_updateInProgress = false;
}

void MissionReadmeDialog::populateWindow()
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxPanel* panel = loadNamedPanel(this, "MissionInfoReadmeDialogMainPanel");
	GetSizer()->Add(panel, 1, wxEXPAND);

	// Add the preview widget
	wxPanel* previewPanel = findNamedObject<wxPanel>(this, "MissionInfoReadmeDialogPreviewPanel");

	_guiView = new MissionInfoGuiView(previewPanel);
	previewPanel->GetSizer()->Add(_guiView, 1, wxEXPAND);

	makeLabelBold(this, "MissionReadmeLabel");

	wxButton* saveButton = findNamedObject<wxButton>(this, "MissionInfoReadmeDialogSaveButton");
	wxButton* cancelButton = findNamedObject<wxButton>(this, "MissionInfoReadmeDialogCancelButton");

	saveButton->Bind(wxEVT_BUTTON, sigc::mem_fun(this, &MissionReadmeDialog::onSave));
	cancelButton->Bind(wxEVT_BUTTON, sigc::mem_fun(this, &MissionReadmeDialog::onCancel));

	// Wire up the text entry boxes to update the preview
	setupNamedEntryBox("MissionInfoReadmeContentsEntry");

	Layout();
	Fit();
	CenterOnScreen();
}

void MissionReadmeDialog::setupNamedEntryBox(const std::string& ctrlName)
{
	wxTextCtrl* ctrl = findNamedObject<wxTextCtrl>(this, ctrlName);

	assert(ctrl != nullptr);
	if (ctrl == nullptr) return;

	ctrl->Bind(wxEVT_TEXT, [this](wxCommandEvent& ev)
	{
		if (_updateInProgress) return;

		// Load the values from the UI to the ReadmeTxt instance
		_readmeFile->setContents(findNamedObject<wxTextCtrl>(this, "MissionInfoReadmeContentsEntry")->GetValue().ToStdString());
		
		_guiView->update();
	});
}

void MissionReadmeDialog::onSave(wxCommandEvent& ev)
{
	try
	{
		// ReadmeTxt is kept in sync all the time, no need to load anything, just save to disk
		_readmeFile->saveToCurrentMod();

		// Close the dialog
		EndModal(wxID_OK);
	}
	catch (std::runtime_error& err)
	{
		wxutil::Messagebox::ShowError(err.what(), this);
	}
}

void MissionReadmeDialog::onCancel(wxCommandEvent& ev)
{
	// destroy dialog without saving
	EndModal(wxID_CANCEL);
}

void MissionReadmeDialog::ShowDialog(const cmd::ArgumentList& args)
{
	MissionReadmeDialog* instance = new MissionReadmeDialog;

	instance->ShowModal();
	instance->Destroy();
}

}
