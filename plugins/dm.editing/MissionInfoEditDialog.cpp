#include "MissionInfoEditDialog.h"

#include "itextstream.h"
#include "i18n.h"
#include <sigc++/functors/mem_fun.h>

#include <fmt/format.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include "wxutil/dialog/MessageBox.h"

namespace ui
{

namespace
{
	const char* const WINDOW_TITLE = N_("Mission Info Editor (darkmod.txt)");
}

MissionInfoEditDialog::MissionInfoEditDialog(wxWindow* parent) :
	DialogBase(_(WINDOW_TITLE), parent)
{
	populateWindow();

	try
	{
		_darkmodTxt = map::DarkmodTxt::LoadForCurrentMod();
	}
	catch (map::DarkmodTxt::ParseException& ex)
	{
		rError() << "Failed to parse darkmod.txt: " << ex.what() << std::endl;

		wxutil::Messagebox::ShowError(
			fmt::format(_("Failed to parse darkmod.txt:\n{0}"), ex.what()), this);

		// Reset the file to defaults
		_darkmodTxt = std::make_shared<map::DarkmodTxt>();
	}

	updateValuesFromDarkmodTxt();
}

void MissionInfoEditDialog::updateValuesFromDarkmodTxt()
{
	if (!_darkmodTxt)
	{
		findNamedObject<wxTextCtrl>(this, "MissionInfoEditDialogTitleEntry")->SetValue("");
		findNamedObject<wxTextCtrl>(this, "MissionInfoEditDialogAuthorEntry")->SetValue("");
		findNamedObject<wxTextCtrl>(this, "MissionInfoEditDialogDescriptionEntry")->SetValue("");
		findNamedObject<wxTextCtrl>(this, "MissionInfoEditDialogVersionEntry")->SetValue("");
		findNamedObject<wxTextCtrl>(this, "MissionInfoEditDialogReqTdmVersionEntry")->SetValue("");

		return;
	}

	findNamedObject<wxTextCtrl>(this, "MissionInfoEditDialogTitleEntry")->SetValue(_darkmodTxt->getTitle());
	findNamedObject<wxTextCtrl>(this, "MissionInfoEditDialogAuthorEntry")->SetValue(_darkmodTxt->getAuthor());
	findNamedObject<wxTextCtrl>(this, "MissionInfoEditDialogDescriptionEntry")->SetValue(_darkmodTxt->getDescription());
	findNamedObject<wxTextCtrl>(this, "MissionInfoEditDialogVersionEntry")->SetValue(_darkmodTxt->getVersion());
	findNamedObject<wxTextCtrl>(this, "MissionInfoEditDialogReqTdmVersionEntry")->SetValue(_darkmodTxt->getReqTdmVersion());
}

void MissionInfoEditDialog::populateWindow()
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxPanel* panel = loadNamedPanel(this, "MissionInfoEditDialogMainPanel");
	GetSizer()->Add(panel, 1, wxEXPAND);

	makeLabelBold(this, "MissionInfoLabel");

	wxButton* saveButton = findNamedObject<wxButton>(this, "MissionInfoEditDialogSaveButton");
	wxButton* cancelButton = findNamedObject<wxButton>(this, "MissionInfoEditDialogCancelButton");

	saveButton->Bind(wxEVT_BUTTON, sigc::mem_fun(this, &MissionInfoEditDialog::onSave));
	cancelButton->Bind(wxEVT_BUTTON, sigc::mem_fun(this, &MissionInfoEditDialog::onCancel));
		
	Layout();
	Fit();
	CenterOnScreen();
}

void MissionInfoEditDialog::onSave(wxCommandEvent& ev)
{
	// Close the dialog
	EndModal(wxID_OK);
}

void MissionInfoEditDialog::onCancel(wxCommandEvent& ev)
{
	// destroy dialog without saving
	EndModal(wxID_CANCEL);
}

void MissionInfoEditDialog::ShowDialog(const cmd::ArgumentList& args)
{
	MissionInfoEditDialog* instance = new MissionInfoEditDialog;

	instance->ShowModal();
	instance->Destroy();
}

}
